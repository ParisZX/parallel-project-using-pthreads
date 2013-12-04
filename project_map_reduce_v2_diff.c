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

#define NUM_OF_POINTS 16000000 //2^24
#define NUM_OF_THREADS 8

//structure with box info
typedef struct Box {
 	int level, boxid, parent, child[8], n, start, colleague[26];
 	double center[3], limits[3][2];
}Box;

//variables and arrays
Box** box;
double A[NUM_OF_POINTS][3], B[NUM_OF_POINTS][3];
long *parents,*newParents,nOfChild[8]; //parents array will be keeping the ids of boxes who are parents

//variables who act as pointers for the threads
long numOfParents,numOfNewParents,boxIdCounter;

//struct for timestamps
struct timeval startwtime, endwtime; 

//mutex for number of boxes
pthread_mutex_t *boxNumMut;
pthread_cond_t *available;
int notAvailable=0;

//mutex for number of parents
pthread_mutex_t *parNumMut;
pthread_cond_t *parDone;
int parNotDone=0;

//functions
void allocateMemForFirstBox();
void allocateMemForTempArray(void *arg);
void freeMemForTempArray(long parent,void *arg);

void initMutAndCond();
void lockBoxIdCounter();
void unlockBoxIdCounter();
void lockNewParents();
void unlockNewParents();

void initFirstBox();
void calculatePoints();
void calculateLimits();
void* map(void *arg);
void reduce(long parent,void *arg);
void initChildren(long newParentId);
long getTimestamp();

int main() {
	int done=0,j; long i;
	
	initMutAndCond();

	calculatePoints();

	numOfParents=1;	parents=(long*)malloc(sizeof(long)); newParents=(long*)malloc(sizeof(long));
	
	allocateMemForFirstBox();
	
	boxIdCounter=9; 

	//get first timestamp
	long starting=getTimestamp();
	long tempTime; int level=0;
	while(!done) {
		tempTime=getTimestamp();
		numOfNewParents=0;
		pthread_t threads[219645];	
		for(i=0;i<numOfParents;i++) 
			pthread_create(&threads[i], NULL, map, &parents[i]);
		
		for(i=0;i<numOfParents;i++) 
				pthread_join(threads[i], NULL);

		if(numOfNewParents==0)
			break;
		parents=(long*)realloc(parents,numOfNewParents*sizeof(long));
		for(i=0;i<numOfNewParents;i++){
			parents[i]=newParents[i];
		}
		numOfParents=numOfNewParents;
		printf("Time for lvl %i: %lu , boxIdCounter: %lu\n",level,getTimestamp()-tempTime,boxIdCounter);
		level++;
	}

	long finishing=getTimestamp();
	
	printf("\n=======================================\n\n");
	for(i=0;i<boxIdCounter;i++)
		if(box[i]->boxid%10000==0)
			printf("Box %i: level=%i, center=%G,%G,%G, n=%i\n",
			box[i]->boxid,box[i]->level,box[i]->center[0],box[i]->center[1],
			box[i]->center[2],box[i]->n);

	printf("Total time: %lu\n",finishing-starting);
}

