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
#include <time.h>        
#include <sys/time.h>

#define NUM_OF_POINTS 200000 //2^24
#define NUM_OF_THREADS 32

//structure with box info
typedef struct Box {
 	int level, boxid, parent, child[8], n, start, colleague[26];
 	float center[3];
}Box;

struct timeval startwtime, endwtime; 

//helpful functions
void initMutAndCond();
void initFirstBox();
void calculatePoints();
void calculateLimitsAndFindPoints(Box *myBox);
void assignChildtoNewBox(int a,Box child);
long getTimestamp();

//the function that does all the work
void *runBox(void *arg);
void *runBoxParallel(void *arg);

//mutex for number of boxes
pthread_mutex_t *boxNumMut;
pthread_cond_t *available;
int notAvailable=0;

//mutex for number of threads
pthread_mutex_t *threadNumMut;
pthread_cond_t *canCheckThreads;
int cantCheckThreads=0;

int activeThreads=0;
long sumOfThreadsUsed=0;

//mutex for bCounter
pthread_mutex_t *bCountMut;
pthread_cond_t *canChangeBCount;
int cantChangeBCount=0;

void lockBoxIdCounter();
void unlockBoxIdCounter();
void lockActiveThreads();
void unlockActiveThreads();
void lockBCounter();
void unlockBCounter();

Box** box; 		//array of all boxes
//Box *leaf;	//array of non-null leaf boxes

double A[NUM_OF_POINTS][3], B[NUM_OF_POINTS][3];
long boxIdCounter, bCounter;

int main() {

	int i;

	//init mutexes and condition variables
	initMutAndCond();

	//calculate the points
	calculatePoints();

	//allocate memory for first box
	box = (Box**) malloc(sizeof(Box*));  
	box[0] = (Box*) malloc(sizeof(Box));

	//init first box
	initFirstBox();
	
	boxIdCounter=1; bCounter=0;

	//get first timestamp
	long starting=getTimestamp();

	runBox(box[0]);
	
	printf("\n=======================================\n\n");
	for(i=0;i<boxIdCounter;i++)
	if(box[i]->boxid%1000==0)	
		printf("Box %i: level=%i, center=%G,%G,%G, n=%i\n",
			box[i]->boxid,box[i]->level,box[i]->center[0],box[i]->center[1],
			box[i]->center[2],box[i]->n);
	
	long finishing=getTimestamp();
	printf("Total time: %lu, threads used:%lu\n",finishing-starting,sumOfThreadsUsed);
}

void *runBoxParallel(void *arg) {

	long i;
	
	Box *myBox=(Box *)arg; //typecasting the argument, which is the actual box
	Box children[8];
	pthread_t threads[8];
	int isThread[8];

	calculateLimitsAndFindPoints(myBox);
	// if(myBox->boxid%10000==0)	
	// 	printf("Box %i: level=%i, center=%G,%G,%G, n=%i, active threads=%i\n",
	// 		myBox->boxid,myBox->level,myBox->center[0],myBox->center[1],
	// 		myBox->center[2],myBox->n,activeThreads);
	
	if(myBox->n==0) {
		lockActiveThreads();
		activeThreads--;
		unlockActiveThreads();
		return 0;
	}
	else if(myBox->n<=20) {
		lockActiveThreads();
		activeThreads--;
		unlockActiveThreads();
		return 0;
	}
	else {
		//create binary represantation of i, to find the centers of the 8 subboxes
		int bin0,bin1,bin2;

		for(i=0;i<8;i++) {
			//here we need the exclusive access to boxIdCounter
			lockBoxIdCounter();

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
			
			box = (Box**) realloc(box,boxIdCounter*sizeof(Box*));
			int tempCounter=boxIdCounter-1;
			
			box[tempCounter]=(Box*) malloc(sizeof(Box));
			assignChildtoNewBox(tempCounter,children[i]);

			//we are done with boxIdCounter
			unlockBoxIdCounter();

			lockActiveThreads();			
			
			if(activeThreads<NUM_OF_THREADS) {
				activeThreads++;
				sumOfThreadsUsed++;
				unlockActiveThreads();
				isThread[i]=1;
				pthread_create(&threads[i], NULL, runBoxParallel,(void *)box[tempCounter]);
			}
			else{
				unlockActiveThreads();
				isThread[i]=0;
				runBox(box[tempCounter]);
			}
		}

		lockActiveThreads();
		activeThreads--;
		unlockActiveThreads();

		for(i=0;i<8;i++) {
			if(isThread[i]) {
				pthread_join(threads[i],NULL);	
			}
		}
	}
}

