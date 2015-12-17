#include<stdlib.h>
#include<stdio.h>
#define TwoPow31Minus1 2147483647
#define factor 1000.0
bool *get_knots(short *, int);
POINT *autocorrelate(int *x, int arraysize);
double **characterise(short *, bool *, int);
float *get_pitch_of_sample(short *, int arraysize);
int *get_peak_envelope(short *x, int arraysize);

bool *get_knots(short *x, int arraysize){

	int i = 0, j = 0, k = 0, count = 0, count2 = 0, prev = 0;
	bool max_true, min_true;
	bool *bpKnot;
	bpKnot = (bool *)malloc(sizeof(bool)*arraysize);

	for (i = 0; i < arraysize; i++){
		bpKnot[i] = false;
	}


	count = 0;
	bpKnot[0] = true;

	for (i = 4; i < arraysize - 4; i++){

		//Test for the zero crossing
		if (x[i] * x[i + 1] <= 0 && (x[i] != 0 || x[i + 1] != 0)){
			//x[i] is the zero crossing
			bpKnot[i] = true;
			i = i + 4;
			continue;
		}
		//Test for the maxima
		max_true = true;
		for (j = i - 4; j < i + 5; j++){
			max_true = max_true*(x[i] > x[j] || (i == j));
			if (max_true == false)
				break;
		}

		if (max_true == false && x[i] == x[i - 1]){
			max_true = true;
			for (j = i + 1; j < i + 5; j++){
				max_true = max_true*(x[i] > x[j]);
				if (max_true == false)
					break;
			}
		}
		else if (max_true == false && x[i] == x[i + 1]){
			max_true = true;
			for (j = i - 4; j < i; j++){
				max_true = max_true*(x[i] > x[j]);
				if (max_true == false)
					break;
			}
		}

		if (max_true == true){
			//x[i] is the point of maxima
			bpKnot[i] = true;
			i = i + 4;
			continue;
		}

		//Test for the minima
		min_true = true;
		for (j = i - 4; j < i + 5; j++){
			min_true = min_true*(x[i] < x[j] || (i == j));
			if (min_true == false)
				break;
		}

		if (min_true == false && x[i] == x[i - 1]){
			min_true = true;
			for (j = i + 1; j < i + 5; j++){
				min_true = min_true*(x[i] < x[j]);
				if (min_true == false)
					break;
			}
		}
		else if (min_true == false && x[i] == x[i + 1]){
			min_true = true;
			for (j = i - 4; j < i; j++){
				min_true = min_true*(x[i] < x[j]);
				if (min_true == false)
					break;
			}
		}


		if (min_true == true){
			//x[i] is the point of minima
			bpKnot[i] = true;
			i = i + 4;
		}
	}	//end of FOR loop

	bpKnot[arraysize - 1] = true;

	return bpKnot;
}	//end of function

POINT *autocorrelate(int *x, int arraysize){

	long double val, divideMe;
	int i = 0, j = 0;
	//	int nInit, nFinal;
	char *number;
	POINT *arr;
	arr = (POINT *)malloc(sizeof(POINT) * arraysize);

	val = 0;
	for (j = i; j<arraysize; j++){
		val = val + (((float)x[j] / 1024.0) * ((float)x[j - i] / 1024.0));

	}
	divideMe = val / 256.0;
	arr[i].x = i;
	arr[i].y = (val / divideMe);

	for (i = 1; i <= (arraysize - 1); i++){
		//corr will be stored in array;
		//nInit = (int)max(i, 0);
		//nFinal = (int)min(arraysize, arraysize + i);
		val = 0;
		for (j = i; j<arraysize; j++){
			val = val + (((float)x[j] / 1024.0) * ((float)x[j - i] / 1024.0));

		}
		arr[i].x = i;
		arr[i].y = val / divideMe;
	}
	return arr;
}

