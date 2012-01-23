#ifndef _GLOBALS_
#define _GLOBALS_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define READONLY_TRANSACTION    		0
#define READWRITE_TRANSACTION   		1
#define MISCELLANEOUS_TRANSACTION		2	//We create a pseudo transaction type called miscellaneous transction. Such transaction will contain operations such as recover, fail, dump 

#define MAX_SITES		11	//Sites are numbered 1 to 10
#define MAX_VARIABLES		21	//Variables are numbered from 1 to 20

#define ALL_VARIABLES		100	//A value indicating that operation is to be performed on all variables. Operation probably is a dump operation
#define ALL_SITES		100	//A value indicating that operation is to be performed on all sites. Operation probably is a dump operation

#define READ_OPERATION		0
#define WRITE_OPERATION		1
#define END_OPERATION		2
#define ABORT_OPERATION		-2
#define DUMP_OPERATION		3
#define FAIL_OPERATION		-4
#define RECOVER_OPERATION	4
#define QUERY_STATE_OPERATION	5

#define OPERATION_PENDING	0
#define OPERATION_BLOCKED	-1
#define OPERATION_REJECTED	-2
#define OPERATION_IGNORE	-3	//Indicates operation at specific is to be ignored since the site has failed
#define OPERATION_COMPLETE	1

struct operation {
 int tid ;				//Transaction ID to which this operation belongs
 int operationType ;			//The operation could be a read/write/commit/abort/dump/recover/fail operation
 int operationTimestamp ;
 int transactionType ;			//Type of the transaction to which the operation belongs
 int transactionTimestamp ;
 int varNo ;
 int valueToWrite ;
 int valueRead ;
 int siteNo ;					//For operations such as dump/recover/fail, site on which operation will be applied
 int operationWaitListed_tickNo ;	        //Value of tick at which operation was sent to a given site, this will be useful in timing out operations
 int operationStatusAtSites[MAX_SITES] ; 	//Every site will set its status for the operation at the appropriate array index
 struct operation *nextOperationSite ;
 struct operation *nextOperationTM ;
} ;

void logString(char *) ;
#endif
