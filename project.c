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

#define NUM_OF_POINTS 30 //2^24
#define NUM_OF_BOXES 10000
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

Box box[NUM_OF_BOXES]; 	//array of all boxes
Box *leaf;	//array of non-null leaf boxes
double A[NUM_OF_POINTS][3], B[NUM_OF_POINTS][3];
int counterB=0, threadIdCounter=0;

//threads
pthread_t Threads[NUM_OF_THREADS];
void *exec (void *arg);

//MUTEX
pthread_mutex_t mut;
pthread_cond_t available;
int unavailable=0;


int main() {

	//generate random solutions to the problem
	double xsqr, ysqr, zsqr, x, y, z, xmean=0, ymean=0, zmean=0;
	long i;
	
	//allocate memory for the arrays of boxes
	//box = (Box *) malloc (sizeof (Box));
	leaf = (Box *) malloc (sizeof (Box));
	srand(0);
	pthread_mutex_init(&mut, NULL);
	pthread_cond_init (&available, NULL);

	for(i=0;i<NUM_OF_POINTS;i++) {
		
		//find x
		xsqr = ((double)rand() / (double)RAND_MAX);
		x=sqrt(xsqr);
		//printf("x^2 = %G and x = %G\n",xsqr,x);

		//find y
		ysqr = ((double)rand() / (double)RAND_MAX * (double)(1-xsqr));
		y=sqrt(ysqr);
		//printf("y^2 = %G and y = %G\n",ysqr,y);

		//find z
		zsqr= 1-xsqr-ysqr;
		z=sqrt(zsqr);
		//printf("z^2 = %G and z = %G\n",zsqr,z);
		
		//print the number of points (for validating purposes)
		//printf("point %lu:	%G,	%G,	%G\n",i,x,y,z);

		//check if correct
		//printf("x^2 + y^2 + z^2 = %G\n",xsqr+ysqr+zsqr);

		//insert the x,y,z values for each point into array A
		A[i][0]=x; A[i][1]=y; A[i][2]=z;
	}
	//first thread
	box[0].level=0;
	box[0].boxid=1;
	box[0].center[0]=0.5;
	box[0].center[1]=0.5;
	box[0].center[2]=0.5;
	pthread_create(&Threads[0], NULL, exec, &box[0]);
	threadIdCounter++;
	pthread_join(Threads[0], NULL);
	printf("\nexiting...\n\n");
	pthread_exit(NULL);
}

void *exec(void *arg) {
	long i;
	double *start,xLowerLim,xUpperLim,yLowerLim,yUpperLim,zLowerLim,zUpperLim;
	double temp[8][3];
	Box *myBox=(Box *)arg; //typecasting the argument, which is the actual box
	Box children[8];

	//printf(");
	xLowerLim=(myBox->center[0]-(1/pow(2,(myBox->level+1))));
	xUpperLim=(myBox->center[0]+(1/pow(2,(myBox->level+1))));
	//printf("xLowerLim=%G,xUpperLim=%G\n",xLowerLim,xUpperLim);
	yLowerLim=(myBox->center[1]-(1/pow(2,(myBox->level+1))));
	yUpperLim=(myBox->center[1]+(1/pow(2,(myBox->level+1))));
	zLowerLim=(myBox->center[2]-(1/pow(2,(myBox->level+1))));
	zUpperLim=(myBox->center[2]+(1/pow(2,(myBox->level+1))));
	for(i=0;i<NUM_OF_POINTS;i++) {
			//evaluate if point is in the limits of the box
			if((A[i][0]>=xLowerLim)&&(A[i][0]<xUpperLim)
				&&(A[i][1]>=yLowerLim)&&(A[i][1]<yUpperLim)
				&&(A[i][2]>=zLowerLim)&&(A[i][2]<zUpperLim)) {
				myBox->n++; //increment the number of points
				if(myBox->n==1)
					start=A[i];
				if(myBox->n<=8) {
					temp[myBox->n-1][0]=A[i][0];
					temp[myBox->n-1][1]=A[i][1];
					temp[myBox->n-1][2]=A[i][2];
				}
			}
		}
		printf("\nThread %i:	level = %i, center=%G,%G,%G, 	n=%i\n\n",myBox->boxid,myBox->level,myBox->center[0],myBox->center[1],myBox->center[2],myBox->n);
	if(myBox->n==0) {
		myBox->boxid=0;
		return 0;
	}
	else if(myBox->n<=20) {
		returnSt *ret = (returnSt *)malloc(sizeof(returnSt));
		ret->level = myBox->level;
		ret->n = myBox->n;
		ret->start = start;
		for(i=0;i<20;i++) {
			B[counterB][0]=temp[i][0];
			B[counterB][1]=temp[i][1];
			B[counterB][2]=temp[i][2];
			counterB++;
		}
		return (void *)ret;
	}
	else {
		//create binary represantation of i, to find the centers of the 8 subboxes
		int bin0,bin1,bin2,ids[8];

		pthread_mutex_lock(&mut);
		while(unavailable) {
			pthread_cond_wait(&available,&mut);
		}
		unavailable=1;

		for(i=0;i<8;i++) {
			bin0=i%2;
			bin1=(i/2)%2;
			bin2=i/4;
			if(bin0==0) bin0=(-1);
			if(bin1==0) bin1=(-1);
			if(bin2==0) bin2=(-1);
						
			myBox->child[i]=threadIdCounter;
			children[i].level=myBox->level+1;
			children[i].boxid=threadIdCounter+i+1;
			children[i].center[0]=myBox->center[0]+(1/pow(2,(myBox->level+1)))*bin0*0.5;
			children[i].center[1]=myBox->center[1]+(1/pow(2,(myBox->level+1)))*bin1*0.5;
			children[i].center[2]=myBox->center[2]+(1/pow(2,(myBox->level+1)))*bin2*0.5;
			ids[i]=threadIdCounter;
		}
		for(i=0;i<8;i++) {
			threadIdCounter++;
			box[threadIdCounter]=children[i];
			//printf("level=%i, id=%i, center=%G,%G,%G\n",	box[threadIdCounter].level,box[threadIdCounter].boxid,
			//												box[threadIdCounter].center[0],box[threadIdCounter].center[1],box[threadIdCounter].center[2]);
			pthread_create(&Threads[threadIdCounter], NULL, exec, &box[threadIdCounter]);
		}
		pthread_mutex_unlock(&mut);
		unavailable=0;
		pthread_cond_signal(&available);

		for(i=0;i<8;i++)
			pthread_join(Threads[ids[i]], NULL);
	}
}