void *map(void *arg) {
	
	long i,j,*parentAddr=(long *)arg, parent=*parentAddr; parent--; //"parents" array keeps the ids of the parents, we need the pointers, which are id-1
	double ***pointsOfChild;

	//allocateMemForTempArray(&pointsOfChild);
	pointsOfChild=(double***)malloc(8*sizeof(double**));
	//printf("217\n");
	for(j=0;j<8;j++) {
		pointsOfChild[j]=(double**)malloc(sizeof(double*));
		pointsOfChild[j][0]=(double*)malloc(3*sizeof(double));
	}
	//=========================================

	calculateLimits(parent);
	//printf("im in map\n");
	long childsId,start,finish; 
	start=box[parent]->start; finish=box[parent]->start+box[parent]->n;
	//printf("start...finish %lu....%lu, in boxes: %G %G %G , %G %G %G\n",start,finish-1,B[start][0],B[start][1],B[start][2],B[finish-1][0],B[finish-1][1],B[finish-1][2]);

	for(i=start;i<finish;i++) {
		//check which child each point belongs to
		for(j=0;j<8;j++) {
			childsId=box[parent]->child[j]-1;
			if( (B[i][0]>=box[childsId]->limits[0][0])&&(B[i][0]<box[childsId]->limits[0][1])&&
				(B[i][1]>=box[childsId]->limits[1][0])&&(B[i][1]<box[childsId]->limits[1][1])&&
				(B[i][2]>=box[childsId]->limits[2][0])&&(B[i][2]<box[childsId]->limits[2][1])) {
				pointsOfChild[j]=(double**)realloc(pointsOfChild[j],(box[childsId]->n+1)*sizeof(double*));
				pointsOfChild[j][box[childsId]->n]=(double*)malloc(3*sizeof(double));
				pointsOfChild[j][box[childsId]->n][0]=B[i][0];
				pointsOfChild[j][box[childsId]->n][1]=B[i][1];
				pointsOfChild[j][box[childsId]->n][2]=B[i][2];
				box[childsId]->n++; //inc the childs n	
			}
		}
	}
	

	long bCounter=box[parent]->start;

	for(i=0;i<8;i++) {
		
		childsId=box[parent]->child[i]-1;

		if(box[childsId]->n==0) {
			//child[i].boxid=0;
			printf("");
		}
		if(box[childsId]->n<=20) {
		//do all the things u need to do with B array,
		//and change bCounter.
			for(j=0;j<box[childsId]->n;j++) {
				B[bCounter][0]=pointsOfChild[i][j][0];
				B[bCounter][1]=pointsOfChild[i][j][1];
				B[bCounter][2]=pointsOfChild[i][j][2];
				bCounter++;
			}			
		}
		
		if(box[childsId]->n>20) {
			//printf("for Box %lu bCounter=%lu\n",childsId+1,bCounter);
			//child is gonna be parent. Do B changes.
			box[childsId]->start=bCounter;
			initChildren(childsId);

			lockNewParents();

			newParents[numOfNewParents]=childsId+1;
			numOfNewParents++;
			newParents=(long*)realloc(newParents,(numOfNewParents+1)*sizeof(long));
			
			unlockNewParents();

			for(j=0;j<box[childsId]->n;j++) {
				//printf("%lu\n",j);
				//printf("pointsOfChild[%i][%lu]=%G %G %G\n",i,j,pointsOfChild[i][j][0],pointsOfChild[i][j][1],pointsOfChild[i][j][2]);
				B[bCounter][0]=pointsOfChild[i][j][0];
				B[bCounter][1]=pointsOfChild[i][j][1];
				B[bCounter][2]=pointsOfChild[i][j][2];
				bCounter++;
			}
		}
	}

	
	//freeMemForTempArray(parent,&pointsOfChild);
	for(i=0;i<8;i++) {
		childsId=box[parent]->child[i]-1;
		for(j=0;j<box[childsId]->n;j++) {
			//for(k=0;k<3;k++)
				//free(pointsOfChild[i][j][k]);
			free(pointsOfChild[i][j]);
		}
		free(pointsOfChild[i]);
	}
	
	//============================================

}

void allocateMemForFirstBox() {
	int i,bin0,bin1,bin2;

	box = (Box**) malloc(9*sizeof(Box*));  
	box[0] = (Box*) malloc(sizeof(Box));
	initFirstBox();

	for(i=1;i<9;i++) {
		bin0=(i-1)%2;
		bin1=((i-1)/2)%2;
		bin2=(i-1)/4;

		if(bin0==0) bin0=(-1);
		if(bin1==0) bin1=(-1);
		if(bin2==0) bin2=(-1);

		box[i]=(Box*)malloc(sizeof(Box));
		box[i]->boxid=i+1;
		box[i]->level=1;
		box[i]->n=0;
		box[i]->parent=1;
		box[i]->center[0]=box[0]->center[0]+(1/pow(2,(box[0]->level+1)))*bin0*0.5;
		box[i]->center[1]=box[0]->center[1]+(1/pow(2,(box[0]->level+1)))*bin1*0.5;
		box[i]->center[2]=box[0]->center[2]+(1/pow(2,(box[0]->level+1)))*bin2*0.5;
	}
}

