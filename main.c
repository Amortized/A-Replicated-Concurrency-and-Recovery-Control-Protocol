#include "globals.h"
#include "transaction_manager_structures.h"
#include "site.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  char inputSequenceFile[100], log_desc[1000] ;
  int ret ;

  if(argc != 2) {
    printf("Usage: %s <input-transaction-file-path>\n", argv[0]) ;
    return  0 ;
  }
  strcpy(inputSequenceFile, argv[1]) ;
  ret = checkFileExists(inputSequenceFile) ;
  if(ret == -1) {
   printf("main: File %s does not exist or is an empty file. Exiting\n", inputSequenceFile) ;
   return  0 ;
  }
  FILE *fp = fopen("repcrec.log", "w") ;
  if(fp == NULL) {
    printf("main: failed to open log file in write mode. Error: %s\n", (char *)strerror(errno)) ;
  }
  else {
    fclose(fp) ;
  }
  initializeTransactionManager() ;
  initializeSiteData() ;
  ret = parseInput(inputSequenceFile) ;
  if(ret == -1) {
   printf("main: parseInput could not parse file %s. Exiting\n", inputSequenceFile) ;
   return  0 ;
  }
  startTransactionManager() ;
  sprintf(log_desc, "main: Transaction Manager exiting\n") ;
  logString(log_desc) ;
  return  0 ;
}

//This function checks if the given exists or not and returns err if the file is empty
int checkFileExists(char *fname) {
  int ret;
  struct  stat _curr ;
  ret = lstat(fname, &_curr) ;

  if (ret == -1) {
   printf("checkFileExists: lstat error for file %s: %s\n", fname, (char *)strerror(errno)) ;
   return -1 ;
  }
  if(_curr.st_size == 0){
   printf("checkFileExists: empty file %s\n", fname);
   return -1 ;
  }
  return 0 ;
}

void logString(char * log_desc) {

   FILE *fp = fopen("repcrec.log", "a") ;
   printf("%s", log_desc) ;
   if(fp == NULL) {
     printf("logString: failed to open log file in append mode. Error: %s\n", (char *)strerror(errno)) ;
     return ;
   }
   fprintf(fp, "%s", log_desc) ;
   fclose(fp) ;
}
