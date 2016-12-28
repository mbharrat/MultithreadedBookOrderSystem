/*
 *
 * bookorder.c
 * Varun Sharma & Michael Bharrat
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include <pthread.h>
#include <semaphore.h>
#include "tokenSearch.c" 

#define TotalQueue 100
 
typedef struct clNode {

	char* category;
	int isRecieving; //used by consumer
	int isDone;
	pthread_mutex_t mutex; 
	pthread_cond_t  dataAvailable;
	pthread_cond_t  spaceAvailable;
	int front; //front of queue
	int count; //how many spots are filled
	int bufSize; //max size
	char* buf[TotalQueue]; //queue
	//sem_t	empty_count;
	//sem_t	full_count;
	//sem_t	use_queue;
	struct clNode * next;

} catNode;

catNode* catFront;

typedef struct smNode { //Small Linked list for each book order
	
	char* bookTitle;
	double price;
	double remaining;
	struct smNode * next;

} node_s; 

typedef struct node { //Linked list for each customer

	char* name;
	int userID;
	double creditLimit;
	double origLimit;
	char* address;
	char* state;
	char* zipCode;
	struct smNode* successful;
	struct smNode* rejected;
	int isNotFree;
	pthread_mutex_t mutex;
	pthread_cond_t isAvailable;
	struct node* next;

} node_C; 

node_C* customerFront; //front is a global variable

int mcounter;//counter for print

int setupCat(char* fPath) {

	char  buff[1024];
    FILE* trace_file;
    TokenizerT* tokenizer;
    int len;
    catNode* temp;
    catNode* prevNode;

    trace_file = fopen(fPath, "r");

    if (trace_file != NULL) {

        while (!feof(trace_file)) {

            memset(buff, '\0', sizeof(buff));
            fgets(buff, 1024, (FILE*)trace_file);
	
			tokenizer = TKCreate("|\"", buff); // "\\ " as separators
	
			if(tokenizer == NULL) {
				//printf("Error: unable to create tokenizer\n");
				return 1;
			}
	
			char* token = NULL;
			
			while((token = TKGetNextToken(tokenizer)) != NULL) {
				len = strlen(token); 
				if(strcmp(token, "\n") == 0 ) {
					free(token);
					continue;
				}
				else if (strcmp(token + len - 1, "\n") == 0) {
					token[strlen(token)-1] = '\0'; 
				}

				temp = malloc(sizeof(catNode));
				temp->category = malloc(sizeof(token) + 1);
				memcpy(temp->category, token, strlen(token));
				temp->category[strlen(token)] = '\0';
				temp->next = NULL;
				temp->isRecieving = 1;
				temp->front = 0;
				temp->count = 0;
				temp->bufSize = TotalQueue;
				temp->isDone = 0;
				//sem_open( &temp->empty_count, 0, TotalQueue);
				//sem_open( &temp->full_count, 0, 0 );
				//sem_open( &temp->use_queue, 0, 1 );

				pthread_mutex_init( &temp->mutex, 0 );
				pthread_cond_init( &temp->dataAvailable, 0 );
				pthread_cond_init( &temp->spaceAvailable, 0 );

				if (catFront == NULL) {
					catFront = temp;
					prevNode = catFront;
				}
				else {
					prevNode->next = temp;
					prevNode = prevNode->next;
				}

				free(token);
			}
	
			TKDestroy(tokenizer);
           
        }

        fclose(trace_file);

   	}
   	else {
   		printf("\nFile does not exit or the input does not include the extension\nEx. sample.txt\n\n");
   		return 1;
   	}

   	return 0;

}

int setUpCus(char* fPath) {
	/*
		This method initializes the list of customers
	*/

	char  buff[1024];
    FILE* trace_file;
    TokenizerT* tokenizer;
    int len;
    int count;
    node_C* temp;
    node_C* prevNode;

    count = 0;
    mcounter = 0;

    trace_file = fopen(fPath, "r");

    if (trace_file != NULL) {

        while (!feof(trace_file)) {

            memset(buff, '\0', sizeof(buff));
            fgets(buff, 1024, (FILE*)trace_file);
	
			tokenizer = TKCreate("|\"", buff); // "\\ " as separators
	
			if(tokenizer == NULL) {
				//printf("Error: unable to create tokenizer\n");
				return 1;
			}
	
			char* token = NULL;
			
			while((token = TKGetNextToken(tokenizer)) != NULL) {
				len = strlen(token); 
				if(strcmp(token, "\n") == 0 ) {
					free(token);
					continue;
				}
				else if (strcmp(token + len - 1, "\n") == 0) {
					token[strlen(token)-1] = '\0'; 
				}

				if (count == 0) {
					if(isalpha(token[1] == 0)) {
						return 1;
					}
					temp = malloc(sizeof(node_C));
					temp->name = malloc(sizeof(token) + 1);
					memcpy(temp->name, token, strlen(token));
					temp->name[strlen(token)] = '\0';
					count = count + 1;
					mcounter = mcounter + 1;
				}
				else if (count == 1) {
					temp->userID = atoi(token);
					count = count + 1;
				}
				else if (count == 2) {
					if(isdigit(token[1] == 0)) {
						return 1;
					}
					temp->creditLimit = atof(token);
					temp->origLimit = atof(token);
					count = count + 1;
				}
				else if (count == 3) {
					temp->address = malloc(strlen(token) + 1);
					memcpy(temp->address, token, strlen(token));
					temp->address[strlen(token)] = '\0';
					count = count + 1;
				}
				else if (count == 4) {
					if(isalpha(token[1] == 0)) {
						return 1;
					}
					temp->state = malloc(sizeof(token) + 1);
					memcpy(temp->state, token, strlen(token));
					temp->state[strlen(token)] = '\0';
					count = count + 1;
				}
				else {
					if(isalpha(token[1])) {
						return 1;
					}
					temp->zipCode = malloc(sizeof(token) + 1);
					memcpy(temp->zipCode, token, strlen(token));
					temp->zipCode[strlen(token)] = '\0';
					temp->next = NULL;
					temp->rejected = NULL;
					temp->successful = NULL;
					temp->isNotFree = 0;
			
					pthread_mutex_init( &temp->mutex, 0 );
					pthread_cond_init( &temp->isAvailable, 0 );
					count = 0;

					if (customerFront == NULL) {
						customerFront = temp;
						prevNode = customerFront;
					} 
					else {
						prevNode->next = temp;
						prevNode = prevNode->next;
					}
				}

				free(token);
			}
	
			TKDestroy(tokenizer);
           
        }

        fclose(trace_file);

   	}
   	else {
   		printf("\nFile does not exit or the input does not include the extension\nEx. sample.txt\n\n");
   		return 1;
   	}

   	return 0;

}

