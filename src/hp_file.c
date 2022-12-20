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
    int file_desc, blocks_num;
    BF_CreateFile(fileName);  
    CALL_BF(BF_OpenFile(fileName,&file_desc)); 
    
    CALL_BF(BF_GetBlockCounter(file_desc,&blocks_num)); 

    BF_Block * block;                 //Allocating block 0 of file 
    BF_Block_Init(&block); 
    BF_AllocateBlock(file_desc,block); 

    void* data = BF_Block_GetData(block);
    HP_info hp_info; 
    hp_info.file_type = "heap file";             
    hp_info.file_desc = file_desc;
    hp_info.max_records = BF_BLOCK_SIZE/sizeof(Record); 
    hp_info.blocks_number=1; 
    hp_info.last_block_id = 0;
    
    memcpy(data,&hp_info,sizeof(HP_info));    //Inserting metadata of file (HP_INFO) in block 0 

    BF_Block_SetDirty(block); 
    CALL_BF(BF_UnpinBlock(block)); 
    BF_Block_Destroy(&block); 
    


    CALL_BF(BF_CloseFile(file_desc)); 
    return 0;
}

HP_info* HP_OpenFile(char *fileName){
  int file_desc; 
  CALL_BF(BF_OpenFile(fileName, &file_desc));
  
  BF_Block *block;
  BF_Block_Init(&block); 

  CALL_BF(BF_GetBlock(file_desc, 0, block));     //Getting block 0 
  char* data; 
  data = BF_Block_GetData(block); 
  static HP_info hp_info; 
  memcpy(&hp_info,data,sizeof(HP_info));         // Taking file's metadata (HP_INFO) from block 0 

  CALL_BF(BF_UnpinBlock(block)); 
  BF_Block_Destroy(&block); 

  
  if (hp_info.file_type == "heap file") {     //If file is heap file then it's good so return hp info. 
    printf("file is good."); 
    return &hp_info;  
  }

    return NULL ;
}


int HP_CloseFile( HP_info* hp_info ){
    CALL_BF(BF_CloseFile(hp_info->file_desc));
    return 0;
}

int HP_InsertEntry(HP_info* hp_info, Record record){          
  int blocks_num; 
  int last_block_id = hp_info->last_block_id; 
  int fd = hp_info->file_desc; 

  
  BF_Block *block;                  //Initializing a block. 
  BF_Block_Init(&block); 

  if (hp_info->last_block_id == 0) {        //If file has 1 block (only block 0 which contains metadata) , then allocate a new block  
    CALL_BF(BF_AllocateBlock(fd,block));
    hp_info->last_block_id++; 
    hp_info->blocks_number++; 
    void* data = BF_Block_GetData(block);

    HP_block_info hp_block_info;        //metadata of block 1 (HP BLOCK INFO)
    hp_block_info.next_block=2;
    hp_block_info.records_number=0;


    int offset = hp_info->max_records*sizeof(Record);        
    memcpy(data+offset,&hp_block_info,sizeof(HP_block_info));        //inserting metadata of block 1 (hp block info ) inside block  

  }else {
     CALL_BF(BF_GetBlock(hp_info->file_desc,last_block_id,block));   //if file already has active blocks(meaning blocks that isn't block 0), then get last block 
   }

  
  char * data = BF_Block_GetData(block); 
  HP_block_info hp_block_info; 

  int hp_block_info_offset = hp_info->max_records*sizeof(Record);
  memcpy(&hp_block_info,data+hp_block_info_offset,sizeof(HP_block_info));          //get hp block info of given block 


  int total_records_space = hp_block_info.records_number * sizeof(Record); 
  
  int free_space = BF_BLOCK_SIZE - sizeof(HP_block_info) - total_records_space; 

 
  if (free_space >= sizeof(record)) {               /// if insertion of record doesn't overflow the block 
    int total_records = hp_block_info.records_number; 
    int offset = total_records*sizeof(Record);
    memcpy(data+offset,&record,sizeof(Record));       //insert the record in the block in the correct place(offset) 
    hp_block_info.records_number++; 
    memcpy(data+hp_block_info_offset,&hp_block_info,sizeof(HP_block_info));    //insert the new and changed metadata of block 
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);

  }else {                                           //if insertion of record overflows the given block 
                                                    // allocate a new block. putting hp_block_info in it in the correct offset, then adding the record 
    BF_Block *new_block; 
    BF_Block_Init(&new_block); 
    CALL_BF(BF_AllocateBlock(fd,new_block));  
    
    char* data = BF_Block_GetData(new_block); 

                                        //changing metadata of block 
    hp_block_info.next_block++;
    hp_block_info.records_number=0;

    int offset = hp_info->max_records*sizeof(Record);
    memcpy(data+offset,&hp_block_info,sizeof(HP_block_info));   //inserting the new,changed metadata in the block 

    memcpy(data,&record,sizeof(Record)); 

    hp_info->blocks_number++;             //changing the metadata of file(HP INFO)
    hp_info->last_block_id++;
  
    BF_Block_SetDirty(new_block);
    CALL_BF(BF_UnpinBlock(new_block));  
    BF_Block_Destroy(&new_block);  
    CALL_BF(BF_UnpinBlock(block));  
    BF_Block_Destroy(&block);  
  }

  


  BF_Block *block0;                                     //updating block 0 of file (hp info) 
  BF_Block_Init(&block0); 

  CALL_BF(BF_GetBlock(hp_info->file_desc, 0, block0)); 

  char* data0;
  data0 = BF_Block_GetData(block0); 
  static HP_info new_hp_info; 
  memcpy(&new_hp_info,data,sizeof(HP_info)); 
  new_hp_info.blocks_number = hp_info->blocks_number;
  new_hp_info.last_block_id = hp_info->last_block_id;

  BF_Block_SetDirty(block0); 
  CALL_BF(BF_UnpinBlock(block0));  


  BF_Block_Destroy(&block0); 

  return hp_info->last_block_id; 

  return 0;
}

int HP_GetAllEntries(HP_info* hp_info, int value){
  int total_blocks_read = 0; 
  int found_record = 0; 
 
  for (int i = 1; i < hp_info->blocks_number; i++) {         //iterating through blocks 
    BF_Block * temp_block; 
    BF_Block_Init(&temp_block); 

    CALL_BF(BF_GetBlock(hp_info->file_desc, i, temp_block)); 

    char* temp_data;
    temp_data = BF_Block_GetData(temp_block);           //getting blocks data 
    HP_block_info hp_block_info; 

    int hp_block_info_offset = hp_info->max_records*sizeof(Record);          //getting blocks metadata
    memcpy(&hp_block_info,temp_data+hp_block_info_offset,sizeof(HP_block_info));

    for (int j = 0; j < hp_block_info.records_number; j++) {      //iterating through records 
      int offset = j * sizeof(Record); 
      Record temp_record; 
      memcpy(&temp_record,temp_data+offset,sizeof(Record)); 
      if (temp_record.id == value) {
        found_record = 1; 
        printf("%s, id: %d, name: %s, surname: %s, city: %s \n", temp_record.record, temp_record.id, temp_record.name,
                                                                  temp_record.surname, temp_record.city); 
        total_blocks_read = i;                                          
      }
    }

    CALL_BF(BF_UnpinBlock(temp_block));  
    BF_Block_Destroy(&temp_block); 
   
    
     
  }    
  if (found_record == 1) {
        printf("total blocks read to find record: %d\n",total_blocks_read);
        return total_blocks_read;
  }
  
   return -1;
}

