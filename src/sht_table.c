#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "sht_table.h"
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


int hash_string(char * name, int buckets) 
{
  if (name[0] != '\0') {
    return name[0] % buckets + hash_string(name+1,buckets);
  } else {
    return 0; 
  }
}


int SHT_CreateSecondaryIndex(char *sfileName,  int buckets, char* fileName){
    int file_desc, blocks_num;
    BF_CreateFile(sfileName);  
    CALL_OR_DIE(BF_OpenFile(sfileName,&file_desc)); 
    
    int HT_file_desc; 
    CALL_OR_DIE(BF_OpenFile(fileName,&HT_file_desc));

    BF_Block * block;                 //Allocating block 0 of index file  
    BF_Block_Init(&block); 
    BF_AllocateBlock(file_desc,block); 

    SHT_info sht_info;                //creating metadata of index file as sht_info 
    sht_info.file_type = "secondary index file";            
    sht_info.file_desc = file_desc;
    sht_info.max_pairs = BF_BLOCK_SIZE/sizeof(Pair) - 1;   
    sht_info.blocks_number=1; 
    sht_info.last_block_id = 0;
    sht_info.num_of_buckets = buckets;
    
    for (int i = 0; i < buckets; i++) {
      sht_info.Hash_table[i] = i + 1; 
    }

    int HT_blocks; 
    CALL_OR_DIE(BF_GetBlockCounter(HT_file_desc,&HT_blocks))      
    sht_info.HT_num_of_blocks = HT_blocks - 1;      //-1 because block 0 is for ht info

    
    void* data = BF_Block_GetData(block);

    memcpy(data,&sht_info,sizeof(SHT_info));  //inserting metadata of file in block 0 (SHT INFO)

    BF_Block_SetDirty(block); 
    CALL_OR_DIE(BF_UnpinBlock(block)); 
    BF_Block_Destroy(&block); 
    
    CALL_OR_DIE(BF_CloseFile(file_desc)); 
    CALL_OR_DIE(BF_CloseFile(HT_file_desc));


}

SHT_info* SHT_OpenSecondaryIndex(char *indexName){
  int file_desc; 
  CALL_OR_DIE(BF_OpenFile(indexName, &file_desc));

  BF_Block *block;
  BF_Block_Init(&block); 

  CALL_OR_DIE(BF_GetBlock(file_desc, 0, block));      //Getting block 0 of index file which contains metadata
  char* data;
  data = BF_Block_GetData(block); 
  static SHT_info sht_info; 
  memcpy(&sht_info,data,sizeof(SHT_info));     //Taking index file's metadata (SHT INFO )

  
  int offset = BF_BLOCK_SIZE - sizeof(Pair);   //offset of SHT block info. 

  for (int i = 0; i < sht_info.num_of_buckets; i++) {       //creating blocks of index file, as many as blocks of hash file. 
    BF_Block *temp_block;
    BF_Block_Init(&temp_block);
    CALL_OR_DIE(BF_AllocateBlock(file_desc,temp_block));  //allocating as many blocks as the number of buckets 

    sht_info.last_block_id++; 
    sht_info.blocks_number++; 
    void* data = BF_Block_GetData(temp_block);

    SHT_block_info sht_block_info;            //initializing metadata of block 
    sht_block_info.pairs_number=0;
    sht_block_info.block_id = i + 1; 
    sht_block_info.bucket = i; 
  
    memcpy(data+offset,&sht_block_info,sizeof(SHT_block_info)); 

    BF_Block_SetDirty(temp_block); 
    CALL_OR_DIE(BF_UnpinBlock(temp_block)); 
    BF_Block_Destroy(&temp_block); 

  }


  BF_Block_SetDirty(block); 
  CALL_OR_DIE(BF_UnpinBlock(block)); 
  BF_Block_Destroy(&block); 

  
  if (sht_info.file_type == "secondary index file") {
    printf("Index file is good.\n"); 
    return &sht_info;  
  }

    return NULL;
}



int SHT_CloseSecondaryIndex( SHT_info* SHT_info ){
  CALL_OR_DIE(BF_CloseFile(SHT_info->file_desc));

  return 0;
}

