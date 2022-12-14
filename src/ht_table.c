#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }


int HT_CreateFile(char *fileName,  int buckets){
    int file_desc, blocks_num;
    BF_CreateFile(fileName);  
    CALL_OR_DIE(BF_OpenFile(fileName,&file_desc)); 

    BF_Block * block; 
    BF_Block_Init(&block); 
    BF_AllocateBlock(file_desc,block); 

    void* data = BF_Block_GetData(block);
    HT_info ht_info; 
    ht_info.file_type = "hash file";            
    ht_info.file_desc = file_desc;
    ht_info.max_records = BF_BLOCK_SIZE/sizeof(Record); 
    ht_info.blocks_number=1; 
    ht_info.last_block_id = 0;
    ht_info.num_of_buckets = buckets;
    ht_info.hash_table[buckets]; 


    memcpy(data,&ht_info,sizeof(HT_info));

    BF_Block_SetDirty(block); 
    CALL_OR_DIE(BF_UnpinBlock(block)); 
    BF_Block_Destroy(&block); 
    

    CALL_OR_DIE(BF_CloseFile(file_desc)); 

    return 0;
}

HT_info* HT_OpenFile(char *fileName){
  int file_desc; 
  CALL_OR_DIE(BF_OpenFile(fileName, &file_desc));

  BF_Block *block;
  BF_Block_Init(&block); 

  CALL_OR_DIE(BF_GetBlock(file_desc, 0, block)); 
  char* data;
  data = BF_Block_GetData(block); 
  static HT_info ht_info; 
  memcpy(&ht_info,data,sizeof(HT_info)); 


  int offset = ht_info.max_records*sizeof(Record);

  for (int i = 0; i < ht_info.num_of_buckets; i++) {
    BF_Block *temp_block;
    BF_Block_Init(&temp_block);
    CALL_OR_DIE(BF_AllocateBlock(file_desc,temp_block)); 

    ht_info.last_block_id++; 
    ht_info.blocks_number++; 
    void* data = BF_Block_GetData(block);

    HT_block_info ht_block_info;        //metadedomena block 1 
    // ht_block_info.next_block=2;
    ht_block_info.records_number=0;
    ht_block_info.block_id = i + 1; 
    ht_info.hash_table[i] = ht_block_info.block_id; 

    memcpy(data+offset,&ht_block_info,sizeof(HT_block_info)); 

    BF_Block_SetDirty(temp_block); 
    CALL_OR_DIE(BF_UnpinBlock(temp_block)); 
    BF_Block_Destroy(&temp_block); 

  }



  BF_Block_SetDirty(block); 
  CALL_OR_DIE(BF_UnpinBlock(block)); 
  BF_Block_Destroy(&block); 

  
  if (ht_info.file_type == "hash file") {
    printf("file is good (hash file).\n"); 
    return &ht_info;  
  }

    return NULL;
}


int HT_CloseFile( HT_info* HT_info ){
    CALL_OR_DIE(BF_CloseFile(HT_info->file_desc));

    return 0;
}

int HT_InsertEntry(HT_info* ht_info, Record record){         // hash function = id%numofbuckets => auto sou dinei to bucket, kai apo ayto to bucket pairneis to block toy,
        //an xwraei h eggrafh thn bazeis, an oxi allocate kainoyrio kai allagh metadedomenwn ht info kai ht block info vlepe diafaneies 
  
  // BF_Block *block0;
  // BF_Block_Init(&block0); 
  // CALL_OR_DIE(BF_GetBlock(ht_info->file_desc,0,block0)); 
  
  
  // char * data = BF_Block_GetData(block0); 
  // memcpy(&ht_info,data,sizeof(HT_info)); 

  // for (int i = 0; i < ht_info->num_of_buckets; i++) {
  //   printf("bucket %d , %d\n", i , ht_info->hash_table[i]); 
  // }
  
  
  // CALL_OR_DIE(BF_UnpinBlock(block0));  


  // BF_Block_Destroy(&block0); 
    return 0;
}

int HT_GetAllEntries(HT_info* ht_info, void *value ){
    return 0;
}




