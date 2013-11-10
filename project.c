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

//structure with box info
typedef struct Box {
 	int level, boxid, parent, *child[8], n, start, colleague[26];
 	float center[3];
}Box;

//the return struct of 0<n<=8
typedef struct returnSt {
	int level, n;
	double *start; //starting address of points in B for that particular box
}returnSt;

Box box[]; 	//array of all boxes
Box leaf[];	//array of non-null leaf boxes
long NUM_OF_POINTS = 16777216; //2^24
double A[NUM_OF_POINTS][3], B[NUM_OF_POINTS][3];
int counterB;
	int main() {

	//generate random solutions to the problem
	double xsqr, ysqr, zsqr, x, y, z, xmean=0, ymean=0, zmean=0;
	long i;
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

		//check if correct
		//printf("x^2 + y^2 + z^2 = %G\n",xsqr+ysqr+zsqr);
	}

	void *exec(void *box) {
		long i;
		double *start;
		double temp[8][3];
		Box *myBox=(Box *)box; //typecasting the argument, which is the actual box
		for(i=0;i<NUM_OF_POINTS;i++) {
			//evaluate if point is in the limits of the box
			xLowerLim=(myBox.center[0]-(1/pow(myBox.level,2)));
			xUpperLim=(myBox.center[0]+(1/pow(myBox.level,2)));
			yLowerLim=(myBox.center[1]+(1/pow(myBox.level,2)));
			yUpperLim=(myBox.center[1]+(1/pow(myBox.level,2)));
			zLowerLim=(myBox.center[2]+(1/pow(myBox.level,2)));
			zUpperLim=(myBox.center[2]+(1/pow(myBox.level,2)));
			if((A[i][0]>=xLowerLim)&&(A[i][0]<xUpperLim)
				&&(A[i][1]>=yLowerLim)&&(A[i][1]<yUpperLim)
				&&(A[i][2]>=zLowerLim)&&(A[i][2]<zUpperLim)) {
				myBox.n++; //increment the number of points
				if(myBox.n==1)
					start=A[i];
				if(n<=8) {
					temp[n-1][0]=A[i][0];
					temp[n-1][1]=A[i][1];
					temp[n-1][2]=A[i][2];
				}
			}
		}
		if(myBox.n==0) {
			myBox.boxid=0;
			return 0;
		else if(myBox.n<=8) {
			returnSt ret;
			ret.level = myBox.level;
			ret.n = myBox.n;
			ret.start = start;
			for(i=0;i<8;i++) {
				B[count][0]=temp[i][0];
				B[count][1]=temp[i][1];
				B[count][2]=temp[i][2];
				count++;
			}
			return ret;
		}
		else {
			
		}
	}
}