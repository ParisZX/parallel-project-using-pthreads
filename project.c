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

#define NUM_OF_POINTS 1000 //2^24
#define NUM_OF_THREADS 10000

//structure with box info
typedef struct Box {
 	int level, boxid, parent, child[8], n, start, colleague[26];
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
void initFirstBox();
void assignChildtoNewBox(int a,Box child);

//mutex for number of threads
pthread_mutex_t *threadNumMut;
pthread_cond_t *canCheckThreads;
int activeThreads=0,cantCheckThreads=0,sumOfThreadsUsed=0;

//mutex for number of boxes
pthread_mutex_t *boxNumMut;
pthread_cond_t *available;
int notAvailable=0;

void lockBoxIdCounter();
void unlockBoxIdCounter();
void lockActiveThreads();
void unlockActiveThreads();

Box** box; 	//array of all boxes
//Box *leaf;	//array of non-null leaf boxes

double A[NUM_OF_POINTS][3], B[NUM_OF_POINTS][3];
int boxIdCounter;

int main() {

	int i;

	//calculate the points
	calculatePoints();
	
	//allocate memory for first box
	box = (Box**) malloc(1*sizeof(Box*));  
	box[0] = (Box*) malloc(1*sizeof(Box));

	//init first box
	initFirstBox();
	
	boxIdCounter=1;

	runBox(box[0]);
	
	printf("\n=======================================\n\n");
	for(i=0;i<boxIdCounter;i++)
	printf("Box %i: level=%i, center=%G,%G,%G, n=%i\n",
			box[i]->boxid,box[i]->level,box[i]->center[0],box[i]->center[1],
			box[i]->center[2],box[i]->n);
}

void *runBox(void *arg) {

	long i;
	double *start,xLowerLim,xUpperLim,yLowerLim,yUpperLim,zLowerLim,zUpperLim;
	Box *myBox=(Box *)arg; //typecasting the argument, which is the actual box
	Box children[8];

	myBox->n=calculateLimitsAndFindPoints(myBox);
		
	printf("Box %i: level=%i, center=%G,%G,%G, n=%i\n",
			myBox->boxid,myBox->level,myBox->center[0],myBox->center[1],
			myBox->center[2],myBox->n);
	
	if(myBox->n==0) {
		//myBox->boxid=0;
		return 0;
	}
	else if(myBox->n<=20) {
		return 0;
	}
	else {
		//create binary represantation of i, to find the centers of the 8 subboxes
		int bin0,bin1,bin2;

		for(i=0;i<8;i++) {
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
			box[boxIdCounter-1]=(Box*) malloc(1*sizeof(Box));
			
			assignChildtoNewBox(boxIdCounter-1,children[i]);
			
			runBox(box[boxIdCounter-1]);
			
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


//==========================================================================

//threads
// pthread_t Threads[NUM_OF_THREADS];
// void *exec (void *arg);

// //MUTEX
// pthread_mutex_t mut;
// pthread_cond_t available;
// int unavailable=0;

