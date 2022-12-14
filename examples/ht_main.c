#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "ht_table.h"

#define RECORDS_NUM 200 // you can change it if you want
#define FILE_NAME "data.db"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

// #define CALL_BF(call)       \
// {                           \
//   BF_ErrorCode code = call; \
//   if (code != BF_OK) {         \
//     BF_PrintError(code);    \
//     return HT_ERROR;        \
//   }        


int main() {
  BF_Init(LRU);

  HT_CreateFile(FILE_NAME,10);
  HT_info* info = HT_OpenFile(FILE_NAME);

   for (int i = 0; i < info->num_of_buckets; i++) {
    printf("bucket %d , %d\n", i , info->hash_table[i]); 
  }

  int blocksnum; 
  CALL_OR_DIE(BF_GetBlockCounter(info->file_desc,&blocksnum)); 
  printf("blocks num %d\n", blocksnum); 

  Record record;
  srand(12569874);
  int r;
  printf("Insert Entries\n");
  for (int id = 0; id < RECORDS_NUM; ++id) {
    record = randomRecord();
    HT_InsertEntry(info, record);
  }

  printf("RUN PrintAllEntries\n");
  int id = rand() % RECORDS_NUM;
  HT_GetAllEntries(info, &id);

  HT_CloseFile(info);
  BF_Close();
}