void *runBox(void *arg) {

	long i;
	
	Box *myBox=(Box *)arg; //typecasting the argument, which is the actual box
	Box children[8];
	pthread_t threads[8];
	int isThread[8];

	calculateLimitsAndFindPoints(myBox);
	//if(myBox->boxid%10000==0)	
		// printf("Box %i: level=%i, center=%G,%G,%G, n=%i, active threads=%i\n",
		// 	myBox->boxid,myBox->level,myBox->center[0],myBox->center[1],
		// 	myBox->center[2],myBox->n,activeThreads);
	
	if(myBox->n==0) {
		return 0;
	}
	else if(myBox->n<=20) {
		return 0;
	}
	else {
		//create binary represantation of i, to find the centers of the 8 subboxes
		int bin0,bin1,bin2;

		for(i=0;i<8;i++) {
			//here we need the exclusive access to boxIdCounter
			lockBoxIdCounter();

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
			
			box = (Box**) realloc(box,boxIdCounter*sizeof(Box*));
			int tempCounter=boxIdCounter-1;
			
			box[tempCounter]=(Box*) malloc(sizeof(Box));
			assignChildtoNewBox(tempCounter,children[i]);

			//we are done with boxIdCounter
			unlockBoxIdCounter();

			lockActiveThreads();			
			
			if(activeThreads<NUM_OF_THREADS) {
				activeThreads++;
				sumOfThreadsUsed++;
				unlockActiveThreads();
				isThread[i]=1;
				pthread_create(&threads[i], NULL, runBoxParallel,(void *)box[tempCounter]);
			}
			else{
				unlockActiveThreads();
				isThread[i]=0;
				runBox(box[tempCounter]);
			}
		}

		for(i=0;i<8;i++) {
			if(isThread[i]) {
				pthread_join(threads[i],NULL);	
			}
		}
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

void calculateLimitsAndFindPoints(Box *myBox) {
	myBox->n=0;
	long i;
	double tempArray[20][3];
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
			if(myBox->n<20) {
				tempArray[myBox->n][0]=A[i][0];
				tempArray[myBox->n][1]=A[i][1];
				tempArray[myBox->n][2]=A[i][2];
			}
		myBox->n++; //increment the number of points
		}
	}
	if(myBox->n>0&&myBox->n<=20) {
		//here we need exclusive access to bCounter
		lockBCounter();

		myBox->start=bCounter;
		//printf("n=%i so fill B array from bCounter=%i\n",myBox->n,bCounter);
		for(i=0;i<myBox->n;i++) {
		 	B[bCounter][0]=tempArray[i][0];
			B[bCounter][1]=tempArray[i][1];
			B[bCounter][2]=tempArray[i][2];
			
			//printf("B[%i]=%G,%G,%G\n",bCounter,B[bCounter][0],B[bCounter][1],B[bCounter][2]);
			bCounter++;
		}

		//we are done with bCounter
		unlockBCounter();
	}
}

void initMutAndCond() {
	boxNumMut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init(boxNumMut, NULL);

	threadNumMut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init(threadNumMut, NULL);
	
	bCountMut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init(bCountMut, NULL);

	//init condition variables
	available = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  	pthread_cond_init (available, NULL);
  
  	canCheckThreads = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  	pthread_cond_init (canCheckThreads, NULL);
  
  	canChangeBCount = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  	pthread_cond_init (canChangeBCount, NULL);
}

void initFirstBox() {
	box[0]->level=0;	
	box[0]->boxid=1;
	box[0]->center[0]=0.5;
	box[0]->center[1]=0.5;
	box[0]->center[2]=0.5;
}

void assignChildtoNewBox(int a,Box child) {
	box[a]->boxid=child.boxid;
	box[a]->level=child.level;
	box[a]->center[0]=child.center[0];
	box[a]->center[1]=child.center[1];
	box[a]->center[2]=child.center[2];
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

void lockBCounter() {
	pthread_mutex_lock(bCountMut);
	while(cantChangeBCount)
		pthread_cond_wait(canChangeBCount,bCountMut);
	cantChangeBCount=1;	
}

void unlockBCounter() {
	cantChangeBCount=0;
	pthread_mutex_unlock(bCountMut);
	pthread_cond_signal(canChangeBCount);
}

long getTimestamp(){

  gettimeofday(&endwtime, NULL);

  return((double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
                  + endwtime.tv_sec - startwtime.tv_sec)*1000);
}
//==========================================================================

//threads
// pthread_t Threads[NUM_OF_THREADS];
// void *exec (void *arg);
