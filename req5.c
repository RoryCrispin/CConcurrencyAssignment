// Rory Crispin - psyrc3 - 4250167
//
// G52OSC Task 5
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define JOB_INDEX 0
#define BURST_TIME 1
#define REMAINING_TIME 2
#define PRIORITY 3
#define JOBS_REMOVED 4
#define JOBS_INBUF 5

/* 
 * -- Configuraton constants -- 
 */
#define QUEUE_SIZE 5
#define NUMBER_OF_JOBS 1000
#define NUMBER_OF_CONSUMERS 4



/*
 * Foreward decleration of functions for cleanliness
 */
int * enqueueJob();
void printJob(int iId, int iBurstTime, int iRemainingTime);
void printFinishedJob(int id, int b_time, int s_time, int jremoved, int jinbuf );

int jobsProduced;
int jobsRemoved;

__thread int consumerID;
/*
 * -- QUEUE --
 *
 * Code relating to managing the queue
 * much of this code was copied through from req4.c
 * I originally write this as a single file and included
 * queue.h in req4,5 and 6 but was to>d not to for submission 
 *
 */

int threadTimes[NUMBER_OF_CONSUMERS];

int fronti; //These ints point to the front and rear of the array.
int reari; //I use integer indexes rather than pointers to avoid pointer arithmetic bugs.

int currentQueueCount;
int jobsGenerated;

int queue[QUEUE_SIZE][4];

int getIndex(int loc) {
	return ((loc+1)%QUEUE_SIZE);	
}

void initQueue(){
	fronti = 0;
	reari = 0;
	currentQueueCount = 0;
}

int isEmpty(){
	return fronti==reari;
}

int isFull(){
	return currentQueueCount == QUEUE_SIZE;
}

int * enqueue(){
	int *job;
	if (isFull() ==1){
		printf("Queue Full!\n");
		job =(int *) -1;
		return job;
	}	
	fronti++;
	job = enqueueJob(getIndex(fronti));
	jobsGenerated++;
	currentQueueCount++;	
	return job;
}

int getQueueSize(){
	return currentQueueCount;	
}


int* dequeue(){

	reari++;
	int * item = queue[getIndex(reari)];
	currentQueueCount --;
	/*printf("Dequeue: %c\n", item);*/
	return item;
}
int * getTopOfQueue(){
	return queue[getIndex(reari+1)];
}


int startTime[NUMBER_OF_JOBS];


double getArraySum(int *array, int length){
	double sum = 0;
	int i;
	for(i=0; i<length; i++ ){
		sum += array[i];
	}
	return sum;
}

double getArrayAvg(int *array, int length){
	return getArraySum(array, length)/length;	
}



void jobStarted(int index, int s_Time){
	startTime[index] = s_Time;
}



long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end)
{
	int iSeconds = end.tv_sec - start.tv_sec;
	int iUSeconds = end.tv_usec - start.tv_usec;
	long int mtime = (iSeconds * 1000 + iUSeconds / 1000.0);
	return mtime;
}

void simulateJob(int iTime)
{
	long int iDifference = 0;
	struct timeval startTime, currentTime;
	gettimeofday(&startTime, NULL);
	do
	{	
		gettimeofday(&currentTime, NULL);
		iDifference = getDifferenceInMilliSeconds(startTime, currentTime);
	} while(iDifference < iTime);}



int getThreadTime(){
	return threadTimes[consumerID];
}

void addThereadTime(int time){
	threadTimes[consumerID] += time;
}


// Function that simulates running jobs, decrements their counter and registers when jobs are started and finished. 
// Makes use of functions jobStarted,  simulateJob, printProgress documented elsewhere. 
void runJob(int *job){

	int rem_timeBeforeSimulate = job[REMAINING_TIME]; // cache remaining time before simulating running job	
	int startTime = getThreadTime();
	jobStarted(job[JOB_INDEX], startTime);
	simulateJob(rem_timeBeforeSimulate);
    job[REMAINING_TIME] = 0;
	printFinishedJob(job[JOB_INDEX], job[BURST_TIME], startTime, job[JOBS_REMOVED], job[JOBS_INBUF]);
	addThereadTime(job[BURST_TIME]);
}

void printOutJob(int * job){
	printJob(job[JOB_INDEX], job[BURST_TIME], job[REMAINING_TIME]);
}

// Takes a pointer to a function and applies each job in the queue to it 
void forEachInQueue(void (*queueAction)(int*)){
	int temp = reari+1;
	if (isEmpty()){
		return;
	}
	while (temp!=fronti+1){
		int index = getIndex(temp);
		queueAction(queue[index]);
		temp++;
	}
	printf("\n");
}


/*
 * Generates a job and inserts it into the queue at index i.
 * This function is called by the queue's enqueue function such that
 * i is the next free space in the queue.
 */

int * enqueueJob(int i)
{
	queue[i][JOB_INDEX] = jobsGenerated;
	queue[i][BURST_TIME] = rand() % 100;
	queue[i][REMAINING_TIME] = queue[i][BURST_TIME];
	queue[i][PRIORITY] = rand()%10;
	return queue[i];
}


void printJob(int iId, int iBurstTime, int iRemainingTime)
{
	printf("Id = %d, Burst Time = %d, Remaining Time = %d, ", iId, iBurstTime, iRemainingTime);
}

