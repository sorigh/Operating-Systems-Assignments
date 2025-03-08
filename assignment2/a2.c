#include "a2_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

//process 6
#define N6 4

//process 7
#define N7 44
#define MAX_THREADS 5


#define N5 5
typedef struct {
    int thread_id;
    int process_id;
    sem_t *start_sem;
    sem_t *end_sem;
} ThreadArgs;


//process 7
int nrThreads;
int totalThreads;
sem_t max_threads_sem;
sem_t max_passing_threads;

void *thread_function6(void *arg){
	ThreadArgs *args = (ThreadArgs*)arg;
	//-----------------------------------begin statements
	if (args->thread_id == 1) {
		//------ procese diferite
		sem_t * t61_can_start = sem_open("/t61_started", O_CREAT, 0644,0);
		sem_wait(t61_can_start);
		info(BEGIN, args->process_id, args->thread_id);
		sem_destroy(t61_can_start); // last time used here
		
		
	}else if (args->thread_id == 3) {
		info(BEGIN, args->process_id, args->thread_id);
        	sem_post(args->start_sem); //thread 3 starts before thread 2
    	} else if (args->thread_id == 2) {
		sem_wait(args->start_sem); //thread 2 waits for thread 3 to start
		info(BEGIN, args->process_id, args->thread_id);
	} else {
	//other threads
	info(BEGIN, args->process_id, args->thread_id);
	}
	
	//----------------------------------- end statements
	 if (args->thread_id == 3) {
	 
		sem_wait(args->end_sem); //thread 3 waits for thread 2 to end
		info(END, args->process_id, args->thread_id);
		
    	 } else if (args->thread_id == 2) {
    	 
   	 	info(END, args->process_id, args->thread_id);
        	sem_post(args->end_sem); //thread 2 ends before thread 3
        	
   	} else {
   	 //other threads
   	 	if (args->thread_id == 1){
   	 		//------ procese diferite
 			info(END, args->process_id, args->thread_id);
 			sem_t * t55_can_start = sem_open("/t55_can_start", O_EXCL);
 			sem_post(t55_can_start);
 			
 		} else {
 			info(END, args->process_id, args->thread_id);
 		}
 	}
    	return NULL;
}

void Proces6Activity() {
	pthread_t threads[N6];
	ThreadArgs thread_args[N6];
	sem_t start_sem, end_sem;
    	sem_init(&start_sem, 0, 0);
    	sem_init(&end_sem, 0, 0);
    
    	//starting threads
	for (int i = 0; i < N6; i++) {
		thread_args[i].thread_id = i+1;
		thread_args[i].process_id = 6;
		thread_args[i].start_sem = &start_sem;
        	thread_args[i].end_sem = &end_sem;
		pthread_create(&threads[i], NULL, thread_function6, &thread_args[i]);
	}
	
	//ending threads
	for (int i = 0; i < N6; i++) {
	if (thread_args[i].thread_id != 3)
        	pthread_join(threads[i], NULL);
    	}
    	//ending thread 3 last
	pthread_join(threads[2], NULL);
	
	
 	sem_destroy(&start_sem);
    	sem_destroy(&end_sem);
}

void *thread_function7(void *arg){
	ThreadArgs *args = (ThreadArgs*)arg;
	sem_wait(&max_threads_sem);
	nrThreads++;
	//-----------------------------------begin statements
	info(BEGIN, args->process_id, args->thread_id);
	totalThreads++; //count ALL passing threads
	
	//condition for thread 15 to end when 5 threads are running
	if (args->thread_id == 15){
		//waits for all threads to pass
		while (totalThreads!= N7); 
		
		//we know the last 4 threads are blocked by max_passing_threads
		info(END, args->process_id, args->thread_id);

		//release others waiting
		sem_post(&max_passing_threads);
		sem_post(&max_passing_threads);
		sem_post(&max_passing_threads);
		sem_post(&max_passing_threads);
		
		//release itself
		sem_post(&max_passing_threads); 
	}
	sem_wait(&max_passing_threads);

	//-----------------------------------end statements
	info(END, args->process_id, args->thread_id);
	
	nrThreads--;
	sem_post(&max_threads_sem);
	return NULL;
}

