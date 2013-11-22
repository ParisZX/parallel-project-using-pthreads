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

#define NUM_OF_POINTS 400000 //2^24
#define NUM_OF_THREADS 32

//structure with box info
typedef struct Box {
 	int level, boxid, parent, child[8], n, start, colleague[26];
 	double center[3], limits[3][2];
}Box;

// typedef struct LevelDetails {
// 	int threadid;
// 	int** box;		//first column will be the boxid, the second its n
// }LevelDetails;

struct timeval startwtime, endwtime; 

//helpful functions
void initFirstBox();
void calculatePoints();
void calculateLimits();
int checkPoint(int i, int boxid);
long getTimestamp();

//the function that does all the work
void *map(void *arg);
int reduce();

Box** box; 		//array of all boxes
//Box *leaf;	//array of non-null leaf boxes
int *thisLevelsBox,maxBoxes,**tempN;

double A[NUM_OF_POINTS][3], B[NUM_OF_POINTS][3];
long boxIdCounter, bCounter,howManyChecks;

int main() {

	int i; pthread_t threads[NUM_OF_THREADS];

	//calculate the points
	calculatePoints();

	//allocate memory for first box
	box = (Box**) malloc(sizeof(Box*));  
	box[0] = (Box*) malloc(sizeof(Box));
	thisLevelsBox = (int*) malloc(sizeof(int));
	thisLevelsBox[0]=0;	//this array will have the pointers to boxes, not their ids (id=pointer+1)
	maxBoxes=1;

	//init first box
	initFirstBox();
	
	boxIdCounter=1; howManyChecks=NUM_OF_POINTS/NUM_OF_THREADS+1;

	//get first timestamp
	long starting=getTimestamp();

	int threadId[NUM_OF_THREADS];

	calculateLimits(); //calculate Limits for 0 level (box[0])
	
	tempN = (int**) malloc(NUM_OF_THREADS*sizeof(int*));
	
	for(i=0;i<NUM_OF_THREADS;i++)
			tempN[i] = (int*) malloc(sizeof(int));

	int done=0,j;

	while(!done)
	{
		//do map
		for(i=0;i<NUM_OF_THREADS;i++) {
			threadId[i]=i+1;
			pthread_create(&threads[i], NULL, map, &threadId[i]);
		}
	
		for(i=0;i<NUM_OF_THREADS;i++) 
			pthread_join(threads[i], NULL);

		//afterwards, do reduce
		// for(i=0;i<NUM_OF_THREADS;i++) {
		// 	threadId[i]=i+1;
		// 	pthread_create(&threads[i], NULL, reduce, &threadId[i]);
		// }
	
		// for(i=0;i<NUM_OF_THREADS;i++) 
		// 	pthread_join(threads[i], NULL);
		
		done=reduce();	

		for(i=0;i<NUM_OF_THREADS;i++) 
			tempN[i] = (int*) realloc(tempN[i],maxBoxes*sizeof(int));
		printf("maxBoxes for this level=%i\n",maxBoxes);
		calculateLimits();
			
	}

	printf("\n=======================================\n\n");
	for(i=0;i<boxIdCounter;i++)
		if(box[i]->boxid%1000==0)
			printf("Box %i: level=%i, center=%G,%G,%G, n=%i\n",
			box[i]->boxid,box[i]->level,box[i]->center[0],box[i]->center[1],
			box[i]->center[2],box[i]->n);
	
	long finishing=getTimestamp();
	printf("Total time: %lu\n",finishing-starting);
}

void *map(void *arg) {

	int *threadIdAddr=(int *)arg, threadId=*threadIdAddr;
	
	long i,j,finish=howManyChecks*threadId,start=finish-howManyChecks;
	if(finish>NUM_OF_POINTS) {
		finish=NUM_OF_POINTS;
	}
	// printf("my id is: %i\n",threadId);
	
	for(i=0;i<maxBoxes;i++) {
		tempN[threadId-1][i]=0;
		
	}
	
	//printf("maxBoxes=%i, points are %lu\n",maxBoxes,finish-start);

	for(i=start;i<finish;i++) {
		for(j=0;j<maxBoxes;j++) {			
			if(checkPoint(i,thisLevelsBox[j]))
				tempN[threadId-1][j]++;

		}
	}
}

