#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>


#define NUMBER_OF_JOBS 1000
#define JOB_INDEX 0
#define BURST_TIME 1
#define REMAINING_TIME 2
#define PRIORITY 3
#define TIME_SLICE 25

int aiJobs[NUMBER_OF_JOBS][4];
int cur_time;
int startTime[NUMBER_OF_JOBS];
int finishTime[NUMBER_OF_JOBS];

void generateJobs()
{
	int i;
	for(i = 0; i < NUMBER_OF_JOBS;i++)
	{
		aiJobs[i][JOB_INDEX] = i;
		aiJobs[i][BURST_TIME] = rand() % 100;
		aiJobs[i][REMAINING_TIME] = aiJobs[i][BURST_TIME];
		aiJobs[i][PRIORITY] = rand()%10;
	}
}


void printJob(int iId, int iBurstTime, int iRemainingTime, int iPriority)
{
	printf("Id = %d, Burst Time = %d, Remaining Time = %d, Priority = %d\n", iId, iBurstTime, iRemainingTime, iPriority);
}

void printFinishOpenJob(int id, int s_time, int e_time, int priority ){
	printf("JOB ID = %d, re-start time = %d, End time = %d, Priority = %d\n", id, s_time, e_time, priority);
}

void printFinishedNewJob(int id, int s_time, int e_time, int priority ){
	printf("JOB ID = %d, Start time = %d, End time = %d, Priority = %d\n", id, s_time, e_time, priority);
}

void printRestartJob(int id, int rs_time, int rem_time, int priority){
	printf("JOB ID = %d, re-start time = %d, Remaining time = %d, Priority = %d\n", id, rs_time, rem_time, priority);
}

void printProgressJob(int id, int s_time, int rem_time, int priority){
	printf("JOB ID = %d, Start time = %d, Remaining time = %d, Priority = %d\n", id, s_time, rem_time, priority);
}


// All purpose print function that selects the type of print appropriate for each job during execurion
void printProgres(int *job, int startTime, int iTime, int newJob){
	if (job[REMAINING_TIME] == 0)
	{
		if(newJob == 1){
			printFinishedNewJob(job[JOB_INDEX], startTime, cur_time, job[PRIORITY]);
		} else {
			printFinishOpenJob(job[JOB_INDEX], startTime, cur_time, job[PRIORITY]);//TODO print new/old job finishs differently
		}
	}
	else if (job[REMAINING_TIME]+iTime == job[BURST_TIME])
	{
		printProgressJob(job[JOB_INDEX], startTime, job[REMAINING_TIME], job[PRIORITY]);
	}
	else 
	{
		printRestartJob(job[JOB_INDEX], startTime, job[REMAINING_TIME], job[PRIORITY]);
	}
}


void printJobs()
{
	int i;
	printf("JOBS: \n");
	for(i = 0; i < NUMBER_OF_JOBS; i++)
		printJob(aiJobs[i][JOB_INDEX], aiJobs[i][BURST_TIME], aiJobs[i][REMAINING_TIME], aiJobs[i][PRIORITY]);
}

void swap(int *a, int *b){
	int temp = *a;
	*a=*b;
	*b=temp;
}

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

//sort the job list, pass in a pointer to the working job list. No need for a return value.
void bubbleSort(int list[][4], int size)
{
	int swapped = 0;
	int i;
	for (i = 0;  i < size; i++)
	{
		if (i<(size-1)) { // Don't swap the last entries with null
			if (list[i][PRIORITY] > list[i+1][PRIORITY]){
				swapRows(list[i], list[i+1]);
				swapped = 1; 
			}
		}
	}


	if (swapped == 1){ 
		bubbleSort(list, size);
	} else { return; }
}

long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end)
{
	int iSeconds = end.tv_sec - start.tv_sec;
	int iUSeconds = end.tv_usec - start.tv_usec;
	long int mtime = (iSeconds * 1000 + iUSeconds / 1000.0);
	return mtime;
}

//Returns the sum of a simple int arrray
double getArraySum(int *array, int length){
	double sum = 0;
	int i;
	for(i=0; i<length; i++ ){
		sum += array[i];
	}
	return sum;
}


//Returns the double average of an simple int array
double getArrayAvg(int *array, int length){
	return getArraySum(array, length)/length;	
}

