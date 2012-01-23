#define MAX_TRANSACTIONS	10000
#define MISCELLANEOUS_TID       9999             //All dump/fail/recover operations will be assigned to this tid
#define MAX_WAITING_TICKS	20		//Max number of ticks before which the operation for which an operation can be blocked

#include "globals.h"
#include <time.h>

#define TRANSACTION_COMMITED	1
#define TRANSACTION_ABORTED	0

struct siteAccessInfo {		//This struct gives access information for a site for a specific transaction
 int tick_firstAccessed ;
 int flagWriteAccessed ;
} ;

struct siteInformation {
  int flag_varNo[MAX_SITES] ;	//A flag indicating whether the variable is present at the given site or not
  int flag_siteAvailable ;	//A flag indicating if the given site is available or down
  int tick_upTime ;		//The value of tick since which the site is up
} ;
typedef struct siteInformation siteInformation ;
siteInformation siteInfo[MAX_SITES] ;

struct Transaction {
 struct siteAccessInfo sites_accessed[MAX_SITES];
 int timestamp ;
 int flag_transactionValid ;
 int transactionType ;
 int transactionCompletionStatus ;
 int inactiveTickNo ;
 struct operation *first_opn ;
 struct operation *last_opn ;
 struct operation *current_opn ;
} ;

struct Transaction T[MAX_TRANSACTIONS] ;
void initializeTransactionManager() ;
void startTransactionManager() ;