int reduce() {
	long i; int j,newMaxBoxes=0,done=1;

	for(i=0;i<maxBoxes;i++) {
		
		for(j=0;j<NUM_OF_THREADS;j++) 
			box[thisLevelsBox[i]]->n+=tempN[j][i];

		// printf("Box %i: level=%i, center=%G,%G,%G, n=%i\n",
		// 	box[0]->boxid,box[0]->level,box[0]->center[0],box[0]->center[1],
		// 	box[0]->center[2],box[0]->n);
		
		if(box[thisLevelsBox[i]]->n==0) {
			continue;
		}
		else if(box[thisLevelsBox[i]]->n<=20) {
			continue;
		}
		else {
			//a flag to check if we are done building the tree
			done=0;

			//this will give us info for the new level
			newMaxBoxes+=8;
			boxIdCounter+=8;
	
			box = (Box**) realloc(box,boxIdCounter*sizeof(Box*));
	
			int bin0,bin1,bin2;
			for(j=0;j<8;j++) {

				bin0=j%2;
				bin1=(j/2)%2;
				bin2=j/4;
				if(bin0==0) bin0=(-1);
				if(bin1==0) bin1=(-1);
				if(bin2==0) bin2=(-1);
				
				box[boxIdCounter-j-1] = (Box*) malloc(sizeof(Box));
				box[boxIdCounter-j-1]->boxid=boxIdCounter-j;
				box[boxIdCounter-j-1]->level=box[thisLevelsBox[i]]->level+1;
				box[boxIdCounter-j-1]->n=0;
				box[boxIdCounter-j-1]->center[0]=box[thisLevelsBox[i]]->center[0]+(1/pow(2,(box[thisLevelsBox[i]]->level+1)))*bin0*0.5;
				box[boxIdCounter-j-1]->center[1]=box[thisLevelsBox[i]]->center[1]+(1/pow(2,(box[thisLevelsBox[i]]->level+1)))*bin1*0.5;
				box[boxIdCounter-j-1]->center[2]=box[thisLevelsBox[i]]->center[2]+(1/pow(2,(box[thisLevelsBox[i]]->level+1)))*bin2*0.5;
			}
		}
	}
	//creation of next level
	maxBoxes=newMaxBoxes;
	
	thisLevelsBox = (int*) realloc(thisLevelsBox,maxBoxes*sizeof(int));
	for(i=0;i<maxBoxes;i++) {
		thisLevelsBox[i]=boxIdCounter-maxBoxes+i; 	//boxIdCounter starts at 1, and maxBoxes starts at 0,
													//so after resolving first (0) level, boxIdCounter=1+8=9
													//and maxBoxes=0+8=8, so 9-8=1 and 1+i for i=1...maxBoxes,
													//gives proper results.
	}
	return done;
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

void calculateLimits() {
	long i;

	for(i=0;i<maxBoxes;i++) {
		//x lower lim
		box[thisLevelsBox[i]]->limits[0][0]=(box[thisLevelsBox[i]]->center[0]-(1/pow(2,(box[thisLevelsBox[i]]->level+1))));
		//x upper lim
		box[thisLevelsBox[i]]->limits[0][1]=(box[thisLevelsBox[i]]->center[0]+(1/pow(2,(box[thisLevelsBox[i]]->level+1))));
		//y lower lim
		box[thisLevelsBox[i]]->limits[1][0]=(box[thisLevelsBox[i]]->center[1]-(1/pow(2,(box[thisLevelsBox[i]]->level+1))));
		//y upper lim
		box[thisLevelsBox[i]]->limits[1][1]=(box[thisLevelsBox[i]]->center[1]+(1/pow(2,(box[thisLevelsBox[i]]->level+1))));
		//z lower lim
		box[thisLevelsBox[i]]->limits[2][0]=(box[thisLevelsBox[i]]->center[2]-(1/pow(2,(box[thisLevelsBox[i]]->level+1))));
		//z upper lim
		box[thisLevelsBox[i]]->limits[2][1]=(box[thisLevelsBox[i]]->center[2]+(1/pow(2,(box[thisLevelsBox[i]]->level+1))));
	}	
}

int checkPoint(int i, int j) {
	//evaluate if point is in the limits of the box
	if((A[i][0]>=box[j]->limits[0][0])&&(A[i][0]<box[j]->limits[0][1])
		&&(A[i][1]>=box[j]->limits[1][0])&&(A[i][1]<box[j]->limits[1][1])
		&&(A[i][2]>=box[j]->limits[2][0])&&(A[i][2]<box[j]->limits[2][1])) 
			return 1; 
	
	return 0;
}

void initFirstBox() {
	box[0]->level=0;
	box[0]->n=0;	
	box[0]->boxid=1;
	box[0]->center[0]=0.5;
	box[0]->center[1]=0.5;
	box[0]->center[2]=0.5;
}

long getTimestamp(){

  gettimeofday(&endwtime, NULL);

  return((double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
                  + endwtime.tv_sec - startwtime.tv_sec)*1000);
}