double **characterise(short *spBuffer, bool *bpKnot, int arraysize){
	int i = 0, j = 0, k = 0, l = 0;
	int nKnotCount = 0, nExtraKnot = 0, count = 0;
	double **dppabcd, **xMatrix;
	double **xInverse;
	double yMatrix[4];
	int nthreeKnots[3][2];
	POINT five_points[5];
	FILE *fpcreate = fopen("abcd.txt","w");

	xMatrix = (double **)malloc(sizeof(double *) * 4);

	for (i = 0; i < 4; i++){
		xMatrix[i] = (double *)malloc(sizeof(double) * 4);
	}

	for (i = 0; i < arraysize; i++){
		nKnotCount = nKnotCount + (int)bpKnot[i];
	}

	if (nKnotCount < 3){
		bpKnot[(arraysize - 1) / 2] = true;
		nKnotCount++;
	}
	else
		if ((nKnotCount - 3) % 2 != 0){

			for (j = arraysize - 2; j > 0; j--){
				if (bpKnot[j]){
					nExtraKnot = nExtraKnot + j;
					count++;
				}
				if (count == 2){
					count = 0;
					break;
				}
			}
			bpKnot[nExtraKnot / 2] = true;
			nKnotCount++;
		}

	dppabcd = (double **)malloc(sizeof(double *) * (1 + 1 + (nKnotCount - 3) / 2));

	for (i = 0; i < (1 + 1 + (nKnotCount - 3) / 2); i++){
		dppabcd[i] = (double *)malloc(sizeof(double) * 6);
	}

	nthreeKnots[0][0] = 0;
	nthreeKnots[0][1] = 1;
	nthreeKnots[1][1] = 0;
	nthreeKnots[2][1] = 0;

	dppabcd[0][0] = (double)(1 + (nKnotCount - 3) / 2);	//to store the size of array

	count = 1;


	for (i = 1; i < arraysize; i++){

		if (bpKnot[i]){
			if (nthreeKnots[1][1] == 0){
				nthreeKnots[1][0] = i;
				nthreeKnots[1][1] = 1;
			}
			else if (nthreeKnots[2][1] == 0){
				nthreeKnots[2][0] = i;
				nthreeKnots[2][1] = 1;
				//make the polynomial and make all of em zero or whatever

				five_points[0].x = (double)(nthreeKnots[0][0] - nthreeKnots[0][0]);
				five_points[0].y = (double)spBuffer[nthreeKnots[0][0]];
				five_points[1].x = (double)((nthreeKnots[0][0] + nthreeKnots[1][0]) / 2 - nthreeKnots[0][0]);
				five_points[1].y = (double)spBuffer[(nthreeKnots[0][0] + nthreeKnots[1][0]) / 2];
				five_points[2].x = (double)(nthreeKnots[1][0] - nthreeKnots[0][0]);
				five_points[2].y = (double)spBuffer[nthreeKnots[1][0]];
				five_points[3].x = (double)((nthreeKnots[1][0] + nthreeKnots[2][0]) / 2 - nthreeKnots[0][0]);
				five_points[3].y = (double)spBuffer[nthreeKnots[0][0]];
				five_points[4].x = (double)(nthreeKnots[2][0] - nthreeKnots[0][0]);
				five_points[4].y = (double)spBuffer[nthreeKnots[2][0]];

				//fill the X matrix
				for (k = 0; k < 4; k++){
					for (l = 0; l < 4; l++){
						xMatrix[k][l] = 0.0;
					}
				}//erase every cell
				for (j = 0; j < 5; j++){
					for (k = 0; k < 4; k++){
						for (l = 0; l < 4; l++){
							xMatrix[k][l] = xMatrix[k][l] + (double)xpowery(five_points[j].x, l + k);
						}
					}
				}									//fill every cell


				xInverse = Inverse(xMatrix, 4);
				//free(xMatrix);
				//xInverse = xMatrix;
				//fill the Y matrix
				for (j = 0; j < 4; j++){
					yMatrix[j] = 0.0;
				}									//erase every cell
				for (j = 0; j < 5; j++){
					for (k = 0; k < 4; k++){
						yMatrix[k] = yMatrix[k] + (double)(five_points[j].y / 128.0)*(double)xpowery(five_points[j].x, k);
					}
				}									//fill every cell

				for (j = 0; j < 6; j++){
					dppabcd[count][j] = 0.0;
				}		//fill wid zeroes

				for (j = 0; j < 4; j++){
					for (k = 0; k < 4; k++){
						dppabcd[count][j] = dppabcd[count][j] + xInverse[j][k] * yMatrix[k];
					}
				}			//fill dppabcd arrays wid 0=d 1=c 2=b 3=a for ax3 + bx2 +cx + d

				dppabcd[count][4] = (double)nthreeKnots[0][0];
				dppabcd[count][5] = (double)nthreeKnots[2][0];

				for (j = 0; j < 6; j++){
					fprintf(fpcreate, "%lf", dppabcd[count][j]);
					fwrite("\t", sizeof(char), 1, fpcreate);
				}
				fwrite("\n", sizeof(char), 1, fpcreate);
				count++;

				//set three bullets for another round
				nthreeKnots[0][0] = nthreeKnots[2][0];
				nthreeKnots[1][1] = 0;
				nthreeKnots[2][1] = 0;
			}
		}		//end of if
	}			//end of FOR

	return dppabcd;
}

