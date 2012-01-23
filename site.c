#include <stdio.h>
#include <stdlib.h>

#include "site.h"

int availableSites[MAX_SITES] ; //The array stores a value '1' if a given site is avaiable and stores '0' if site is down, where site is indexed by its position in the array
void releaseLocks(int site_No,int tid);

//////////////////////////////////////////////////////////////////////
/*read is Enabled is called if a transaction that wrote on ‘varNo’ has committed.read is Disabled for all variables of a site when a site first recovers from a failure.*/
/* 1 Enable ; 0 for Disable; */
void readEnableDisable(int site_No,int varNo,int enable)
{
	sites[site_No].lock_Entries[varNo].readAvailable = enable; 
}
//////////////////////////////////////////////////////////////////////

/*checkReadAvailability is called for read operations of update transactions.
Returns ‘1’ if data item ‘varNo’ is available for read else it returns ‘0’.*/
int checkReadAvailability(int site_No,int varNo,int tid)
{

struct operation *current = sites[site_No].lock_Entries[varNo].first_active_operation;
if(current!=NULL)
{
	if(current->tid==tid && current->operationType==WRITE_OPERATION)  //If the same transaction already has the write lock
	{
		return 1;  // Return that the item is availaible to be read;
	}
}

return sites[site_No].lock_Entries[varNo].readAvailable;
}
//////////////////////////////////////////////////////////////////////


int readonly_Versiontable(int site_No,int var,int timestamp)
{
		char log_desc[1000];
	        int valueRead=0;
                if(sites[site_No].variable[var].flag==1)
                {
			struct version* current = sites[site_No].variable[var].head;
			int largest=-1;
		    
 		while(current!=NULL)
			 {
				if(current->W_Timestamp<=timestamp && current->W_Timestamp>largest)
				{
					largest=current->W_Timestamp;
					valueRead=current->value;
				}   
				current=current->next;
			 } 

		}
		


return valueRead;
}
//////////////////////////////////////////////////////////////////////

// Forget all the information in lock table
void siteFail(int site_No)
{
int j;
	for(j=1;j<=MAX_VARIABLES;j++)
	{
	    if(sites[site_No].lock_Entries[j].flag==1)  //If the variable is present at that site
		{
			sites[site_No].lock_Entries[j].first_active_operation=NULL;	
			sites[site_No].lock_Entries[j].first_blocked_operation=NULL;
		}
	}

}



//////////////////////////////////////////////////////////////////////
//A new entry will be added if the transaction issues a first write and value of R-Timestamp and 
// W-Timestamp is set to very high. For that further transactions writes this value will be updated.
//If the transaction commits the R-Timestamp and W-Timestamp will be set accordingly.
//If the transaction aborts then those entries will be deleted from the list.
void UpdateVersionTable(int site_No,int varNo,struct operation *op)
{
   int j;
  if(varNo>0)
{ 
   if(sites[site_No].variable[varNo].flag==1)
     {
	    if(op->operationType==WRITE_OPERATION)     //Only if the operation type is write
	       { 	
		// Run through and check if the first write has been done
		 	struct version* current = sites[site_No].variable[varNo].head;
	         	int exist=0;
		     	while(current!=NULL)
			 {
				if((op->tid==current->tid)&&(current->W_Timestamp==MAX_TRANSACTION_TIMESTAMP))
				{
					current->value=op->valueToWrite;
					exist=1;
				}   
				current=current->next;
			 } 

		//If there is no entry for that transaction;added it	
			if(exist==0)
			{
				struct version *newNode = (struct version *) malloc (sizeof(struct version));
				newNode->tid=op->tid;
                       		newNode->value=op->valueToWrite;
 				newNode->R_Timestamp=MAX_TRANSACTION_TIMESTAMP;
				newNode->W_Timestamp=MAX_TRANSACTION_TIMESTAMP;
				newNode->next=sites[site_No].variable[varNo].head;
				sites[site_No].variable[varNo].head=newNode;	
			}
                }

		if(op->operationType==READ_OPERATION)   //Only if the operation is a read operation
		{
			// Run through and check if the first write has been done;then set it for read
		 	struct version* current = sites[site_No].variable[varNo].head;
	         	int exist=0;
		     	while(current!=NULL)
			 {
				if((op->tid==current->tid)&&(current->W_Timestamp==MAX_TRANSACTION_TIMESTAMP))
				{
					op->valueRead=current->value;  //Returning the value Read
					exist=1;
				}   
				current=current->next;
			 }
			 
			 if(exist==0)  //If no write has been done till now,then read the recent committed value
			 {
			    op->valueRead=readonly_Versiontable(site_No,varNo,op->transactionTimestamp);  //Returning the read value; 
			 }
			 

	        }
     }   //If for checking variable present ends.

}
//Only if the operation type is commit;update the R and W Timestamp for that transaction in all variables;and make variable read available
     if(op->operationType==END_OPERATION)  
      { 
		
	   for(j=1;j<=MAX_VARIABLES;j++)
             { 
                if(sites[site_No].variable[j].flag==1)
                {
			struct version* current = sites[site_No].variable[j].head;
		     if(current!=NULL)
		     {
		     	while(current!=NULL)
			 {
				if((op->tid==current->tid)&&(current->W_Timestamp==MAX_TRANSACTION_TIMESTAMP))
				{
					current->W_Timestamp=op->transactionTimestamp;
					current->R_Timestamp=op->transactionTimestamp;    
					if(checkReadAvailability(site_No,j,op->tid)==0)  //If the variable is unavailable make it available
						readEnableDisable(site_No,j,1);   		 
				}   
				current=current->next;
			 }		  		 
		     }	
		}
	    } 
       }
    



}