int SHT_SecondaryInsertEntry(SHT_info* sht_info, Record record, int block_id){

  int bucket = hash_string(record.name,sht_info->num_of_buckets) % sht_info->num_of_buckets; 

  int block_number = sht_info->Hash_table[bucket];    //getting the correct block to insert from hash function, from hash table array 

  Pair pair;                   
  pair.name = strdup(record.name);         
  pair.block_id = block_id; 

  BF_Block *block;
  BF_Block_Init(&block); 
  CALL_OR_DIE(BF_GetBlock(sht_info->file_desc,block_number,block));   //getting the correct block that the record was inserted in hash file

  SHT_block_info sht_block_info; 
  char * data = BF_Block_GetData(block); 
  int sht_block_info_offset = BF_BLOCK_SIZE - sizeof(Pair);
  memcpy(&sht_block_info,data+sht_block_info_offset,sizeof(SHT_block_info));  //getting blocks metadata (SHT BLOCK INFO) 

  int total_pairs_space = sht_block_info.pairs_number * sizeof(Pair); 
  
  int free_space = BF_BLOCK_SIZE - sizeof(SHT_block_info) - total_pairs_space; 

  if (free_space >= sizeof(Pair)) {               //if new pair can fit inside block 
    int total_pairs = sht_block_info.pairs_number;
    int offset = total_pairs * sizeof(Pair);
    memcpy(data+offset, &pair, sizeof(Pair));       //inserting pair in the correct place, which we get from offset 
    sht_block_info.pairs_number++; 
    memcpy(data+sht_block_info_offset,&sht_block_info,sizeof(SHT_block_info));       //updating blocks metadata
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));
    BF_Block_Destroy(&block); 
  }

  BF_Block *block0;                                     //updating block 0 of index file (sht info) 
  BF_Block_Init(&block0); 

  CALL_OR_DIE(BF_GetBlock(sht_info->file_desc, 0, block0)); 

  char* data0;
  data0 = BF_Block_GetData(block0); 
  static SHT_info new_sht_info; 
  memcpy(&new_sht_info,data0,sizeof(SHT_info)); 
  new_sht_info.blocks_number = sht_info->blocks_number;
  new_sht_info.last_block_id = sht_info->last_block_id;
  new_sht_info.Hash_table[bucket] = sht_info->last_block_id; 

  BF_Block_SetDirty(block0); 
  CALL_OR_DIE(BF_UnpinBlock(block0));  
  BF_Block_Destroy(&block0); 
  

  return block_number; 
}

int SHT_SecondaryGetAllEntries(HT_info* ht_info, SHT_info* sht_info, char* name){
  for (int i = 0; i < ht_info->blocks_number; i++) {              //iterating every block 
    int return_offset;
    return_offset = SHT_search(i , ht_info , name);                 //SHT_search searches for a record with record.name==name and returns its offset inside the block.
    if (return_offset != -1){                           //if record with record.name == name was found 
      BF_Block * temp_block; 
      BF_Block_Init(&temp_block); 
      CALL_OR_DIE(BF_GetBlock(ht_info->file_desc, i, temp_block));       //get block which contains record with record.name == name 
      char* temp_data;
      temp_data = BF_Block_GetData(temp_block);  
      Record temp_record; 
      memcpy(&temp_record,temp_data+return_offset,sizeof(Record));        //get the record
      for (int j = 1; j < sht_info->blocks_number; j++) {             //iterating through every block of index file to find a pair with pair.name == name
        BF_Block * index_block; 
        BF_Block_Init(&index_block); 
        CALL_OR_DIE(BF_GetBlock(sht_info->file_desc, j, index_block)); 
        
        int total_blocks_read = 0; 
        int found_name = 0; 
        char* index_data;
        index_data = BF_Block_GetData(index_block);   
        SHT_block_info sht_block_info; 

        int offset = BF_BLOCK_SIZE - sizeof(Pair); 

        memcpy(&sht_block_info,index_data+offset,sizeof(SHT_block_info));    

        for (int z = 0; z < sht_block_info.pairs_number; z++) {    //iterating through pairs inside block  
          int pair_offset = z * sizeof(Pair); 
          Pair temp_pair; 
          memcpy(&temp_pair,index_data+pair_offset,sizeof(Pair)); 
          if (strcmp(temp_pair.name , name) == 0) {               // if a pair with pair.name == name is found then print it. 
            found_name = 1; 
            printf("Pair: (%d,%s)\n", temp_pair.block_id, temp_pair.name); 
            printf("%s, id: %d, name: %s, surname: %s, city: %s \n", temp_record.record, temp_record.id, temp_record.name,
                                                                  temp_record.surname, temp_record.city); 
            printf("Total blocks read to find name in hash file: %d\n", i);                                    
            printf("Total blocks read to find name in index: %d\n", j);

          }
        }


        CALL_OR_DIE(BF_UnpinBlock(index_block));  
        BF_Block_Destroy(&index_block); 

      }

      CALL_OR_DIE(BF_UnpinBlock(temp_block));  
      BF_Block_Destroy(&temp_block); 
    }
    else {
    }
  }


}


int SHT_search(int block_id, HT_info * ht_info, char * name){      //function to search record names in a specific given block (block_id).
  int return_offset = -1;
  Record final_record;  
  int found_record = 0; 
  BF_Block * temp_block; 
  BF_Block_Init(&temp_block); 

  CALL_OR_DIE(BF_GetBlock(ht_info->file_desc, block_id, temp_block)); 

  char* temp_data;
  temp_data = BF_Block_GetData(temp_block); 
  HT_block_info ht_block_info; 

  int ht_block_info_offset = ht_info->max_records*sizeof(Record);
  memcpy(&ht_block_info,temp_data+ht_block_info_offset,sizeof(HT_block_info));

  for (int j = 0; j < ht_block_info.records_number; j++) {    //iterating through records of block 
      int offset = j * sizeof(Record); 
      Record temp_record; 
      memcpy(&temp_record,temp_data+offset,sizeof(Record)); 
      if (strcmp(temp_record.name,name) == 0) {                  //if record with the given name is found return the offset to get this record. Else returns -1
        found_record = 1; 
        final_record = temp_record; 
        return_offset = offset;                                    
      }
    }
  
    CALL_OR_DIE(BF_UnpinBlock(temp_block));  
    BF_Block_Destroy(&temp_block); 

    
    return return_offset; 
}