void* producer(void* arg) {
//void producer(void* arg) {

	char* fPath;
	char  buff[1024];
    FILE* trace_file;
    TokenizerT* tokenizer;
    int len;
    int count;
    int back;
    catNode* front;

    fPath = (char *)arg;
    
    pthread_detach( pthread_self() ); //thread

    trace_file = fopen(fPath, "r");

    if (trace_file != NULL) {

        while (!feof(trace_file)) {

        	count = 0;
        	front = catFront;
            memset(buff, '\0', sizeof(buff));
            fgets(buff, 1024, (FILE*)trace_file);

			tokenizer = TKCreate("|\"", buff); // "\\ " as separators
	
			if(tokenizer == NULL) {
				//printf("Error: unable to create tokenizer\n");
				return 0;
			}
	
			char* token = NULL;
			
			while((token = TKGetNextToken(tokenizer)) != NULL) {
				len = strlen(token); 
				if(strcmp(token, "\n") == 0 ) {
					free(token);
					continue;
				}
				else if (strcmp(token + len - 1, "\n") == 0) {
					token[strlen(token)-1] = '\0'; 
				}

				if (count == 3) {

					while (front != NULL) {
						if (strcmp(front->category, token) == 0) {
							break;
						}
						front = front->next;
					}

					if(front == NULL) {
						printf("Book order category: %s. Was not found.\n", token);
						return 0;
					}

					pthread_mutex_lock( &front->mutex ); //thread
					printf("Producer locked queue for thread: %s\n", token);

	
					while (front->count == front->bufSize) { //queue is filled

						pthread_cond_signal( &front->dataAvailable );
						printf("Queue: %s. Is full. Producer is waiting for open positions.\n", token);
						pthread_cond_wait( &front->spaceAvailable, &front->mutex );


					}
			

					back = (front->front + front->count) % front->bufSize;
					front->buf[back] = malloc(strlen(buff) + 1);
					memcpy(front->buf[back], buff, strlen(buff));
					front->buf[back][strlen(buff)] = '\0';
					front->count = front->count + 1;

					pthread_cond_signal( &front->dataAvailable );
					pthread_mutex_unlock( &front->mutex );
					printf("Producer unlocked queue for thread: %s\n", token);

				}
				else {
					count = count + 1;

				}

				free(token);
			}
	
			TKDestroy(tokenizer);
           
        }

        fclose(trace_file);
		//double check ending


   	}
   	else {
   		printf("\nFile does not exit or the input does not include the extension\nEx. sample.txt\n\n");
   		
   	}


        front = catFront;
        while (front != NULL) {
			printf("%s is FINISHED\n", front->category);
			pthread_cond_signal( &front->dataAvailable );
			front->isRecieving = 0;

			while (front->isDone != 1) {
				pthread_cond_signal( &front->dataAvailable );
			}

			front = front->next;
		}

   	return 0;

}

