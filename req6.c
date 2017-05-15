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
#define NUMBER_OF_CONSUMERS 3
#define TIME_SLICE 25



/*
 * Foreward decleration of functions for cleanliness
 */
int * enqueueJob();
void printJobProduced(int iId, int iBurstTime, int iRemainingTime, int priority);
void printJobDetails(int iId, int iBurstTime, int iRemainingTime, int priority);

int getNumItemsWithPriority(int query);
void forEachInQueue(void (*queueAction)(int*));
int * getNthElement(int n);
void printFinishedJob(int id, int b_time, int s_time );
void printOutJob(int * job);
void insertItemSorted(); 
void swapRows(int *a, int *b);
void printProgres(int *job, int startTime, int newJob, int delta_runningTime, int jobFinished);

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
/*int threadsThatHaveRun[NUMBER_OF_CONSUMERS];*/

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

/*
 * This funciton is called by the producer to generate
 * random jobs and add them to the queue
 *
 */

int * generateAndEnqueue(){
	int *job;
	if (isFull() ==1){
		printf("Queue Full!\n");
		job =(int *) -1;
		return job;
	}	
	fronti++;
	job = enqueueJob(getIndex(fronti));
	printOutJob(job);
	insertItemSorted();
	jobsGenerated++;
	currentQueueCount++;	
	return job;
}

int * enqueueWithParams(int jobIndex, int bTime, int remTime, int priority){
	if (isFull() ==1){
		printf("Queue Full!\n");
	}	
	fronti++;

	if (jobIndex == -1){
		queue[ getIndex(fronti)][JOB_INDEX] = jobsGenerated;
	} else {
		queue[getIndex(fronti)][JOB_INDEX] = jobIndex;
	}
	queue[getIndex(fronti)][BURST_TIME] = bTime;
	queue[getIndex(fronti)][REMAINING_TIME] = remTime;
	queue[getIndex(fronti)][PRIORITY] = priority;

	insertItemSorted(); //Place item where it should be in queue
	/*printOutJob(job);*/
	jobsGenerated++;
	currentQueueCount++;	
}

void enqueueWithJobRef(int * job, int deltaTime){
	enqueueWithParams(job[JOB_INDEX], job[BURST_TIME], job[REMAINING_TIME]-deltaTime
			, job[PRIORITY]);
}


int getQueueSize(){
	return currentQueueCount;	
}


int* dequeue(){
	reari++;
	int * item = queue[getIndex(reari)];
	currentQueueCount --;
	return item;
}
int * getTopOfQueue(){
	return queue[getIndex(reari+1)];
}

int * getNthElement(int n){
	if (n> currentQueueCount){
		printf("Tried to fetch nth element greater than queue: %d in %d!\n", n, currentQueueCount);	
	}
	return queue[getIndex(reari+1+n)];
}

void forEachInQueue(void (*queueAction)(int*)){

	int i;
	for(i=0; i<currentQueueCount; i++){
		queueAction(getNthElement(i));
	}
}


// This is a debugging function that prints out the queue array 
void printQueue(){	int i; 
	for(i=0; i<QUEUE_SIZE; i++){
		if (getIndex(fronti) == i) printf("f");
		if (getIndex(reari) == i) printf("r");
		printf("i=%d, ITEM: ID %d bTime %d\n", i,queue[i][JOB_INDEX], queue[i][BURST_TIME]);
	}
}


void moveFrontToBackOfQueue(int deltTime){
	int *job;
	job=dequeue();
	/*printf("id=%d", job[JOB_INDEX]);	*/
	enqueueWithJobRef(job, -deltTime);

}

/*
 *
 *
 * Sorting 
 *
 */


//Clones b row INTO a
void cloneRow(int *a, int *b){
	a[JOB_INDEX] = b[JOB_INDEX];
	a[BURST_TIME] = b[BURST_TIME];
	a[REMAINING_TIME] = b[REMAINING_TIME];
	a[PRIORITY] = b[PRIORITY];
}