void initChildren(long newParentId) { 
	int i,bin0,bin1,bin2;
	lockBoxIdCounter();
	box = (Box**) realloc(box,(boxIdCounter+8)*sizeof(Box*));
	for(i=0;i<8;i++) {
		bin0=i%2;
		bin1=(i/2)%2;
		bin2=i/4;

		if(bin0==0) bin0=(-1);
		if(bin1==0) bin1=(-1);
		if(bin2==0) bin2=(-1);

		box[newParentId]->child[i]=boxIdCounter+1;
		box[boxIdCounter]=(Box*)malloc(sizeof(Box));
		box[boxIdCounter]->boxid=boxIdCounter+1;
		//printf("for Box[%lu] child %i = %i\n",newParentId,i,box[newParentId]->child[i]);
		box[boxIdCounter]->level=box[newParentId]->level+1;
		box[boxIdCounter]->n=0;
		box[boxIdCounter]->parent=newParentId+1;
		box[boxIdCounter]->center[0]=box[newParentId]->center[0]+(1/pow(2,(box[newParentId]->level+1)))*bin0*0.5;
		box[boxIdCounter]->center[1]=box[newParentId]->center[1]+(1/pow(2,(box[newParentId]->level+1)))*bin1*0.5;
		box[boxIdCounter]->center[2]=box[newParentId]->center[2]+(1/pow(2,(box[newParentId]->level+1)))*bin2*0.5;
		boxIdCounter++;
	}
	unlockBoxIdCounter();
}

void initFirstBox() {
	int i;

	box[0]->level=0;
	box[0]->n=NUM_OF_POINTS;	
	box[0]->boxid=1;
	box[0]->center[0]=0.5;
	box[0]->center[1]=0.5;
	box[0]->center[2]=0.5;
	box[0]->start=0;
	for(i=0;i<8;i++)
		box[0]->child[i]=i+2;
	parents[0]=1;
}

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
		B[i][0]=x; B[i][1]=y; B[i][2]=z;
		
	}
}

void calculateLimits(long parent) {
	//printf("Parent=%lu\n",parent);
	long i,childsId;
	for(i=0;i<8;i++) {
		childsId=(box[parent]->child[i])-1;
		//printf("child=%lu\n",childsId);
		//x lower lim
		box[childsId]->limits[0][0]=(box[childsId]->center[0]-(1/pow(2,(box[childsId]->level+1))));
		//x upper lim
		box[childsId]->limits[0][1]=(box[childsId]->center[0]+(1/pow(2,(box[childsId]->level+1))));
		//y lower lim
		box[childsId]->limits[1][0]=(box[childsId]->center[1]-(1/pow(2,(box[childsId]->level+1))));
		//y upper lim
		box[childsId]->limits[1][1]=(box[childsId]->center[1]+(1/pow(2,(box[childsId]->level+1))));
		//z lower lim
		box[childsId]->limits[2][0]=(box[childsId]->center[2]-(1/pow(2,(box[childsId]->level+1))));
		//z upper lim
		box[childsId]->limits[2][1]=(box[childsId]->center[2]+(1/pow(2,(box[childsId]->level+1))));
	}	
}

void initMutAndCond() {
	boxNumMut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init(boxNumMut, NULL);
	parNumMut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
	pthread_mutex_init(parNumMut, NULL);

	//init condition variables
	available = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  	pthread_cond_init (available, NULL);
  	parDone = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  	pthread_cond_init (parDone, NULL);
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

void lockNewParents() {
	pthread_mutex_lock(parNumMut);
	while(parNotDone)
		pthread_cond_wait(parDone,boxNumMut);
	parNotDone=1;
}

void unlockNewParents() {
	parNotDone=0;	
	pthread_mutex_unlock(parNumMut);
	pthread_cond_signal(parDone);
}

long getTimestamp(){

  gettimeofday(&endwtime, NULL);

  return((double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
                  + endwtime.tv_sec - startwtime.tv_sec)*1000);
}