void* consumer(void* arg) {
//void consumer(void* arg) {

	char* tString;
	TokenizerT* tokenizer;
    int len;
	int count;
	node_C* front;
	node_s* temp;
	node_s* prevSuccess;
	node_s* prevRejected;
	catNode* curNode = (catNode* )arg;

	pthread_detach( pthread_self() );
	while (curNode->isRecieving) {

		pthread_mutex_lock( &curNode->mutex );
		printf("Consumer locked queue for thread: %s\n", curNode->category);


		while( (curNode->count == 0) && curNode->isRecieving) {
			pthread_cond_signal( &curNode->spaceAvailable );
			printf("Queue: %s. Is empty, consumer is waiting for open positions.\n", curNode->category);
			pthread_cond_wait( &curNode->dataAvailable, &curNode->mutex );
			if ( (curNode->isRecieving == 0) && (curNode->count == 0)) {
				pthread_mutex_unlock( &curNode->mutex );
				printf("Done: %s (in middle)\n", curNode->category);
				curNode->isDone = 1;
				return 0;
			}
		}
		int i;
		for ( i = 0 ; (curNode->count > 0) ; i++ ) {

			count = 0;

			tString = malloc(strlen(curNode->buf[curNode->front]) + 1);
			memcpy(tString, curNode->buf[curNode->front], strlen(curNode->buf[curNode->front]));
			tString[strlen(curNode->buf[curNode->front])] = '\0';

			//find customer ID

			tokenizer = TKCreate("|\"", tString); // "\\ " as separators
	
			if(tokenizer == NULL) {
				//printf("Error: unable to create tokenizer\n");
				return 0;
			}
	
			char* token = NULL;
			
			while((token = TKGetNextToken(tokenizer)) != NULL) {
				len = strlen(token); 
				if(strcmp(token, "\n") == 0 ) {
					free(token);
					continue;
				}
				else if (strcmp(token + len - 1, "\n") == 0) {
					token[strlen(token)-1] = '\0'; 
				}

				//do stuff here
				//add count for 3rd
				if (count == 0) {
					temp = malloc(sizeof(node_s));
					temp->bookTitle = malloc(strlen(token) + 1);
					memcpy(temp->bookTitle, token, strlen(token));
					temp->bookTitle[strlen(token)] = '\0';
					count = count + 1;
				}
				else if (count == 1) {
					temp->price = atof(token);
					count = count + 1;
				}
				else if (count == 2) {

					front = customerFront;
					while (front != NULL) {
						if(atoi(token) == front->userID) {
							break;
						}
						front = front->next;
					}

					printf("Category: %s. Locking customer: %s\n", curNode->category, front->name);
					pthread_mutex_lock( &front->mutex );

					//process order
					temp->next = NULL;
					
					if((front->creditLimit - temp->price) > 0 ) { //successful 
						temp->remaining = (front->creditLimit - temp->price);
						front->creditLimit = (front->creditLimit - temp->price);


						if (front->successful == NULL) {
							front->successful = temp;
						}
						else {
							prevSuccess = front->successful;
							while(prevSuccess->next != NULL) {
								prevSuccess = prevSuccess->next;
							}
							prevSuccess->next = temp;
						}
					}

					else { //rejected
						temp->remaining = 0;

						if (front->rejected == NULL) {
							front->rejected = temp;
						}
						else {
							prevRejected = front->rejected;
							while(prevRejected->next != NULL) {
								prevRejected = prevRejected->next;
							}
							prevRejected->next = temp;

							/*
							prevRejected->next = temp;
							prevRejected = prevRejected->next;
							*/
						}


					}
					printf("Placed order: %s. with %.2f left.\n", temp->bookTitle, front->creditLimit);
					pthread_mutex_unlock( &front->mutex );
					
					count = count + 1;

				}
				else{
					count = count + 1;
				}
				
				free(token);
			}
	
			TKDestroy(tokenizer);

			curNode->front = (curNode->front + 1) % curNode->bufSize;
			curNode->count = curNode->count - 1;

		}

		pthread_cond_signal( &curNode->spaceAvailable );
		pthread_mutex_unlock( &curNode->mutex );
		printf("Consumer unlocked queue for thread: %s\n", curNode->category);
	}
	printf("Done: %s\n", curNode->category);
	curNode->isDone = 1;

	return 0;
}