void printFinishedJob(int id, int b_time, int s_time, int jremoved, int jinbuf ){
	printf("Thread %d removes: JOB ID = %d, Burst time = %d Start time = %d (jobs removed from buffer = %d, jobs in buffer = %d)\n", consumerID, id, b_time, s_time, jremoved, jinbuf);
}


/*
 * 
 * -- Poducer Consumer --
 * This code handles the producer/consumer problem. 
 * 
 */

sem_t sSync;
sem_t sDelay_Consumer;
sem_t sDelay_Producer;


int finishedProducing;

/*
 * The physical work done by the producer thread
 * I've extracted it from code relating to the
 * mutex for cleanliness
 *
 */

void producerWork(){
	jobsProduced++;
	printf("Producing: ");
	printOutJob(enqueue());
	printf("(jobs produced = %d, jobs in buffer: %d)\n", jobsProduced, currentQueueCount);
}
void * producer(void *arg) {
	int i;
	for(i=0;i<NUMBER_OF_JOBS; i++){
		sem_wait(&sSync);
		/*printNum();*/
		if (getQueueSize() >= 1){
			sem_post(&sDelay_Consumer);
		} 
		if (isFull() != 1) producerWork();
		sem_post(&sSync);
		if (isFull() == 1){
			sem_wait(&sDelay_Producer);
		}
	}
	finishedProducing = 1;
	sem_post(&sDelay_Consumer);
	return NULL;
}



void printSem(sem_t * sem){
	int sval;
	sem_getvalue(sem, &sval);
	printf("%d", sval);
}

void printSems(){
	printf("sSync = ");
	printSem(&sSync);
	printf(", sDelay_Consumer = ");
	printSem(&sDelay_Consumer);
	printf(", sDelay_Producer = ");
	printSem(&sDelay_Producer);
	printf("\n");
}

void consumerWork(int* job){
	/*printf("Thread %d removes: ", consumerID);*/
	runJob(job);
	/*printf("(jobs removed fom buffer = %d, jobs in buffer = %d)\n", jobsRemoved, currentQueueCount);*/

}
void cloneRow(int *a, int *b){
	a[JOB_INDEX] = b[JOB_INDEX];
	a[BURST_TIME] = b[BURST_TIME];
	a[REMAINING_TIME] = b[REMAINING_TIME];
	a[PRIORITY] = b[PRIORITY];
}
void * consumer(int consumerIDarg){
	int temp;
	sem_wait(&sDelay_Consumer);
	int lastProduced = 0;
	consumerID = consumerIDarg;
	int jobStore[6];
	int *job;
	while(1){
		if (isEmpty()!= 1){
			sem_wait(&sSync);
			temp = getQueueSize();	
			if(getQueueSize() == QUEUE_SIZE) {
				lastProduced = 1;
			}
			if (getQueueSize() > 0){ // Only dequeue if buffer not empty
				job = dequeue();
				jobsRemoved++;
				cloneRow(jobStore, job); //Store a temporary copy of the job as it may be overwritten/moved in the queue once the critical section is left	
				jobStore[JOBS_REMOVED] = jobsRemoved;
				jobStore[JOBS_INBUF] = currentQueueCount;
				temp = getQueueSize();

				if (lastProduced == 1){
					sem_post(&sDelay_Producer);
					lastProduced = 0;
				}
			}

			sem_post(&sSync);	

			consumerWork(jobStore); //Consume the job *after* releasing the lock. The critical section is only the time taken to remove the job from the queue.
			temp = getQueueSize();
		}
		/*if (getQueueSize() <=0){*/
		if (temp <=0){
			if (finishedProducing ==1){
				int sval;
				sem_getvalue(&sDelay_Consumer, &sval);
				if (sval<=0){
					sem_post(&sDelay_Consumer); //Wake up the next sleeping consumer so it can terminate
				}
				/*printf("Thread %d closing!\n", consumerID);*/
				return NULL; // If buffer empty & producer finished, exit thread

			}	
			sem_wait(&sDelay_Consumer);
		}
		}

		return NULL;

}



void setupSemaphores(){
	jobsProduced = 0;
	jobsRemoved = 0;
	sem_init(&sSync, 1, 1);
	sem_init(&sDelay_Consumer, 1,0);
	sem_init(&sDelay_Producer, 1,0);

	pthread_t p1;
	pthread_create(&p1, NULL, producer, NULL);


	int i;
	pthread_t consumerThreads[NUMBER_OF_CONSUMERS];
	for(i=0; i<NUMBER_OF_CONSUMERS; i++){
		pthread_create(&consumerThreads[i], NULL, consumer, i);
	}
	for(i=0; i<NUMBER_OF_CONSUMERS; i++){
		pthread_join(consumerThreads[i], NULL);
	}

	pthread_join(p1, NULL);

}

int main(int argc, char *argv[]) {

	if (NUMBER_OF_CONSUMERS == 0 || NUMBER_OF_JOBS ==0){
		printf("Invalid prams\n");
		return 0;
	}
	initQueue();
	setupSemaphores();
	/*forEachInQueue(printOutJob);*/
	/*printf("Average Runtime:%f\n", getAverageRuntime());*/

	printSems();
	printf("Average Start Time: %f\n", getArrayAvg(startTime, NUMBER_OF_JOBS) );
	return 0;

}