//////////////////////////////////////////////////////////////////////

//*addToActiveList sets the node as the first pointer

void addToActiveList(int site_No,int varNo,struct operation *node,int request)
{

if(request==0)   // Same transaction is getting added 
{
	sites[site_No].lock_Entries[varNo].first_active_operation = node;
	node->nextOperationSite = NULL;
}
else
{
	//Traverse the list
	struct operation *current = sites[site_No].lock_Entries[varNo].first_active_operation;
	while(current->nextOperationSite != NULL)
		current=current->nextOperationSite;

	current->nextOperationSite=node;
	node->nextOperationSite=NULL;
}

}
///////////////////////////////////////////////////////////////////////

//*addToBlockedList adds node to the end of blocked list

void addToBlockedList(int site_No,int varNo,struct operation *node)
{

if(sites[site_No].lock_Entries[varNo].first_blocked_operation == NULL)  //No blocked operation
{       
	sites[site_No].lock_Entries[varNo].first_blocked_operation=node;
	node->nextOperationSite = NULL;
} 
else
{
//Traverse the list
struct operation *current = sites[site_No].lock_Entries[varNo].first_blocked_operation;
while(current->nextOperationSite != NULL)
	current=current->nextOperationSite;

current->nextOperationSite=node;
node->nextOperationSite=NULL;
}

}
///////////////////////////////////////////////////////////////////////

/*checkLockIsNecessary Checks if a transaction ‘tid’ needs to acquire a lock on a data item or if it already has a required or the stronger lock Returns ‘1’ if transaction has the lock else it returns ‘0’ if it needs to acquire a lock.*/
int checkLockIsNecessary (int site_No,int varNo,int tid,int lock_Mode)
{
		struct operation *first=sites[site_No].lock_Entries[varNo].first_active_operation ;
                //printf("first_active %d\n", sites[site_No].lock_Entries[varNo].first_active_operation->tid) ;
                if(first != NULL ) {
		  if(first->tid!=tid)  //If the transaction tid is not current
		  {
			return 0;
		  }		
		  else
		  {
			if(first->operationType >= lock_Mode)   //If the transaction has the same lock or a higher lock;
			   return 1;
			else
			   return 0; //Request for W Lock,Has R Lock Currently
			     
		  }
               }
               return 0 ;
		
}
//////////////////////////////////////////////////////////////////////


/* checkConflictAndDeadlockPrevention checks if transaction tid’s access on data item ‘var’ conflicts with any other transaction in conflicting mode.Returns 0 of is there is no conflict and therefore lock can be granted; 1 if requesting tid should be blocked ; -1 if requesting tid should be aborted*/