void swapRows(int *a, int *b){
	int tempRow[4];
	cloneRow(tempRow, a);
	cloneRow(a,b);
	cloneRow(b,tempRow);
}	

int getNumItemsWithPriority_(int priorityq){
	int i; 
	int num;
	for(i=0; i<currentQueueCount; i++){
		if (getNthElement(i)[PRIORITY]==priorityq){//TODO strictly equal to
			num++;
		}
	}
	return num; 
}

void insertItemSorted(){
	int i;
	for (i=currentQueueCount; i>0; i--){
		if (getNthElement(i)[PRIORITY]<getNthElement(i-1)[PRIORITY]){
			swapRows(getNthElement(i), getNthElement(i-1));
		}
	}
}



/*
   :* Code that handles calculating the runtime and response time
 * arrays are keps with the start and finish time of each job,
 * these are then calculated upon to find average times
 *
 */


int startTime[NUMBER_OF_JOBS];
int finishTime[NUMBER_OF_JOBS];


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

int* subTwoArrays(int * array1, int * array2){
	static int sum[NUMBER_OF_JOBS];
	int i;
	for (i=0; i<NUMBER_OF_JOBS; i++){
		sum[i] = array1[i] - array2[i]; 
	}	
	return sum;
}
int getRunningtime(int index){
	return finishTime[index] - startTime[index]; 	  
}

void jobFinished(int index){
	finishTime[index] = getThreadTime();
}


void jobStarted(int index, int s_Time){
	startTime[index] = s_Time;
}

double getAverageRuntime(){
	return getArrayAvg(subTwoArrays(finishTime, startTime), NUMBER_OF_JOBS);
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
	threadTimes[consumerID] += iTime;
	long int iDifference = 0;
	struct timeval startTime, currentTime;
	gettimeofday(&startTime, NULL);
	do
	{	
		gettimeofday(&currentTime, NULL);
		iDifference = getDifferenceInMilliSeconds(startTime, currentTime);
		/*} while(0);*/
} while(iDifference < iTime);// TODO: Removed delay!
}



int getThreadTime(){
	return threadTimes[consumerID];
}

void addThereadTime(int time){
	threadTimes[consumerID] += time;
}


// Function that simulates running jobs, decrements their counter and registers when jobs are started and finished. 
// Makes use of functions jobStarted, jobFinished, simulateJob, printProgress documented elsewhere. 
void runJob(int *job){
	int newJob = 0;
	int startTime =  threadTimes[consumerID];
	if (job[REMAINING_TIME] == job[BURST_TIME]) {
		newJob = 1;
		jobStarted(job[JOB_INDEX], threadTimes[consumerID]);
	}
	/*printf("p%d:%d  ", getNumItemsWithPriority(job[PRIORITY]), job[PRIORITY]);*/
	if (getNumItemsWithPriority(job[PRIORITY]) <=1 || job[REMAINING_TIME] <=  TIME_SLICE){
		/*printf("don");*/
		printProgres(job, startTime, newJob, job[REMAINING_TIME], 1 );
		simulateJob(job[REMAINING_TIME]);
		job[REMAINING_TIME] = 0;
		/*threadTimes[consumerID] += job[REMAINING_TIME];*/
	} else {
		printProgres(job, startTime, newJob, TIME_SLICE,0);
		simulateJob(TIME_SLICE);
		job[REMAINING_TIME]-=TIME_SLICE;
		/*threadTimes[consumerID]+=TIME_SLICE;*/
	}

}



void printOutJob(int * job){
	printJobProduced(job[JOB_INDEX], job[BURST_TIME], job[REMAINING_TIME], job[PRIORITY]);
}

void printOutJobDetails(int * job){
	printJobDetails(job[JOB_INDEX], job[BURST_TIME], job[REMAINING_TIME], job[PRIORITY]);
}




/*
 * Generates a job and inserts it into the queue at index i.
 * This function is called by the queue's enqueue function such that
 * i is the next free space in the queue.
 */