void results(int co, char* printName, FILE *fp, double totRev){
    //FILE *fp;
    //char* printName;
    fp = fopen( printName , "w+" );
    node_C* temp = customerFront;
     node_s* smTemp;
    fprintf(fp,"Results:\n");
    fprintf(fp, "\nTotal Revenue:%.2f\n\n", totRev);
    while(co!=0){
        fprintf(fp, "=== BEGIN CUSTOMER INFO ===\n");
        fprintf(fp, "### BALANCE ###\n");
        fprintf(fp, "Customer name:%s\n", temp->name);
      fprintf(fp, "Customer ID number:%d\n", temp->userID);
      fprintf(fp, "Remaining credit balance after all purchases (a dollar amount):%.2f\n", temp->creditLimit);
        fprintf(fp, "### SUCCESSFUL ORDERS ###\n");
         smTemp = temp->successful;
        while (smTemp != NULL) {
        	fprintf(fp, "\"%s\" | %.2f | %.2f\n", smTemp->bookTitle, smTemp->price, smTemp->remaining);
        	smTemp = smTemp->next;
        }
        co = co-1;
    
    	//need some loop to dequeue and put stuff in this succesful orders list
    	fprintf(fp, "### REJECTED ORDERS ###\n");

    	smTemp = temp->rejected;
    	while (smTemp != NULL) {
        	fprintf(fp, "\"%s\" | %.2f\n", smTemp->bookTitle, smTemp->price);
        	smTemp = smTemp->next;
        }


    //need anothr loop
    fprintf(fp, "=== END CUSTOMER INFO ===\n");
    fprintf(fp, "\n");
        temp = temp->next;
    }
   fclose(fp);
     
   return;
}

void freeSmall(node_s* del) {
	if(del != NULL) {
		freeSmall(del->next);
		free(del);
	}
}

void freeCus(node_C* del) {
	if(del != NULL) {
		freeCus(del->next);
		freeSmall(del->successful);
		freeSmall(del->rejected);
		free(del);
	}
}

void freeCat(catNode* del) {
	if(del != NULL) {
		freeCat(del->next);
		free(del);
	}
}


int main(int argc, char **argv) {

	catNode* tmp;
	node_C* cusFront;
	int done;
	int totalCount;
	double totalRev;
	FILE* fp;
	pthread_t	ignore;
	
	fp = NULL;
	totalCount = 0;

	if(argc != 4){ //Program will contain 3 arguments
		printf("Error: invalid number of arguments\n");
		return 0;
	}
	
	//do main

	customerFront = NULL;
	//catFront = (catNode* )malloc(sizeof(catNode));
	catFront = NULL;

	if (setUpCus(argv[1])) {
		printf("set up failed\n");
	}
	else if (setupCat(argv[3])) {
		printf("set up failed\n");
	}

	else {
		//producer(argv[2]);
		
		pthread_create( &ignore, 0, producer, argv[2] );

		tmp = catFront;
		while (tmp != NULL) {
			//consumer(tmp);
			totalCount = totalCount + 1;
			pthread_create( &ignore, 0, consumer, tmp );
			tmp = tmp->next;
		}

		if (catFront != NULL) {

			while (1) {
				
				done = 0;
				tmp = catFront;
				while (tmp != NULL) {
					if (tmp->isDone == 1) { 
						done = done + 1;
					}
					tmp = tmp->next;
				}
				if (done == totalCount) {
					break;
				}

			}

			totalRev = 0;
			cusFront = customerFront;
			while (cusFront != NULL) {
				totalRev = totalRev + (cusFront->origLimit - cusFront->creditLimit);
				cusFront = cusFront->next;
			}

			printf("Please wait, printing output file..\n");
			results(mcounter, "finalreport.txt", fp, totalRev);	

			freeCus(customerFront);
			freeCat(catFront);

		}
		
		pthread_exit( 0 );

	


		
	}
/*
	node_C* temp;
	node_s* smTemp;
	temp = customerFront;
	
	while (temp != NULL) {

					printf("%f\n", temp->creditLimit);
				
					smTemp = temp->successful;
					while (smTemp != NULL) {
						printf("successful: %s\n", smTemp->bookTitle);
						smTemp = smTemp->next;
					}
					smTemp = temp->rejected;
					while (smTemp != NULL) {
						printf("rejected: %s\n", smTemp->bookTitle);
						smTemp = smTemp->next;
					}

		temp = temp->next;
	}
*/


/*
	tmp = catFront;
	printf("start\n");
	while (tmp != NULL) {

		printf("ok%s\n", tmp->category);
		tmp = tmp->next;
	}
*/
/*
	tmp = catFront;

	while (tmp != NULL) {
		printf("%s\n", tmp->category);

		for(int i = 0; i< tmp->count; i++) {
			printf("%s\n", tmp->buf[i]);
		}

		tmp = tmp->next;
	}
*/
	return 0;

}