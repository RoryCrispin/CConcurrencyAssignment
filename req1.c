#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define NUMBER_OF_JOBS 1000
#define JOB_INDEX 0
#define BURST_TIME 1
#define REMAINING_TIME 2
#define PRIORITY 3

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

void printFinishedJob(int id, int s_time, int e_time ){
	printf("JOB ID = %d, Start time = %d, End time = %d\n", id, s_time, e_time);
}


void printJobs()
{
	int i;
	printf("JOBS: \n");
	for(i = 0; i < NUMBER_OF_JOBS; i++)
		printJob(aiJobs[i][JOB_INDEX], aiJobs[i][BURST_TIME], aiJobs[i][REMAINING_TIME], aiJobs[i][PRIORITY]);
}

long int getDifferenceInMilliSeconds(struct timeval start, struct timeval end)
{
	int iSeconds = end.tv_sec - start.tv_sec;
	int iUSeconds = end.tv_usec - start.tv_usec;
 	long int mtime = (iSeconds * 1000 + iUSeconds / 1000.0);
	return mtime;
}

//Code for calculating average running and response times from the arrays that store 
//the start and end time of each job 


//Returns the sum of a simple int arrray
double getArraySum(int *array, int length){
	double sum = 0;
	int i;
	for(i=0; i<length; i++ ){
		sum += array[i];
	}
	return sum;
}

//Returns the average of an simple int array
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

void jobFinished(int index){
	finishTime[index] = cur_time;
}

void jobStarted(int index, int s_Time){
	startTime[index] = s_Time;
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


// Function that simulates running jobs, decrements their counter and registers when jobs are started and finished. 
void runJob(int *job){
    int startTime = cur_time;
    jobStarted(job[JOB_INDEX], startTime);
    simulateJob(job[REMAINING_TIME]);
    job[REMAINING_TIME] = 0;
    printFinishedJob(job[JOB_INDEX], startTime, cur_time);
    jobFinished(job[JOB_INDEX]);
}
void FCFS(){
	printf("\n\nFCFS:\n");
	int i;
	for(i=0; i< NUMBER_OF_JOBS; i++){
		runJob( aiJobs[i]);
	}
	printf("Average Response Time: %f\n", getArrayAvg(startTime, NUMBER_OF_JOBS) );
	printf("Average Runtime:%f\n", getAverageRuntime());
}

int main()
{
    if (NUMBER_OF_JOBS <1){
        printf("Invalid params\n");
        return 0;
    }
	generateJobs();
	printJobs();
	FCFS();

	return 0;
}

