//AUTHOR: Jesse McCarville-Schueths
//COURSE: CS 4760
//DATE: OCT 10 2020
//FILENAME: palin.c
//
//DESCRIPTION: working with Concurrent UNIX Processes and shared memory with semaphores
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <sys/sem.h>

#define PERMS (S_IRUSR | S_IWUSR)

void signal_handle(int signal);
int free_shared_mem();
void timer(int signal);

bool isPalindrome(char str[]);


typedef struct{
	unsigned int sem_id;
	unsigned int sem_key;
	unsigned int id;
	unsigned int turn;
	unsigned int children; //amount of child processes 
        unsigned int flags[20];  // state of using critical section
        char strings[64][64]; // array of strings
	pid_t pgid;
	struct sembuf sem_op;
} shared_memory;
shared_memory *ptr; // pointer for the shared memory
int shm_id;


int main(int argc, char ** argv){
	//ptr->in_critical = false;
//	printf("Palin Starting...\n");
	signal(SIGTERM, signal_handle);
	signal(SIGUSR1, timer);
	int id = atoi(argv[1]);
//	printf("id: %d\n", id);
//	return 0;
	 	
	unsigned int key = ftok("./master", 'a');	

	shm_id = shmget(key, sizeof(shared_memory), PERMS | IPC_CREAT);
	if (shm_id == -1) {
		perror("Failed to find shared memory segment");
		return 1;
	}
	
	// attach shared memory segment
	ptr = (shared_memory*)shmat(shm_id, NULL, 0);
	// shmat(segment_id, NULL, SHM_RDONLY) to attach to read only memory
	if (ptr == (void*)-1) {
		perror("Failed to attach to shared memory segment");
		return 1;
	}

	
	//printf("child argument: %d ,string %s",id, ptr->strings[id]);

	bool palin = isPalindrome(ptr->strings[id]);



	//--------------- Waiting Room for Crit Sect -------------------
	printf("Process ID: %d, PID: %d wants into critical section\n", id, getpid());



	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//ASSIGNMENT 3 PART
	//
	//



	struct sembuf sem_op;

	/* wait on the semaphore, unless it's value is non-negative. */
	sem_op.sem_num = 0;
	sem_op.sem_op = -1;   // wait if crit sect is locked
	sem_op.sem_flg = 0;
	semop(ptr->sem_id, &sem_op, 1);
	printf("Critical section locked...\n");



	

	// SLEEP BETWEEN 0-2 SECONDS -------------------------
	time_t t;
        srand((unsigned) time(&t)); // seed for random sleep time
        sleep(rand() % (3)); // sleep between 0-2 seconds

	
	//-=-=-=-=-=-=-=-=- CRITICAL SECTION -=-=-=-=-=-=-=-=-	
	


	printf("Process ID: %d, PID: %d has entered critical section\n", id, getpid());
	
	// writing to output files
	FILE *file_a = fopen(palin ? "palin.out" : "nopalin.out", "a+"); // a+ appends
	if(file_a == NULL){
		perror("FILE ERROR");
	}
	fprintf(file_a, "%s\n", ptr->strings[id]);
	
	pid_t pid = getpid();
	fclose(file_a); // closes file_a
	
	FILE *file_b = fopen("output.log", "a+");
        if(file_b == NULL){
              perror("FILE ERROR");
        }
	fprintf(file_b, "PID: %d, Index: %d, String %s\n", pid, id, ptr->strings[id]); 

	fclose(file_b);
	
	printf("Process ID: %d, PID: %d has exited critical section\n", id, getpid());	
	
	// ------------------------------------------------------------------------







	// increase semaphore value by one.
	sem_op.sem_num = 0;
	sem_op.sem_op = 1;  // free the crit section
	sem_op.sem_flg = 0;
	semop(ptr->sem_id, &sem_op, 1);	
	printf("Critical section free...\n");

	return 0;
}


bool isPalindrome(char str[]) {
	bool palin = true;
	int l = 0;
	int h = strlen(str) - 1;
	while (h > l){
        	if(str[l++] != str[h--]){		 
			return false;
		}
	}
	return true; 
}

void signal_handle(int signal){
	printf("\nMessage: Ctrl+c in child caught!\n");
	exit(1);
}

void timer(int signal){
	exit(1);
}

int free_shared_mem(){
	//detach from shared memory
	int detach = shmdt(ptr);
        if (detach == -1){
                perror("Failed to detach shared memory segment");
                return 1;
        }

        int delete_mem = shmctl(shm_id, IPC_RMID, NULL);
        if (delete_mem == -1){
                perror("Failed to remove shared memory segment");
                return 1;
        }
	return 1;
}