int * enqueueJob(int i)
{
	queue[i][JOB_INDEX] = jobsProduced;
	queue[i][BURST_TIME] = rand() % 100;
	queue[i][REMAINING_TIME] = queue[i][BURST_TIME];
	queue[i][PRIORITY] = rand()%10;
	return queue[i];
}

/*
 * Printing code
 *
 */



void printFinishOpenJob(int id, int s_time, int e_time, int priority, int jremoved, int jinbuf ){
	printf("Thread %d removes : JOB ID = %d, re-start time = %d, End time = %d, Priority = %d(jobs removed from buffer = %d, jobs in buffer = %d)\n", consumerID, id, s_time, e_time, priority, jremoved, jinbuf);
}

void printFinishedNewJob(int id, int s_time, int e_time, int priority , int jremoved, int jinbuf){
	printf("Thread %d removes : JOB ID = %d, Start time = %d, End time = %d, Priority = %d(jobs removed from buffer = %d, jobs in buffer = %d)\n",consumerID, id, s_time, e_time, priority,  jremoved, jinbuf);
}

void printRestartJob(int id, int rs_time, int rem_time, int priority, int jremoved, int jinbuf){
	printf("Thread %d : JOB ID = %d, re-start time = %d, Remaining time = %d, Priority = %d(jobs removed from buffer = %d, jobs in buffer = %d)\n",consumerID, id, rs_time, rem_time, priority,  jremoved, jinbuf);
}

void printProgressJob(int id, int s_time, int rem_time, int priority, int jremoved, int jinbuf){
	printf("Thread %d : JOB ID = %d, Start time = %d, Remaining time = %d, Priority = %d(jobs removed from buffer = %d, jobs in buffer = %d)\n", consumerID, id, s_time, rem_time, priority, jremoved, jinbuf);
}

void printJobProduced(int iId, int iBurstTime, int iRemainingTime, int priority)
{
	printf("Producing: Id = %d, Burst Time = %d, Remaining Time = %d, Priority = %d (jobs produced = %d, jobs in buffer: %d)\n", iId, iBurstTime, iRemainingTime, priority, jobsProduced, currentQueueCount+1);

}

void printJobDetails(int iId, int iBurstTime, int iRemainingTime, int priority)
{
	printf(" Id = %d, Burst Time = %d, Remaining Time = %d, Priority = %d\n",
			iId, iBurstTime, iRemainingTime, priority);
}

void printFinishedJob(int id, int b_time, int s_time ){
	/*printf("JOB ID = %d, Start time = %d, End time = %d ", id, s_time, e_time);*/
	printf("Thread %d removes: JOB ID = %d, Burst time = %d Start time = %d (jobs removed from buffer = %d, jobs in buffer = %d)\n",
			consumerID, id, b_time, s_time, jobsRemoved, currentQueueCount);
}

int numWithPriority;
int priQ;

void priorityq(int * job){
	if (job[PRIORITY] <= priQ){
		numWithPriority++;
	}
}

int getNumItemsWithPriority(int query){
	numWithPriority = 0;
	priQ = query;
	forEachInQueue(priorityq);
	return numWithPriority;
}
// All purpose print function that selects the type of print appropriate for each job during execurion
void printProgres(int *job, int startTime, int newJob, int delta_runningTime, int jobFinished){

	if (jobFinished == 1)
	{
		if(newJob == 1){
			printFinishedNewJob(job[JOB_INDEX], startTime, threadTimes[consumerID]+delta_runningTime, job[PRIORITY], job[JOBS_REMOVED]+1, job[JOBS_INBUF]-1);
		} else {
			printFinishOpenJob(job[JOB_INDEX], startTime, threadTimes[consumerID]+delta_runningTime, job[PRIORITY], job[JOBS_REMOVED]+1, job[JOBS_INBUF]-1); //The reason for decrementing and adding 1 to the buffer printout is because, in order to print out jobs in the right order, the print statement runs before the simulation happens so the values have to be changed apprehensivley
		}
	}
	else if (newJob == 1)
	{
		printProgressJob(job[JOB_INDEX], startTime, job[REMAINING_TIME]+delta_runningTime, job[PRIORITY], job[JOBS_REMOVED], job[JOBS_INBUF]);
	}
	else 
	{
		printRestartJob(job[JOB_INDEX], startTime, job[REMAINING_TIME]+delta_runningTime, job[PRIORITY], job[JOBS_REMOVED], job[JOBS_INBUF]);
	}
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
	/*printOutJob(enqueue());*/
	generateAndEnqueue();
}

