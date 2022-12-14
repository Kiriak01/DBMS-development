#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hp_file.h"

#define RECORDS_NUM 1000 // you can change it if you want
#define FILE_NAME "data.db"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }


int main() {
  BF_Init(LRU);

  HP_CreateFile(FILE_NAME);
  HP_info* info = HP_OpenFile(FILE_NAME);


  Record record;
  // srand(12569874);
  time_t t;
  srand((unsigned) time(&t));    //changed srand with time because it gave the same value over and over. Sometime no id exists for getAllEntries 

  int r;
  printf("Insert Entries\n");
  printf("before: last block id %d\n",info->last_block_id);
  for (int id = 0; id < RECORDS_NUM; ++id) {
    record = randomRecord();
    HP_InsertEntry(info, record);
    // printf("meta thn insert entry to arxeio exei blocks: %d " , info->blocks_number);
  }
printf("after: last block id %d\n",info->last_block_id);

  printf("RUN PrintAllEntries\n");
  int id = rand() % RECORDS_NUM;
  printf("\nSearching for: %d",id);
  HP_GetAllEntries(info, id);


  HP_CloseFile(info);
  BF_Close();
}
