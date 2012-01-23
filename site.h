 #include "globals.h"
#define MAX_TRANSACTION_TIMESTAMP 10000
void initializeSiteData() ;
void performOperation(struct operation *opn, int siteNo) ;
struct lockTable
{
int flag;  /// To check if the variable exist at that site
int readAvailable;
struct operation *first_active_operation;
struct operation *first_blocked_operation;
};

/*struct operation
{
  int tid;
  int tid_timestamp;  //Trasaction's timestamp to which operation belongs to;
  int timestamp;
  int operationType;  //O Read;1 Write;2 Commit;3 Abort
  int valueToWrite;
  int valueToRead;
  struct operation *next;
};
*/



struct version
{
 int tid;    //This will be used when transaction updates the version table during commit;
 int value;
 int R_Timestamp;
 int W_Timestamp;
 struct version *next;
};

struct versionTable
{
 int flag;  /// To check if the variable exist at that site
 //struct version v;
// struct version *oldest;
 struct version *head;
};

struct site
{
struct versionTable variable[MAX_VARIABLES];
struct lockTable lock_Entries[MAX_VARIABLES];
}sites[MAX_SITES];