void Proces7Activity(){
	pthread_t threads[N7];
	ThreadArgs thread_args[N7];
	sem_init(&max_threads_sem, 0,MAX_THREADS); //limita ptr zona critica
 	sem_init(&max_passing_threads, 0, N7 - 5); //lasam toate sa treaca mai putin ultimele 5 (incluzand 15)

 	//starting threads
	for (int i = 0; i < N7; i++) {
		thread_args[i].thread_id = i+1;
		thread_args[i].process_id = 7;
		pthread_create(&threads[i], NULL, thread_function7, &thread_args[i]);
	}
	
	//ending threads
	for (int i = 0; i < N7; i++) {
        	pthread_join(threads[i], NULL);
    	}
    	
    	sem_destroy(&max_threads_sem);
    	sem_destroy(&max_passing_threads);
    	
}

//in thread function 5 si thread function 6
//1. sem= sem_open("/nume", O_CREAT, 0644,0);
//2. sem = sem_open("/nume", O_EXCL);

void *thread_function5(void *arg){
	ThreadArgs *args = (ThreadArgs*)arg;
	
	//-----------------------------------begin statements
	if (args->thread_id == 5){
		sem_t * t55_can_start = sem_open("/t55_can_start", O_CREAT, 0644,0);
		sem_wait(t55_can_start);
		info(BEGIN, args->process_id, args->thread_id);
		sem_destroy(t55_can_start); //last used here
	} else {
		info(BEGIN, args->process_id, args->thread_id);
	}
	
	
	
	//-----------------------------------end statements
	if (args->thread_id == 1){
		info(END, args->process_id, args->thread_id);
		sem_t * t61_can_start = sem_open("/t61_started", O_EXCL);
		sem_post(t61_can_start);
	} else {
		info(END, args->process_id, args->thread_id);
	}
	return NULL;
}

void Proces5Activity(){
	pthread_t threads[N5];
	ThreadArgs thread_args[N5];
	
	//starting threads
	for (int i = 0; i < N5; i++) {
		thread_args[i].thread_id = i+1;
		thread_args[i].process_id = 5;
		pthread_create(&threads[i], NULL, thread_function5, &thread_args[i]);
	}
	
	//ending threads
	for (int i = 0; i < N5; i++) {
        	pthread_join(threads[i], NULL);
    	}
    	
}
void ierarhie() {
    pid_t pid_p2, pid_p3, pid_p4, pid_p5, pid_p6, pid_p7;
    info(BEGIN, 1, 0);
    pid_p2 = fork();
    if (pid_p2 == 0) { 
        info(BEGIN, 2, 0);
        pid_p6 = fork();
        if (pid_p6 == 0) { //--------------------doar proces 6
            info(BEGIN, 6, 0);
	    Proces6Activity(pid_p6);
            info(END, 6, 0);
            exit(0);
        }
        else { // -------------------- doar proces 2
            waitpid(pid_p6, NULL, 0);
            info(END, 2, 0);
            exit(0);
        }
    }
    else {
        pid_p3 = fork();
        if (pid_p3 == 0) { // -------------------- doar proces 3
            info(BEGIN, 3, 0);
            info(END, 3,0);
            exit(0);
        }
        else {

            pid_p4 = fork();
            //in p4
            if (pid_p4 == 0) {
                info(BEGIN, 4, 0);
                pid_p5 = fork();
                if (pid_p5 == 0) { // -------------------- doar proces 5
                    info(BEGIN, 5, 0);
                    Proces5Activity();
                    info(END, 5, 0);
                    exit(0);
                }
                else{
                    pid_p7 = fork();
                    if (pid_p7 == 0) { // -------------------- doar proces 7
                        info(BEGIN, 7, 0);
                        Proces7Activity();
                        info(END, 7, 0);
                        exit(0);
                    }
                    else { // -------------------- doar proces 4
                        waitpid(pid_p5, NULL, 0);
                        waitpid(pid_p7, NULL, 0);
                        info(END, 4, 0);
                        exit(0);
                    }
                }
            }
            else { // -------------------- doar proces 1
                waitpid(pid_p2, NULL, 0);
                waitpid(pid_p3, NULL, 0);
                waitpid(pid_p4, NULL, 0);
                info(END, 1, 0);

            }
        }
    }
}

int main(int argc, char **argv){
	init();
	ierarhie();
	return 0;
}