//Subtract corrosponding elements from arrays. r1=a1-b1, r2=a2-b2. Return an array of values 
int* subTwoArrays(int * array1, int * array2){
	static int sum[NUMBER_OF_JOBS];
	int i;
	for (i=0; i<NUMBER_OF_JOBS; i++){
		sum[i] = array1[i] - array2[i]; 
	}	
	return sum;
}

double getAverageRuntime(){
	return getArrayAvg(subTwoArrays(finishTime, startTime), NUMBER_OF_JOBS);
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
	} while(iDifference < iTime);
	cur_time += iTime;
}
int getRunningtime(int index){
	return finishTime[index] - startTime[index]; 	  
}

void jobFinished(int index){
	finishTime[index] = cur_time;
}


void jobStarted(int index, int s_Time){
	startTime[index] = s_Time;
}

// Function that simulates running jobs, decrements their counter and registers when jobs are started and finished. 
// Makes use of functions jobStarted, jobFinished, simulateJob, printProgress documented elsewhere. 
void runJob(int iTime, int *job){
	if (job[REMAINING_TIME] >= iTime){
		int startTime = cur_time;
		int newJob = 0;
		if (job[REMAINING_TIME] == job[BURST_TIME]){
			newJob = 1;
			jobStarted(job[JOB_INDEX], startTime);
		}
		simulateJob(iTime);
		job[REMAINING_TIME] -= iTime;
		printProgres(job, startTime, iTime, newJob);	
		if(job[REMAINING_TIME] == 0){
			jobFinished(job[JOB_INDEX]);
		}
		else if (job[REMAINING_TIME] == job[BURST_TIME])
		{
			jobStarted(job[JOB_INDEX], startTime);
		}
	} else {
		runJob(job[REMAINING_TIME], job);
	}
}

// returns the sum of remaining time from the queue for all jobs with a priority value lower than the max parameter.
int isQueueEmpty(int max){
	int remainingTime = 0;
	int i;
	for (i=0; i<NUMBER_OF_JOBS; i++){
		if (aiJobs[i][PRIORITY] <= max && aiJobs[i] > 0){
			remainingTime += aiJobs[i][REMAINING_TIME]; 
		}
	}
	return remainingTime;
}

//Returns an int number representing the exact number of unfinished jobs with a priority equal to the priority parameter
int numberOfJobsWithPriority(int priority){
	int num = 0;
	int i;
	for (i =0; i<NUMBER_OF_JOBS; i++){
		if (aiJobs[i][PRIORITY] == priority && aiJobs[i][REMAINING_TIME] >0){
			num++;
		}
	}
	return num;
}


//Round robin scheduling code.
void rr_scheduler(){
	int prioritySelect = 0;
	while (isQueueEmpty(10) != 0){ // While there are jobs in queue
		while (isQueueEmpty(prioritySelect) != 0){ // while jobs in queue at cur priority level
			int i;
			for(i=0; i<NUMBER_OF_JOBS; i++){ //inc through queue
				if (aiJobs[i][PRIORITY] <= prioritySelect && aiJobs[i][REMAINING_TIME] != 0){ // slect unfinished jobs at priority level
					if (numberOfJobsWithPriority(prioritySelect) == 1){ // more than one job in queue implement time slice, else finish job
						runJob(aiJobs[i][REMAINING_TIME], aiJobs[i]);
					} else {
						runJob(TIME_SLICE, aiJobs[i]);
					}
				}
			}
		}
		prioritySelect++; //Once completing all jobs at a given priority level, increment the targeted level
	}
	return;
}


// Invokes the round robin scheduler and prints detail to log
void roundRobin(){
	printf("\n\nRound Robin; time slice, %d\n", TIME_SLICE);
	rr_scheduler();
	printf("Average Response Time: %f\n", getArrayAvg(startTime, NUMBER_OF_JOBS) );
	printf("Average Runtime:%f\n", getAverageRuntime());
}


int main()
{
	generateJobs();
	printJobs();
	bubbleSort(aiJobs, NUMBER_OF_JOBS);
	printf("Sorting jobs by priority \n");
	printJobs();
	roundRobin();
	/*printJobs();*/
	return 0;
}