float *get_pitch_of_sample(short *x, int arraysize){

	int i = 0, j = 0;
	double spCorrArray[512];
	bool max_true;
	int nfirstpeak = 0, nsecondpeak = 0;			//first peak is zero obviously
	float distance = 0;
	float *ffrequency;
	ffrequency = (float *)malloc(sizeof(float) * 3);

	for (i = 0; i < arraysize / 2; i++){
		spCorrArray[i] = 0;
		for (j = 0; j < arraysize / 2; j++){
			spCorrArray[i] = spCorrArray[i] + ((x[j] / factor) * (x[j + i] / factor));
		}
	}
	nfirstpeak = spCorrArray[0];
	for (i = 0; i < arraysize/2; i++){
		if (spCorrArray[i] * spCorrArray[i + 1] <= 0){
			break;
		}
	}
	if (i == arraysize / 2){
		ffrequency[0] = 0.0;
		ffrequency[1] = 0.0;
		ffrequency[2] = 0.0;
		return ffrequency;
	}

	for (i = i; i < arraysize / 2; i++){
		//Test for the maxima
		if (spCorrArray[i] > nsecondpeak){
			distance = i;
			nsecondpeak = spCorrArray[i];
		}
	}

	if (nfirstpeak - nsecondpeak<0){
		ffrequency[0] = 0.0;
		ffrequency[1] = 0.0;
		ffrequency[2] = 0.0;
		return ffrequency;
	}


	ffrequency[0] = 44100.0 / (distance*1.0);
	ffrequency[1] = (float)(nfirstpeak - nsecondpeak) / ((float)nfirstpeak / 100.0);
	ffrequency[2] = (10.0*nsecondpeak) / (nfirstpeak*2.0);
	//free(spCorrArray);
	return ffrequency;
}

int *get_peak_envelope(short *x, int arraysize){

	int i = 0, j = 0, k = 0, prev = 0;
	bool peak_true;
	int *npBuffer;
	npBuffer = (int *)malloc(sizeof(int)*arraysize);

	// check maxima in a size of nine samples i.e. frequencies more than (44100/9)~4900 aren't considered.
	for (i = 4; i < arraysize - 4; i++){

		//Test for the maxima
		peak_true = true;
		for (j = i - 4; j < i + 5; j++){
			peak_true = peak_true*(x[i]*x[i] > x[j]*x[j] || (i == j));
			if (peak_true == false)
				break;
		}

		if (peak_true == false && x[i] == x[i - 1]){
			peak_true = true;
			for (j = i + 1; j < i + 5; j++){
				peak_true = peak_true*(x[i]*x[i] > x[j]*x[j]);
				if (peak_true == false)
					break;
			}
		}
		else if (peak_true == false && x[i] == x[i + 1]){
			peak_true = true;
			for (j = i - 4; j < i; j++){
				peak_true = peak_true*(x[i]*x[i] > x[j]*x[j]);
				if (peak_true == false)
					break;
			}
		}

		if (peak_true == true){
			//x[i] is the point of maxima
			for (k = prev; k <= i; k++){
				npBuffer[k] = (x[i] * x[i])/4;
			}
			prev = i+1;
			i = i + 3;
		}
	}	//end of FOR loop

	for (j = prev; j < arraysize; j++){
		npBuffer[j] = x[i] * x[i];
	}

	return npBuffer;
}
