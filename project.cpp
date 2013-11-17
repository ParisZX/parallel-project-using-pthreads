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

#define NUM_OF_POINTS 100000 //2^24
//#define NUM_OF_BOXES 10000
#define NUM_OF_THREADS 32

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
void calculateLimits(double *xll,double *xul,double *yll,double *yul,
					 double *zll,double *zul,Box *myBox);
void *runBox(void *arg);

Box *box;	 	//array of all boxes
//Box *leaf;	//array of non-null leaf boxes

double A[NUM_OF_POINTS][3], B[NUM_OF_POINTS][3];
int boxIdCounter;

int main() {

	//calculate the points
	calculatePoints();
	
	
	boxIdCounter=1;

	box = new Box[boxIdCounter];

	//init first box
	box[0].level=0;	box[0].boxid=1;
	box[0].center[0]=0.5;
	box[0].center[1]=0.5;
	box[0].center[2]=0.5;
	

	runBox(&box[0]);
	delete [] box;

}

void *runBox(void *arg) {

	long i;
	double *start,xLowerLim,xUpperLim,yLowerLim,yUpperLim,zLowerLim,zUpperLim;
	Box *myBox=(Box *)arg; //typecasting the argument, which is the actual box
	Box children[8];

	calculateLimits(&xLowerLim,&xUpperLim,&yLowerLim,&yUpperLim,
					&zLowerLim,&zUpperLim,myBox);
	
	myBox->n=0;

	for(i=0;i<NUM_OF_POINTS;i++) {
			//evaluate if point is in the limits of the box
			if((A[i][0]>=xLowerLim)&&(A[i][0]<xUpperLim)
				&&(A[i][1]>=yLowerLim)&&(A[i][1]<yUpperLim)
				&&(A[i][2]>=zLowerLim)&&(A[i][2]<zUpperLim)) {
				myBox->n++; //increment the number of points
			}
		}
	
	printf("\nBox %i:	level = %i, center=%G,%G,%G, 		n=%i\n",
			myBox->boxid,myBox->level,myBox->center[0],myBox->center[1],
			myBox->center[2],myBox->n);
	
	if(myBox->n==0) {
		myBox->boxid=0;
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
			
			box=new Box[boxIdCounter];
			//box=(Box *)realloc(box,boxIdCounter*sizeof(Box));
			box[boxIdCounter-1]=children[i];
			
			runBox(&box[boxIdCounter-1]);
		
			
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

void calculateLimits(double *xll,double *xul,double *yll,double *yul,
					 double *zll,double *zul, Box *myBox) {

	*xll=(myBox->center[0]-(1/pow(2,(myBox->level+1))));
	*xul=(myBox->center[0]+(1/pow(2,(myBox->level+1))));
	*yll=(myBox->center[1]-(1/pow(2,(myBox->level+1))));
	*yul=(myBox->center[1]+(1/pow(2,(myBox->level+1))));
	*zll=(myBox->center[2]-(1/pow(2,(myBox->level+1))));
	*zul=(myBox->center[2]+(1/pow(2,(myBox->level+1))));
	
}

//==========================================================================

//threads
// pthread_t Threads[NUM_OF_THREADS];
// void *exec (void *arg);

// //MUTEX
// pthread_mutex_t mut;
// pthread_cond_t available;
// int unavailable=0;

