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

    BF_Block * block;           //Allocating block 0 of file 
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
    ht_info.hash_table[buckets];       //initializing the hash table array 

    


    memcpy(data,&ht_info,sizeof(HT_info));  //inserting metadata of file in block 0 (HT INFO)

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

  CALL_OR_DIE(BF_GetBlock(file_desc, 0, block));  //Getting block 0 of hash file 
  char* data;
  data = BF_Block_GetData(block); 
  static HT_info ht_info; 
  memcpy(&ht_info,data,sizeof(HT_info));     //Taking file's metadata (HT INFO )


  int offset = ht_info.max_records*sizeof(Record);

  for (int i = 0; i < ht_info.num_of_buckets; i++) {
    BF_Block *temp_block;
    BF_Block_Init(&temp_block);
    CALL_OR_DIE(BF_AllocateBlock(file_desc,temp_block));  //allocating as many blocks as the number of buckets 

    ht_info.last_block_id++; 
    ht_info.blocks_number++; 
    void* data = BF_Block_GetData(block);

    HT_block_info ht_block_info;        //metadata of block 
    ht_block_info.records_number=0;
    ht_block_info.block_id = i + 1; 
    ht_block_info.bucket = i; 
    ht_info.hash_table[i] = ht_block_info.block_id;  // e.g. bucket 0 = block 1 (in the hashtable array )

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

int HT_InsertEntry(HT_info* ht_info, Record record){        
  int bucket = record.id % ht_info->num_of_buckets;  //hash function 
  int block_number = ht_info->hash_table[bucket];    //getting the correct block to insert from hash function, from hash table array 

  BF_Block *block;
  BF_Block_Init(&block); 
  CALL_OR_DIE(BF_GetBlock(ht_info->file_desc,block_number,block));   //getting the correct block 

  HT_block_info ht_block_info; 
  char * data = BF_Block_GetData(block); 
  int ht_block_info_offset = ht_info->max_records*sizeof(Record);
  memcpy(&ht_block_info,data+ht_block_info_offset,sizeof(HT_block_info));  //getting blocks metadata (HT BLOCK INFO) 

  int total_records_space = ht_block_info.records_number * sizeof(Record); 
  
  int free_space = BF_BLOCK_SIZE - sizeof(HT_block_info) - total_records_space; 

  if (free_space >= sizeof(record)) {  // there is enough space for entry of record in block 
    int total_records = ht_block_info.records_number; 
    int offset = total_records * sizeof(Record);
    memcpy(data+offset,&record,sizeof(Record)); 
    ht_block_info.records_number++; 
    memcpy(data+ht_block_info_offset,&ht_block_info,sizeof(HT_block_info));
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));
    BF_Block_Destroy(&block); 
  } else {                   //there is not enough space for insertion of record in block 
    BF_Block *new_block; 
    BF_Block_Init(&new_block); 
    CALL_OR_DIE(BF_AllocateBlock(ht_info->file_desc,new_block));  //Allocate a new block 
    char* data = BF_Block_GetData(new_block); 
    HT_block_info new_ht_block_info; 
    new_ht_block_info.records_number=0;
    int blocksnum; 
    CALL_OR_DIE(BF_GetBlockCounter(ht_info->file_desc,&blocksnum));  //Getting which block it is (meaning block number)
    new_ht_block_info.block_id = blocksnum; 
    new_ht_block_info.overflowing_bucket = block_number;          //overflowing bucket is the previous block that overflows with the insertiong of record 
    new_ht_block_info.bucket = bucket;                            //also keeping which bucket the newly allocated block is on

    int offset = ht_info->max_records*sizeof(Record);

    memcpy(data,&record,sizeof(Record)); 

    memcpy(data+offset,&new_ht_block_info,sizeof(HT_block_info));

    
    ht_info->blocks_number++; 
    ht_info->last_block_id++;

    BF_Block_SetDirty(new_block);
    CALL_OR_DIE(BF_UnpinBlock(new_block));  
    BF_Block_Destroy(&new_block);  
    CALL_OR_DIE(BF_UnpinBlock(block));   
    BF_Block_Destroy(&block);  

  }
  
  BF_Block *block0;                                     //updating block 0 of file (hp info) 
  BF_Block_Init(&block0); 

  CALL_OR_DIE(BF_GetBlock(ht_info->file_desc, 0, block0)); 

  char* data0;
  data0 = BF_Block_GetData(block0); 
  static HT_info new_ht_info; 
  memcpy(&new_ht_info,data0,sizeof(HT_info)); 
  new_ht_info.blocks_number = ht_info->blocks_number;
  new_ht_info.last_block_id = ht_info->last_block_id;
  new_ht_info.hash_table[bucket] = ht_info->last_block_id; 

  BF_Block_SetDirty(block0); 
  CALL_OR_DIE(BF_UnpinBlock(block0));  
  BF_Block_Destroy(&block0); 

  return block_number; 

    return 0;
}

