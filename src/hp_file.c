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
    return 0;
}

int HP_InsertEntry(HP_info* hp_info, Record record){
    return 0;
}

int HP_GetAllEntries(HP_info* hp_info, int value){
   return 0;
}

