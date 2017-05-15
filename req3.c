#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define QUEUE_SIZE 20
#define NUMBER_OF_JOBS 1000

#define emptyChar ' '

sem_t sSync;
sem_t sDelay_Consumer;
sem_t sDelay_Producer;

int finishedProducing;

/*
 * -- QUEUE --
 *
 * Code relating to managing the queue
 * much of this code was copied through from req4.c
 * I originally write this as a single file and included
 * queue.h in req4,5 and 6 but was to>d not to for submission 
 *
 */


int fronti;
int reari;
int qSize;



char queue[QUEUE_SIZE];

//returns the item at a specific index in the queue
int getIndex(int loc) {
    return ((loc+1)%QUEUE_SIZE);
}

void initQueue(){
    fronti = 0;
    reari = 0;
    qSize = 0;
}

int isEmpty(){
    return fronti==reari;
}

int isFull(){
    return qSize == QUEUE_SIZE;
}

int enqueue(char item){
    if (isFull() ==1){
        printf("Queue Full!\n");
        return -1;
    }
    fronti++;
    queue[getIndex(fronti)] = item;
    qSize++;
    return 0;
}

int getQueueSize(){
    return qSize;
}


int dequeue(){

    reari++;
    int item = queue[getIndex(reari)];
    qSize --;
    return item;
}

void printQueue(){
    int temp = fronti;
    while (temp!=reari){
        printf("%c", queue[getIndex(temp)]);
        temp--;
    }
    printf("\n");
}


void printNum(){
    printf("iIndex = %d 	", getQueueSize());
    printQueue();
}

/*
 * 
 * -- Poducer Consumer --
 * This code handles the producer/consumer problem. 
 * 
 */

void * producer(void *arg) {
    int i;
    for(i=0;i<NUMBER_OF_JOBS; i++){
        sem_wait(&sSync); //wait for access to the queue 
        enqueue('*');
        printNum();
        if (getQueueSize() == 1){
            sem_post(&sDelay_Consumer); //Wake up consumer if just produced a new item when queue was empty
        }
        sem_post(&sSync);
        if (getQueueSize() == QUEUE_SIZE){
            sem_wait(&sDelay_Producer);//wait if the queue is full
        }
    }
    finishedProducing = 1; // After N items produced, set finish producing flag and exit thread
    sem_post(&sDelay_Consumer);
    return NULL;
}

void printSem(sem_t * sem){
    int sval;
    sem_getvalue(sem, &sval);
    printf("%d", sval);
}


void * consumer(void *arg){
    int temp;
    sem_wait(&sDelay_Consumer);
    int lastProduced = 0;
    while(1){
        sem_wait(&sSync);
        if(getQueueSize() == QUEUE_SIZE) {
            lastProduced = 1; //Set a flag if the consumer is removing from a full queue so we can wake the producer when it's no longer full
        }
        dequeue();
        printNum();
        temp = getQueueSize();

        if (lastProduced == 1){
            sem_post(&sDelay_Producer);
            lastProduced = 0;
        }

        sem_post(&sSync);
        
        if (temp <=0){
            if (finishedProducing ==1) return NULL; // If buffer empty & producer finished, exit thread
            sem_wait(&sDelay_Consumer); //wait if the queue is empty 
        }

    }
    return NULL;

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



int main(int argc, char *argv[]) {
    initQueue();
    sem_init(&sSync, 1, 1);
    sem_init(&sDelay_Consumer, 1,0);
    sem_init(&sDelay_Producer, 1,0);

    pthread_t c1, p1;

    pthread_create(&c1, NULL, consumer, NULL);
    pthread_create(&p1, NULL, producer, NULL);

    pthread_join(p1, NULL);
    pthread_join(c1, NULL);

    printSems();
    return 0;

}