int HT_GetAllEntries(HT_info* ht_info, int value ){
  int total_blocks_read = 0; 
  int found_record = 0; 

  for (int i = 1; i < ht_info->blocks_number; i++) {   //iterating through blocks 
    BF_Block * temp_block; 
    BF_Block_Init(&temp_block); 

    CALL_OR_DIE(BF_GetBlock(ht_info->file_desc, i, temp_block)); 

    char* temp_data;
    temp_data = BF_Block_GetData(temp_block); 
    HT_block_info ht_block_info; 

    int ht_block_info_offset = ht_info->max_records*sizeof(Record);
    memcpy(&ht_block_info,temp_data+ht_block_info_offset,sizeof(HT_block_info));

    for (int j = 0; j < ht_block_info.records_number; j++) {    //iterating through records 
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
  
    CALL_OR_DIE(BF_UnpinBlock(temp_block));  
    BF_Block_Destroy(&temp_block); 
     
  }    
  if (found_record == 1) {
    printf("total blocks read to find record: %d\n",total_blocks_read);
    return total_blocks_read;
  }
  
   return -1;
}

int HashStatistics(char *filename) {
  printf("\nHash Statistics :\n"); 
  HT_info* info = HT_OpenFile(filename);

  int blocksnum; 
  CALL_OR_DIE(BF_GetBlockCounter(info->file_desc, &blocksnum));
  printf("Total Blocks in hash file: %d\n", blocksnum); 

  int offset = info->max_records*sizeof(Record);

  int bucket_array[info->num_of_buckets]; 
  int total_overflowing_buckets[info->num_of_buckets]; 

  for (int i = 0; i < info->num_of_buckets; i++) {
    bucket_array[i] = 0; 
    total_overflowing_buckets[i] = 0; 
  }
  int total_records = 0; 

  for (int i = 1; i < blocksnum; i++) {
    BF_Block * temp_block; 
    BF_Block_Init(&temp_block); 
    CALL_OR_DIE(BF_GetBlock(info->file_desc, i, temp_block));

    HT_block_info ht_block_info; 

    char *data = BF_Block_GetData(temp_block); 
    memcpy(&ht_block_info,data+offset,sizeof(HT_block_info)); 

    int bucket = ht_block_info.bucket;
    bucket_array[bucket]++; 
    total_records+=6; 
    if (ht_block_info.overflowing_bucket!=0) {
      total_overflowing_buckets[bucket]++; 
    }
    
    CALL_OR_DIE(BF_UnpinBlock(temp_block));  
    BF_Block_Destroy(&temp_block); 
  }

  int max_records, min_records; 
  max_records = -1;
  min_records = 100000;
  int total_blocks = 0; 
  for (int i = 0; i < info->num_of_buckets; i++) {
    total_blocks +=bucket_array[i]; 
    int total_recs_in_bucket = bucket_array[i] * 6; 
    if (total_recs_in_bucket > max_records) {
      max_records = total_recs_in_bucket; 
    }
    if (total_recs_in_bucket < min_records){
      min_records = total_recs_in_bucket; 
    }
  }

  int total_buckets=0; 
  for (int i = 0; i < info->num_of_buckets; i++){
    if (total_overflowing_buckets[i] != 0) {
      total_buckets++;
    }
  }
 
  printf("Min amount of records in bucket: %d\n", min_records);
  printf("Max amount of records in bucket: %d\n", max_records);
  printf("Average amount of records in bucket: %d\n", total_records/info->num_of_buckets);
  printf("Average amount of blocks in a bucket: %d\n", total_blocks/info->num_of_buckets);
  printf("Total buckets that have an overflowing bucket: %d\n", total_buckets); 

  for (int i = 0; i < info->num_of_buckets;i++) {
    printf("bucket: %d , has total overflowing buckets: %d\n", i , total_overflowing_buckets[i]);
  }

  HT_CloseFile(info);

}