int checkConflictAndDeadlockPrevention(int site_No,int varNo,int tid,int timestamp,int operationType)
{
		char log_desc[1000];
		int found=0;
                struct operation *first=sites[site_No].lock_Entries[varNo].first_active_operation;
	        	
//If no active operations just grant the lock;on conflict
if(first ==NULL)
{        
 	return 0; 		
}
else
{



//If the same transaction is requesting;No Conflict;This will happen if transaction has a read lock on variable but can't get the lock 
	       if(first->nextOperationSite == NULL)
		if(first->tid == tid) //This function is called only when the transaction had a low strength lock;so block the request
		 {      
			return 0;
		 }
		
		if((first->operationType == operationType) && (operationType==READ_OPERATION))// No conflict if lock req is for read; current is read
		 {
			return 2;    /// This indicates that variable is to be appended to the ;; Change this
		 }    	
		 else		//Compare tids for deadlock prevention 
		 {
		        
			while(first!=NULL)
			{
			   if(timestamp > first->transactionTimestamp)   // If Younger one is requesting;Kill it
			     { 
                          sprintf(log_desc,"At Site %d: tid %d timestamp %d rejected in favor of tid %d timestamp %d has lock on varNo %d due to wait-die\n", site_No, tid, timestamp, first->tid, first->transactionTimestamp, first->varNo) ;
			  logString(log_desc) ;
			      found=1;
			      return -1; 
			     }
			if(first->nextOperationSite==NULL)
			   break;
			first=first->nextOperationSite;
			}
			//printf("\n Timestamp of %d;TID:%d Timestamp of %d;TID:%d",timestamp,tid,first->transactionTimestamp,first->tid);

			if(found==0) {                  // If older one is requesting;block it  
                          sprintf(log_desc,"At Site %d: tid %d timestamp %d blocked since tid %d timestamp %d has the lock on varNo %d\n", site_No, tid, timestamp, first->tid, first->transactionTimestamp, first->varNo) ;
			  logString(log_desc);
			  return 1;
                        }


		 }
		
    }	 
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
void doDump(struct operation *op, int site_No)
{
char log_desc[1000];
int j,temp;
if(op->varNo==ALL_VARIABLES)       // All variables at that site
{
 sprintf(log_desc,"Variables at Site:%d\n",site_No);
 logString(log_desc);
 for(j=1;j<MAX_VARIABLES;j++)
  {
    if(sites[site_No].variable[j].flag==1) {
	temp=readonly_Versiontable(site_No,j,op->transactionTimestamp);
	sprintf(log_desc," x%d: %d",j,temp);
	logString(log_desc);
	}
  }
}
else				 
{  
  sprintf(log_desc,"\nSite:%d x%d: %d",site_No,op->varNo,readonly_Versiontable(site_No,op->varNo,op->transactionTimestamp));	
  logString(log_desc);
}
sprintf(log_desc,"\n") ;
logString(log_desc);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void printActiveandBlockedList(struct operation *op, int site_No)
{

int j;
int flagListEmpty = 1 ;
char log_desc[1000];

sprintf(log_desc,"\n********Active List at Site:%d**********",site_No);
logString(log_desc);

for(j=1;j<MAX_VARIABLES;j++)
  {
    if(sites[site_No].variable[j].flag==1) {
		
		struct operation *first=sites[site_No].lock_Entries[j].first_active_operation;
		if(first!=NULL)
		{
                        if(flagListEmpty == 1) {
                           flagListEmpty = 0 ;
                        }
                        sprintf(log_desc,"\nVarNo:%d Transaction ID(s) holding the lock:",j);
			logString(log_desc);
			while(first!=NULL)
			{
                                if(first->operationType == READ_OPERATION) {
				  sprintf(log_desc,"TID %d lock type read; ",first->tid);
                                }
                                else if(first->operationType == WRITE_OPERATION) {
				  sprintf(log_desc,"TID %d lock type write; ",first->tid);
                                }
				logString(log_desc);
				first=first->nextOperationSite;
			}		  
		}
		/*else
		{
		  sprintf(log_desc,"  Empty");
		  logString(log_desc);	
		}*/
	}
  }
if(flagListEmpty == 1) {
  sprintf(log_desc,"\nList is empty\n") ;
  logString(log_desc);
}
flagListEmpty = 1 ;
sprintf(log_desc,"\n\n******Blocked List at Site:%d*********",site_No);
logString(log_desc);

for(j=1;j<MAX_VARIABLES;j++)
  {
    if(sites[site_No].variable[j].flag==1) {
		struct operation *first=sites[site_No].lock_Entries[j].first_blocked_operation;
		if(first!=NULL)
		{
                        if(flagListEmpty == 1) {
                           flagListEmpty = 0 ;
                        }
                        sprintf(log_desc,"\nVarNo:%d Transaction ID(s) waiting the lock:",j);
			logString(log_desc);
			while(first!=NULL)
			{
                                if(first->operationType == READ_OPERATION) {
				  sprintf(log_desc,"TID %d lock type read; ",first->tid);
                                }
                                else if(first->operationType == WRITE_OPERATION) {
				  sprintf(log_desc,"TID %d lock type write; ",first->tid);
                                }
				
				logString(log_desc);
				first=first->nextOperationSite;
			}		  
		}
		/*else
		{
			  sprintf(log_desc,"  Empty");
			  logString(log_desc);
		}*/
	}
  }
if(flagListEmpty == 1) {
  sprintf(log_desc,"\nList is empty\n") ;
  logString(log_desc);
}
sprintf(log_desc,"\n") ;
logString(log_desc);


}
//////////////////////////////////////////////////////////////////////












void performOperation(struct operation *op, int site_No)
{

char log_desc[1000];

if((availableSites[site_No]==0)  && op->operationType != RECOVER_OPERATION ) //if the site is down simply reject the operation
{
	op->operationStatusAtSites[site_No]=OPERATION_REJECTED;
	return;
}

if(op->transactionType == READONLY_TRANSACTION )
{
	      if(op->operationType==READ_OPERATION) 				   //If the operation is read operation	
	      {
		if(checkReadAvailability(site_No,op->varNo,op->tid) == 0) 			   //If the variable is not availaible to be read
		{
			   op->operationStatusAtSites[site_No]=OPERATION_REJECTED;
			   sprintf(log_desc,"Rejecting read for tid %d on var %d @ site %d because site has just recovered\n", op->tid, op->varNo, site_No) ;
			   logString(log_desc);	
			   return;
		}
		else
		{
			   op->valueRead=readonly_Versiontable(site_No,op->varNo,op->transactionTimestamp);		
			   op->operationStatusAtSites[site_No]=OPERATION_COMPLETE;
			   return;
		}
	      }
}



if(op->transactionType == READWRITE_TRANSACTION )
{

	      if(op->operationType==READ_OPERATION) 				   //If the operation is read operation	
	      	{
		 if(checkReadAvailability(site_No,op->varNo,op->tid) == 0) 		   //If the variable is not availaible to be read
			{			
			   op->operationStatusAtSites[site_No]=OPERATION_REJECTED;
                           sprintf(log_desc,"Rejecting read for tid %d on var %d @ site %d because site has just recovered\n", op->tid, op->varNo, site_No) ;
			   logString(log_desc);	
          		   return;
			}	
		//if the variable is available
		}
	
	     		
		if(op->operationType==READ_OPERATION || op->operationType== WRITE_OPERATION)
		{
		if(checkLockIsNecessary(site_No,op->varNo,op->tid,op->operationType) == 1)  // If the transaction already has the stronger lock
			{  UpdateVersionTable(site_No,op->varNo,op);
			   op->operationStatusAtSites[site_No]=OPERATION_COMPLETE;  // Lock Granted;
			   return;
			}   
		
               	//Transaction needs to acquire the lock
		
		int request=checkConflictAndDeadlockPrevention(site_No,op->varNo,op->tid,op->transactionTimestamp,op->operationType);
		
		if((request==0) || (request==2)) //Lock Granted,add it to active list
		  {	
			addToActiveList(site_No,op->varNo,op,request);
			UpdateVersionTable(site_No,op->varNo,op);
			op->operationStatusAtSites[site_No]=OPERATION_COMPLETE;
			return;
		  }
		else if(request==1)
		  { 	
			addToBlockedList(site_No,op->varNo,op);
			op->operationStatusAtSites[site_No]=OPERATION_BLOCKED;
			return;
		  }
		else    //If the transaction needs to be aborted
		  {
			op->operationStatusAtSites[site_No]=OPERATION_REJECTED;
			return;
		  }		
		}
	if(op->operationType==END_OPERATION)
	{	//printf("\nvarNo sent by op:%d",op->varNo);
		UpdateVersionTable(site_No,-1,op);        // Update R and W Timestamps; and make read available
		releaseLocks(site_No,op->tid);		  // Release all locks for transaction 'tid'.
		op->operationStatusAtSites[site_No]=OPERATION_COMPLETE;
		return;
	}

	if(op->operationType == ABORT_OPERATION)
	{
		releaseLocks(site_No,op->tid);
		op->operationStatusAtSites[site_No]=OPERATION_COMPLETE;
		return;
	}
	
}		


if(op->transactionType == MISCELLANEOUS_TRANSACTION)
{
	if(op->operationType==FAIL_OPERATION)		
	{       
  		sprintf(log_desc,"Site Failed: %d\n",site_No);
		logString(log_desc);	
		availableSites[site_No]=0;		//Making the Site Down
		siteFail(site_No);                      // Forget the lock information
		op->operationStatusAtSites[site_No]=OPERATION_COMPLETE;
		return;
	}

	if(op->operationType==RECOVER_OPERATION)		
	{
		int j;
		availableSites[site_No]=1;		//Making the Site Up
		sprintf(log_desc,"Site Recovered: %d\n",site_No);
		logString(log_desc);	
		for(j=1;j<MAX_VARIABLES;j++)	{	//Make all the variables at that site as read disable
                  //printf("site %d flag %d %d\n", site_No, j, sites[site_No].variable[j].flag);
                  if(sites[site_No].variable[j].flag==1) {
		     if(j%2==0)
		      {		
                	readEnableDisable(site_No,j,0);
		      }
		      else
		      {
			readEnableDisable(site_No,j,1);	
		      }					
                  }
                }
		op->operationStatusAtSites[site_No]=OPERATION_COMPLETE;
		return;
	}	
	if(op->operationType == DUMP_OPERATION)
	{
		doDump(op,site_No);	
		op->operationStatusAtSites[site_No]=OPERATION_COMPLETE;
		return;	
	}
	if(op->operationType == QUERY_STATE_OPERATION)
	{
		doDump(op,site_No);
		printActiveandBlockedList(op,site_No);
		op->operationStatusAtSites[site_No]=OPERATION_COMPLETE;
		return;
	}
					
	
}




}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


/* Release Locks releases all the locks for the transaction */
void releaseLocks(int site_No,int tid)
{
int j;
	for(j=1;j<=MAX_VARIABLES;j++)
	{
		if(sites[site_No].lock_Entries[j].flag==1)  //If the variable is present at that site
		{
			

			//Scan the Active List
	     			struct operation *node=sites[site_No].lock_Entries[j].first_active_operation;
				if(node!=NULL)
				{
				if(node->tid==tid)
				 {
				  sites[site_No].lock_Entries[j].first_active_operation=NULL;    //Remove the active lock
				 // printf("Release Locks:%d",tid);
				  if(sites[site_No].lock_Entries[j].first_blocked_operation!=NULL) 
				   {	
					sites[site_No].lock_Entries[j].first_active_operation=sites[site_No].lock_Entries[j].first_blocked_operation;
					//printf("Putting in active %d",sites[site_No].lock_Entries[j].first_blocked_operation->tid);
					sites[site_No].lock_Entries[j].first_blocked_operation=sites[site_No].lock_Entries[j].first_blocked_operation->nextOperationSite;
					performOperation(sites[site_No].lock_Entries[j].first_active_operation, site_No);   //Perform the operation 
				   }
				 }
				}
		
			//Scan the Blocked List
	     			struct operation *current=sites[site_No].lock_Entries[j].first_blocked_operation;
				if(current!=NULL)
				{
				  if(current->nextOperationSite==NULL)  // If only one element
			 	     if(current->tid==tid)
					   sites[site_No].lock_Entries[j].first_blocked_operation=NULL;
				  else             //More than one element
				  {
				     	struct operation *previous=current;
					while(current!=NULL)
					{
						if(current->tid==tid)
						 {   //printf("Inside Realease Blocked : %d %d",current->tid,j);
						     current=current->nextOperationSite;	
						     previous->nextOperationSite=current;
						     previous=current;
						 } 
						 else
					 	 {
						     previous=current;
						     current=current->nextOperationSite;
						 }  
					} 

				  }	           													   					
			       }
				   
			
		}		

	}

}


//////////////////////////////////////////////////////////////////////





/////////////////// Initializing Sites	///////////////////////

///Initializing variables 
void initializeSiteData()
{
int i,j,k;
for(i=1;i<=(MAX_SITES-1);i++)
{  
	  for(j=1;j<=(MAX_VARIABLES-1);j++)
             { 
                  if(j%2==0)
                      { 
			// For Version Table, The first one is dummy insertion
			struct version *newNode = (struct version *) malloc (sizeof(struct version));
                        sites[i].variable[j].flag=1;
			newNode->tid=-1;
                        newNode->value=10*j;
 			newNode->R_Timestamp=0;
			newNode->W_Timestamp=0;
			newNode->next=NULL;
			sites[i].variable[j].head=newNode;
			// For Lock Table
			sites[i].lock_Entries[j].flag=1;
			sites[i].lock_Entries[j].readAvailable=1;// Initially the variable needs to be made available for reading 
					

		      }
                  else
		      {
			if(i==((j%10)+1))
			{
			// For Version Table, The first one is dummy insertion
			struct version *newNode = (struct version *) malloc (sizeof(struct version));
                        sites[i].variable[j].flag=1;
			newNode->tid=-1;
                        newNode->value=10*j;
 			newNode->R_Timestamp=0;
			newNode->W_Timestamp=0;
			newNode->next=NULL;
			sites[i].variable[j].head=newNode;
			// For Lock Table
			sites[i].lock_Entries[j].flag=1;
			sites[i].lock_Entries[j].readAvailable=1;// Initially the variable needs to be made available for reading 
			}
		      }			

             }
    

}

//Rahul Pandey
int siteNo ;
for(siteNo = 1; siteNo < MAX_SITES; siteNo++) {
  availableSites[siteNo] = 1 ;  

}
}



/*
int main()
{

 
initializeSiteData();

int i,j,k;


/// Manually insertion dummy values
for(i=1;i<=MAX_SITES;i++)
{     
	  for(j=1;j<=MAX_VARIABLES;j++)
             { 
                if(sites[i].variable[j].flag==1)  //If that variable is at that site.
                {   
                       for(k=1;k<=5;k++)
		     {
			struct version *newNode = (struct version *) malloc (sizeof(struct version));
			newNode->tid=1;
                        newNode->value=10*10*j+k;
 			newNode->R_Timestamp=k;
			newNode->W_Timestamp=k;
			newNode->next=sites[i].variable[j].head;
			sites[i].variable[j].head=newNode;	
                     }
                }
	     }
}

//// 



struct operation op1,op2,op3,op4,op5,op6,op7,op8;


	op1.tid=6;
	op1.operationTimestamp=10;
  	op1.operationType=1;
  	op1.valueToWrite=898;
  	
	op2.tid=6;
	op2.operationTimestamp=10;
  	op2.operationType=1;
  	op2.valueToWrite=9000;

	op3.tid=7;
	op3.operationTimestamp=11;
  	op3.operationType=1;
  	op3.valueToWrite=888;

	op4.tid=7;
	op4.operationTimestamp=11;
  	op4.operationType=1;
  	op4.valueToWrite=599;

	op5.tid=7;
	op5.operationTimestamp=11;
  	op5.operationType=1;
  	op5.valueToWrite=88888;

	op6.tid=6;
	op6.operationTimestamp=10;
  	op6.operationType=2;
  		
	op7.tid=7;
	op7.operationTimestamp=11;
  	op7.operationType=2;

	op8.tid=5;
	op8.operationTimestamp=9;
  	op8.operationType=1;	
	op8.valueToWrite=8898;


UpdateVersionTable(10,2,&op1);
UpdateVersionTable(10,2,&op2);
UpdateVersionTable(10,3,&op3);
UpdateVersionTable(10,4,&op4);
UpdateVersionTable(10,4,&op5);
UpdateVersionTable(10,-1,&op6);
UpdateVersionTable(10,-1,&op7);
UpdateVersionTable(10,2,&op8);

printf("\n Value Read:%d",readonly_Versiontable(10,4,12));
*/
/*

for(i=1;i<=(MAX_SITES-1);i++)
{     
printf("\n Site No %d",i-1);
	  for(j=1;j<=(MAX_VARIABLES-1);j++)
             { 
                if(sites[i].variable[j].flag==1)
                {
		printf("\n Variable No : %d",j);
                
		struct version* current = sites[i].variable[j].head;
		while (current != NULL) {
                printf("\t Value: %d",current->value);
		printf("\t Value: %d",current->tid);
		printf("\t Value: %d",current->R_Timestamp);
		printf("\t Value: %d",current->W_Timestamp);
		printf("\n");
		current=current->next;
		}

                }
	     }
}




return 0;
}*/
