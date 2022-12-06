#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

int HP_CreateFile(char *fileName){
      BF_CreateFile(fileName);  
    int file_desc, blocks_num;
    CALL_BF(BF_OpenFile(fileName,&file_desc)); 
    printf("file has file desc: %d\n", file_desc); 
    
    CALL_BF(BF_GetBlockCounter(file_desc,&blocks_num)); 
    printf("file has: %d,  blocks\n", blocks_num);  

    BF_Block * block; 
    BF_Block_Init(&block); 
    BF_AllocateBlock(file_desc,block); 

    void* data = BF_Block_GetData(block);
    
    HP_info* hp_info = data; 
    hp_info->file_type = "heap file"; 
    hp_info->file_desc = file_desc;
    hp_info->last_block_id = 0;
    hp_info->max_records = BF_BLOCK_SIZE/sizeof(Record); 
    hp_info->blocks_number=1; 

    BF_Block_SetDirty(block); 
    printf("unpinning and deleting blocks\n");
    CALL_BF(BF_UnpinBlock(block)); 
    BF_Block_Destroy(&block); 
    CALL_BF(BF_GetBlockCounter(file_desc,&blocks_num)); 
    printf("file has: %d,  blocks\n", blocks_num);  

    // char * changed_data = BF_Block_GetData

    CALL_BF(BF_CloseFile(file_desc)); 
    return 0;
}

HP_info* HP_OpenFile(char *fileName){
  int file_desc; 
  CALL_BF(BF_OpenFile(fileName, &file_desc));

  
  BF_Block *block;
  BF_Block_Init(&block);

  CALL_BF(BF_GetBlock(file_desc, 0, block)); 
  void* data;
  data = BF_Block_GetData(block); 
  HP_info* hp_info; 
  hp_info = data; 
  char* file_type = hp_info->file_type; 
  printf("file opened with name %s is of type: %s\n", fileName, file_type); 


  CALL_BF(BF_UnpinBlock(block)); 
  BF_Block_Destroy(&block); 
  if (file_type == "heap file") {
    return hp_info; 
  }
  
  // CALL_BF(BF_UnpinBlock(block)); 
  // BF_Block_Destroy(&block); 

  //prosthiki twn swstwn plhroforiwn sto hp info (vlepe parousiasi diafaneia 49)
    return NULL ;
}


int HP_CloseFile( HP_info* hp_info ){
    CALL_BF(BF_CloseFile(hp_info->file_desc));
    return 0;
}

int HP_InsertEntry(HP_info* hp_info, Record record){
  int last_block_id = hp_info->last_block_id; 
  int fd = hp_info->file_desc; 

  printf("record: %s\n", record.name);
  
  BF_Block *block;
  BF_Block_Init(&block); 

  if (last_block_id == 0) {
    CALL_BF(BF_AllocateBlock(fd,block));
    hp_info->last_block_id+=1; 
    void* data = BF_Block_GetData(block);

    HP_block_info * hp_block_info = data;    //metadedomena sygkekrimenou block
    hp_block_info->next_block = 2;
    hp_block_info->records_number = 0; 
    
    hp_info->blocks_number++;               //metadedomena arxeioy 

    

    // printf("size of block 1 is: %ld", sizeof(block)); 

    // CALL_BF(BF_UnpinBlock(block)); 
  }else {
    printf("last block id != 0"); 
    CALL_BF(BF_GetBlock(hp_info->file_desc,last_block_id,block)); 
  }

  void * data = BF_Block_GetData(block); 
  HP_block_info * hp_block_info = data; 


  int rec_space = hp_block_info->records_number * sizeof(Record);   // to records number einai uninitialized ara pairnei terastia timh to free_space

  
  printf("rec space %d", rec_space); 
  int free_space = BF_BLOCK_SIZE - sizeof(HP_block_info); // - (hp_block_info->records_number * sizeof(Record));

  // int offset = BF_BLOCK_SIZE - free_space - sizeof(HP_block_info);   
  // printf("free space , sizeofrecord %d,%ld", free_space,sizeof(record)); 
  // printf("data, offset %d", offset); 
  printf("free space: %d\n", free_space); 
  if (free_space >= sizeof(record)) {               ///3a 
    printf("exw xwro\n"); 
    void * data = BF_Block_GetData(block); 
    HP_block_info * hp_block_info = data; 
    int total_records = hp_block_info->records_number; 
    int offset = total_records*sizeof(record); 
    memcpy(data+offset,&record,sizeof(record)); 
    hp_block_info->records_number++; 
    printf("to block exei xwro: %ld", sizeof(block)); 
    // printf("records number %d\n", hp_block_info->records_number);
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));  
  }else {                                           //3b 
    printf("DEN EXW XWRO\n");
    BF_Block *new_block;
    BF_Block_Init(&new_block); 
    CALL_BF(BF_AllocateBlock(fd,new_block)); 
    hp_info->blocks_number++;
    hp_info->last_block_id++;

    void* data = BF_Block_GetData(new_block);
    int blocks_num; 
    CALL_BF(BF_GetBlockCounter(fd,&blocks_num))

    HP_block_info * hp_block_info = data;    //metadedomena sygkekrimenou block
    hp_block_info->next_block = blocks_num; 
    hp_block_info->records_number = 0;

    int free_space = BF_BLOCK_SIZE - sizeof(hp_block_info); 
    int offset = BF_BLOCK_SIZE - free_space; 
    memcpy(data+offset,&record,sizeof(record)); 
    
    hp_block_info->records_number++; 
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));  
  }
  
 
  // CALL_BF(BF_UnpinBlock(block));  
  BF_Block_Destroy(&block); 

  return 0;
}

int HP_GetAllEntries(HP_info* hp_info, int value){
   return 0;
}