void * producer(void *arg) {
	int i;
	/*for(i=0;i<NUMBER_OF_JOBS; i++){*/
	while (jobsProduced != NUMBER_OF_JOBS){
		sem_wait(&sSync);
		/*printNum();*/
		if (getQueueSize() >= 1){
			sem_post(&sDelay_Consumer);
		} 
		if (isFull() != 1) {
			producerWork();
		}
		sem_post(&sSync);
		if (isFull() == 1){
			sem_wait(&sDelay_Producer);
		}
	}
	finishedProducing = 1;
	sem_post(&sDelay_Consumer);
	return NULL;
}



void printSem(sem_t * sem) {
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

void consumerCriticalSectionWork(int * job){
	if (getNumItemsWithPriority(job[PRIORITY]) > 1){
		if (job[REMAINING_TIME] > TIME_SLICE){
			moveFrontToBackOfQueue(-TIME_SLICE);
		} else {
			dequeue();
			jobsRemoved++;
		}
	} else{
		dequeue();
		jobsRemoved++;
	}
}

void consumerWork(int* job){
	runJob(job);

}



void * consumer(void*  consumerIDarg_ptr){

	int * consumerIDarg = consumerIDarg_ptr;
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
				lastProduced = 1;//Set a flag if the consumer is removing from a full queue so we can wake the producer when it's no longer full
			}
			if (getQueueSize() > 0){ // Only dequeue if buffer not empty
				job = getTopOfQueue();
				cloneRow(jobStore, job); //Store a temporary copy of the job as it may be overwritten/moved in the queue once the critical section is left	
				jobStore[JOBS_REMOVED] = jobsRemoved;
				jobStore[JOBS_INBUF] = currentQueueCount;
				consumerCriticalSectionWork(jobStore);
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
		if (temp <=0){
			if (finishedProducing ==1){
				int sval;
				sem_getvalue(&sDelay_Consumer, &sval);
				if (sval<=0){
					sem_post(&sDelay_Consumer); //Wake up the next sleeping consumer so it can terminate
				}
				return NULL; // If buffer empty & producer finished, exit thread

			}	
			sem_wait(&sDelay_Consumer);
		}
	}
	return NULL;

	}


	void startConsumers(){
		int i;
		pthread_t consumerThreads[NUMBER_OF_CONSUMERS];
		for(i=0; i<NUMBER_OF_CONSUMERS; i++){
			pthread_create(&consumerThreads[i], NULL, consumer, i);
		}
		for(i=0; i<NUMBER_OF_CONSUMERS; i++){
			pthread_join(consumerThreads[i], NULL);
		}

	}

	void setupSemaphores(){
		jobsProduced = 0;
		jobsRemoved = 0;
		sem_init(&sSync, 1, 1);
		sem_init(&sDelay_Consumer, 1,0);
		sem_init(&sDelay_Producer, 1,0);

		pthread_t p1;
		pthread_create(&p1, NULL, producer, NULL);

		startConsumers(); 
		pthread_join(p1, NULL);

	}



	int main(int argc, char *argv[]) {
		if (NUMBER_OF_CONSUMERS == 0 || NUMBER_OF_JOBS ==0){
			printf("Invalid prams\n");
			return 0;
		}
		initQueue();
		setupSemaphores();
		printSems();
		printf("Average Start Time: %f\n", getArrayAvg(startTime, NUMBER_OF_JOBS) );
	}
