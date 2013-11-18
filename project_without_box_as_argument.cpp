/*
 *      1st Project
 *      by Paraskevas Lagakis
 *
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>	
#include <string.h>
#include <pthread.h>
#include <unistd.h>  

#define NUM_OF_POINTS 5000 //2^24
//#define NUM_OF_BOXES 10000
#define NUM_OF_THREADS 16

//structure with box info
typedef struct Box {
 	int level, boxid, parent, child[8], n, start, colleague[26], isThread;
 	float center[3];
}Box;

//the return struct of 0<n<=8
typedef struct returnSt {
	int level, n;
	double *start; //starting address of points in B for that particular box
}returnSt;

//helpful functions
void calculatePoints();
int calculateLimitsAndFindPoints(Box *myBox);

void *runBox(void *arg);
void *init(void *arg);

void lockBoxIdCounter();
void unlockBoxIdCounter();
void lockActiveThreads();
void unlockActiveThreads();
void lockForResize();
void unlockAfterResize();

Box **box;	 	//array of all boxes
//Box *leaf;	//array of non-null leaf boxes

//mutex for number of threads
pthread_mutex_t *threadNumMut;
pthread_cond_t *canCheckThreads;
int activeThreads=0,cantCheckThreads=0,sumOfThreadsUsed=0;

//mutex for number of boxes
pthread_mutex_t *boxNumMut;
pthread_cond_t *available;
int notAvailable=0;

//mutex for the resizing of arrays (must pe in critical path)
pthread_mutex_t *resizeMut;
pthread_cond_t *resizeDone;
int resizing=0;

//arrays of points
double A[NUM_OF_POINTS][3], B[NUM_OF_POINTS][3];
int boxIdCounter;

int main() {

	//initializations
	boxNumMut = new pthread_mutex_t[1];
  	pthread_mutex_init(boxNumMut, NULL);
	available = new pthread_cond_t[1];
  	pthread_cond_init (available, NULL);
	
	threadNumMut = new pthread_mutex_t[1];
  	pthread_mutex_init(threadNumMut, NULL);
	canCheckThreads = new pthread_cond_t[1];
	pthread_cond_init (canCheckThreads, NULL);

	resizeMut = new pthread_mutex_t[1];
  	pthread_mutex_init(resizeMut, NULL);
	resizeDone = new pthread_cond_t[1];
	pthread_cond_init (resizeDone, NULL);


	//calculate the points
	calculatePoints();

	pthread_t firstThread;
  	pthread_create(&firstThread, NULL, init, NULL);
	pthread_join(firstThread,NULL);	
	printf("first thread finished. Summary of threads: %i\n",sumOfThreadsUsed);

	//pthread_exit(NULL);
	int i;
	for(i=0;i<boxIdCounter;i++)
		printf("Box %i: level = %i, center=%G,%G,%G, n=%i, isThread=%i\n",
			box[i]->boxid,box[i]->level,box[i]->center[0],box[i]->center[1],
			box[i]->center[2],box[i]->n,box[i]->isThread);
	delete [] box;
	
}

void *init(void *arg) {

	boxIdCounter=1;

	box = (Box **)malloc(1*sizeof(Box *));

	//init first box
	box[0]->level=0;
	box[0]->boxid=1;
	box[0]->center[0]=0.5;
	box[0]->center[1]=0.5;
	box[0]->center[2]=0.5;
	box[0]->isThread=1;
	
	pthread_t nextThread;
  	pthread_create(&nextThread, NULL, runBox, box[0]);
	//runBox(&box[0]);
	
	usleep(100000000);
	usleep(100000000);

}

void *runBox(void *arg) {
	
	int i,hasThread[8];
	Box *myBox=(Box *)arg; //typecasting the argument, which is the actual box
	Box children[8];
	pthread_t nextThread[8];
				
	myBox->n=calculateLimitsAndFindPoints(myBox);
	if(myBox->boxid%1000==0)
		printf("Box %i: level = %i, center=%G,%G,%G, n=%i, isThread=%i, sum of Threads=%i\n",
			myBox->boxid,myBox->level,myBox->center[0],myBox->center[1],
			myBox->center[2],myBox->n,myBox->isThread,sumOfThreadsUsed);
	
	if(myBox->n==0) {
		myBox->boxid=0;
		return 0;
	}
	else if(myBox->n<=20) {
		return 0;
	}
	else {
		//create binary represantation of i, to find the centers of the 8 subboxes
		int bin0,bin1,bin2,tempCounter,tempThreadNum;

		for(i=0;i<8;i++) {
			
			//wait your turn to access boxIdCounter
			lockBoxIdCounter();

			//now do your thing
			boxIdCounter++;
			bin0=i%2;
			bin1=(i/2)%2;
			bin2=i/4;
			if(bin0==0) bin0=(-1);
			if(bin1==0) bin1=(-1);
			if(bin2==0) bin2=(-1);
						
			myBox->child[i]=boxIdCounter;

			children[i].level=myBox->level+1;
			children[i].boxid=boxIdCounter;
			children[i].center[0]=myBox->center[0]+(1/pow(2,(myBox->level+1)))*bin0*0.5;
			children[i].center[1]=myBox->center[1]+(1/pow(2,(myBox->level+1)))*bin1*0.5;
			children[i].center[2]=myBox->center[2]+(1/pow(2,(myBox->level+1)))*bin2*0.5;
			
			lockForResize();
			try {
				box=(Box **)realloc(box,boxIdCounter*sizeof(Box *));
			}
			catch(int x) {
				printf("Exception occured\n");
			}
			unlockAfterResize();

			//keep the value you need before you unlock access to boxIdCounter
			tempCounter=boxIdCounter-1;
			
			//unlock boxIdCounter
			unlockBoxIdCounter();

			box[tempCounter]=&children[i];
			//wait your turn to accesss activeThreads
			lockActiveThreads();

			//now do your thing
			if(activeThreads<NUM_OF_THREADS) {
				activeThreads++;
				sumOfThreadsUsed++;
				tempThreadNum=activeThreads;
				
				//unlock activeThreads
				unlockActiveThreads();

				box[tempCounter]->isThread=1;
				
				pthread_create(&nextThread[i], NULL, runBox, box[tempCounter]);
				hasThread[i]=1;
			}
			else{
				//unlock activeThreads
				unlockActiveThreads();
				hasThread[i]=0;
				//run the recursive version
				box[tempCounter]->isThread=0;
				runBox(box[tempCounter]);
			}
		}
	}
	
	if(myBox->isThread) {
		lockActiveThreads();
		activeThreads--;
		unlockActiveThreads();
	}
	for(i=0;i<8;i++) {
		if(hasThread[i]) 
			pthread_join(nextThread[i],NULL);
	}
}

//============================================================================
//just some helpful functions

void calculatePoints() {
	//generate random solutions to the problem
	double xsqr, ysqr, zsqr, x, y, z, xmean=0, ymean=0, zmean=0;
	long i;

	//init rand
	srand(0);

	for(i=0;i<NUM_OF_POINTS;i++) {
		
		//find x
		xsqr = ((double)rand() / (double)RAND_MAX);
		x=sqrt(xsqr);
		
		//find y
		ysqr = ((double)rand() / (double)RAND_MAX * (double)(1-xsqr));
		y=sqrt(ysqr);
		
		//find z
		zsqr= 1-xsqr-ysqr;
		z=sqrt(zsqr);
		
		A[i][0]=x; A[i][1]=y; A[i][2]=z;
	}
}

int calculateLimitsAndFindPoints(Box *myBox) {
	int n=0;
	long i;
	double xll=(myBox->center[0]-(1/pow(2,(myBox->level+1))));
	double xul=(myBox->center[0]+(1/pow(2,(myBox->level+1))));
	double yll=(myBox->center[1]-(1/pow(2,(myBox->level+1))));
	double yul=(myBox->center[1]+(1/pow(2,(myBox->level+1))));
	double zll=(myBox->center[2]-(1/pow(2,(myBox->level+1))));
	double zul=(myBox->center[2]+(1/pow(2,(myBox->level+1))));
	
	for(i=0;i<NUM_OF_POINTS;i++) {
			//evaluate if point is in the limits of the box
			if((A[i][0]>=xll)&&(A[i][0]<xul)
				&&(A[i][1]>=yll)&&(A[i][1]<yul)
				&&(A[i][2]>=zll)&&(A[i][2]<zul)) {
				n++; //increment the number of points
			}
		}

	return n;
}


void lockBoxIdCounter() {
	pthread_mutex_lock(boxNumMut);
	while(notAvailable)
		pthread_cond_wait(available,boxNumMut);
	notAvailable=1;
}

void unlockBoxIdCounter() {
	notAvailable=0;
	pthread_mutex_unlock(boxNumMut);
	pthread_cond_signal(available);
}

void lockActiveThreads() {
	pthread_mutex_lock(threadNumMut);
	while(cantCheckThreads)
		pthread_cond_wait(canCheckThreads,threadNumMut);
	cantCheckThreads=1;
}

void unlockActiveThreads() {
	cantCheckThreads=0;
	pthread_mutex_unlock(threadNumMut);
	pthread_cond_signal(canCheckThreads);
}

void lockForResize() {
	pthread_mutex_lock(resizeMut);
	while(resizing)
		pthread_cond_wait(resizeDone,resizeMut);
	resizing=1;
}

void unlockAfterResize() {
	resizing=0;
	pthread_mutex_unlock(resizeMut);
	pthread_cond_signal(resizeDone);
}
//==========================================================================

