#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#define NUMBER_OF_JOBS 1000

sem_t sSync;
sem_t sDelay_Consumer;
int finishedProducing;
int numInQueue;

void printNum(){
	printf("iIndex = %d\n", numInQueue);
}

void * producer(void *arg) {
	int i;
	for(i=0;i<NUMBER_OF_JOBS; i++){ //Produce N items

		sem_wait(&sSync);
		numInQueue++;
		printNum();
		if (numInQueue == 1){
			sem_post(&sDelay_Consumer);
		}
		sem_post(&sSync);
	}
	finishedProducing = 1; // After N items produced, set finish producing flag and exit thread

	return NULL;

}

//returns the value of a semaphore as an int
int getSem(sem_t * sem){
	int sval;
	sem_getvalue(sem, &sval);
	//printf("%d", sval);
	return sval;

}

void * consumer(void *arg){
	int temp;
	sem_wait(&sDelay_Consumer);
	while(1){
		sem_wait(&sSync);
		numInQueue--;
		printNum();
		temp = numInQueue;
		sem_post(&sSync);
		if (temp <=0){
			if (finishedProducing ==1) return NULL; // If buffer empty & producer finished, exit thread
			sem_wait(&sDelay_Consumer);
		}
	}
	return NULL;

}

//Prints formatted semaphores info at the end of the program
void printSems(){
	printf("sSync = %d, sDelay_Consumer = %d\n", getSem(&sSync), getSem(&sDelay_Consumer));
}


int main(int argc, char *argv[]) {
	sem_init(&sSync, 1, 1);
	sem_init(&sDelay_Consumer, 1,0);

	pthread_t c1, p1;
	pthread_create(&c1, NULL, consumer, NULL);
	pthread_create(&p1, NULL, producer, NULL);

	pthread_join(p1, NULL);
	pthread_join(c1, NULL);

	printSems();
	return 0;

}
