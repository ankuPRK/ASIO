#include<stdlib.h>
#include<stdio.h>

#define TwoPow31Minus1 2147483647
#define factor 1000.0

bool *get_knots(short *, int);
bool *get_zero_crossings(short *x, int arraysize);
POINT *self_autocorrelate(short *x, int arraysize);
double **characterise(short *, bool *, int);
float *get_pitch_of_sample(short *, int arraysize);
float *get_energy_peak_envelope(short *x, int arraysize);
POINT *self_autocorrelation_by_window(short *x, int windowsize, int arraysize);
void smoothen_by_averaging(short *x, int arraysize, int Smoothing);
short *get_derivative(short *x, int arraysize);
double *get_convolution(short *x, int arraysize);

//rectangular window structure//
struct window{
	bool bSizeDetermined = false;
	int sampleWidth = 0;			//width of window in terms of number of samples
	float *data;					//stores the samples inside the window
	int nSampleCount = 0;

	//average length of curve analysis parameters
	float *pdLengthPastBuffer;
	float fAvgPastLength = 0;
	int nPastLengthBufferSize = 0;		//the Secondary buffer which corresponds to the previous sound
	int nPastLengthBufferCount = 0;		//memory of the brain
	float dAvgLength = 0;
	double dLengthVariance = 0;

	//average energy analysis parameters
	float *pdEnergyPastBuffer;
	float fAvgPastEnergy = 0;
	int nPastEnergyBufferSize = 0;		//the Secondary buffer which corresponds to the previous sound 
	int nPastEnergyBufferCount = 0;		//memory of the brain
	float dAvgEnergy = 0;
	double dEnergyVariance = 0;
	float PrevMaxAmp = 0;
	float CurrMaxAmp = 0;
	float GlobalMaxAmp = 0;
	bool bStarted = false;
	bool bEnded = true;

}window;

/**1. Takes a SHORT array and its size as input
	  and returns a bool array which is true ( or 1 )
	  at all indices wherever there is:
		a) local maxima
		b) local minima
		c) zero crossings
	  At all other indices value is false ( or 0 ).
*/
bool *get_knots(short *x, int arraysize){

	int i = 0, j = 0, k = 0, count = 0, prev = 0;
	bool peak_true;
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
			i = i + 3;
			continue;
		}
		//Test for the peak
		peak_true = true;
		for (j = i - 4; j < i + 5; j++){
			peak_true = peak_true*(x[i]*x[i]> x[j]*x[j] || (i == j));
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
			//x[i] is the point of maxima/minima
			bpKnot[i] = true;
			i = i + 3;
			continue;
		}

	}	//end of FOR loop

	bpKnot[arraysize - 1] = true;

	return bpKnot;
}	//end of function

/**2. Takes as input a SHORT array and its size
	  to get its auto-correlation to itself.
	  Since auto-correlation is symmetric about n=0
	  we only consider positive shifting.
	  The function returns a POINT array 
	  which contains the auto-correlation values 
	  vs the +ve shift values.
	  Its amplitude varies between -1000 and 1000.
*/
POINT *self_autocorrelate(short *x, int arraysize){

	long double val, divideMe;
	int i = 0, j = 0;
	//	int nInit, nFinal;
	char *number;
	POINT *arr;
	arr = (POINT *)malloc(sizeof(POINT) * arraysize);

	val = 0;
	for (j = i; j<arraysize; j++){
		val = val + (((float)x[j] / 3276.5) * ((float)x[j - i] / 3276.5));

	}
	divideMe = val / 1000.0;
	arr[i].x = i;
	arr[i].y = (val / divideMe);

	for (i = 1; i <= (arraysize - 1); i++){
		//corr will be stored in array;
		//nInit = (int)max(i, 0);
		//nFinal = (int)min(arraysize, arraysize + i);
		val = 0;
		for (j = i; j<arraysize; j++){
			val = val + (((float)x[j] / 3276.5) * ((float)x[j - i] / 3276.5));

		}
		arr[i].x = i;
		arr[i].y = val / divideMe;
	}
	return arr;
}

/**3. This function takes as input a SHORT array,
	  its size and the size of the rectangular autocorrelation window.
	  The window is then traversed along the array 
	  till it first touches the last sample. 
	  Hence total no. of values is ( arraysize - windowsize + 1).
	  The function returns a POINT array which contains the 
	  auto-correlation values vs +ve shift values.  
*/
POINT *self_autocorrelation_by_window(short *x, int windowsize, int arraysize){

	long double val, divideMe;
	int i = 0, j = 0;
	char *number;
	POINT *arr;
	arr = (POINT *)malloc(sizeof(POINT) * (arraysize-windowsize+1));

	for (i = 0; i <= (arraysize - windowsize); i++){
		val = 0;
		for (j = i; j<i+windowsize; j++){
			val = val + (((float)x[j] / 3276.5) * ((float)x[j - i] / 3276.5));
		}
		arr[i].x = i;
		arr[i].y = val;
	}
	return arr;
}

/*
double **characterise(short *spBuffer, bool *bpKnot, int arraysize){
	int i = 0, j = 0, k = 0, l = 0;
	int nKnotCount = 0, nExtraKnot = 0, count = 0;
	double **dppabcd, **xMatrix;
	double **xInverse;
	double yMatrix[4];
	int nthreeKnots[3][2];
	POINT five_points[5];
	FILE *fpcreate = fopen("abcd.txt","w");

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


				xInverse = inverse_of_matrix(xMatrix, 4);
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
*/

/**4. This function finds the time period of a signal
	  by calculating the time difference between 
	  largest and second largest peaks of auto-correlation
	  and returns the inverse of this time-period 
	  as well as the ratio of second peak to first peak.
	  The length of auto-correlation window for this function 
	  is of half the length of the arraysize of the input SHORT array. 
*/
float *get_pitch_of_sample(short *x, int arraysize){

	int i = 0, j = 0;
	double spCorrArray[512];
	bool max_true;
	int nfirstpeak = 0, nsecondpeak = 0;			//first peak is zero obviously
	float distance = 0;
	float *ffrequency;
	ffrequency = (float *)malloc(sizeof(float) * 2);

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
		return ffrequency;
	}


	ffrequency[0] = 44100.0 / (distance*1.0);
	ffrequency[1] = nsecondpeak/nfirstpeak;
	//free(spCorrArray);
	return ffrequency;
}

/**5. Returns a float array which is the 
	  peak envelope of the energy of the input SHORT array.
	  Its value is scaled between 0 to 10000.
	  NOTE: All the consecutive points of the envelope are connected linearly.  
*/
float *get_energy_peak_envelope(short *x, int arraysize){

	int i = 0, j = 0, k = 0, prev = 0;
	bool peak_true;
	float *fpBuffer;
	fpBuffer = (float *)malloc(sizeof(float)*arraysize);
	fpBuffer[i] = (x[i] / 327.68) * (x[i] / 327.68);
	// check maxima in a size of nine samples i.e. frequencies more than (44100/9)~4900 aren't considered.
	for (i = 3; i < arraysize - 3; i++){

		//Test for the maxima
		peak_true = true;
		for (j = i - 3; j < i + 4; j++){
			peak_true = peak_true*(x[i]*x[i] > x[j]*x[j] || (i == j));
			if (peak_true == false)
				break;
		}

		if (peak_true == false && x[i] == x[i - 1]){
			peak_true = true;
			for (j = i + 1; j < i + 4; j++){
				peak_true = peak_true*(x[i]*x[i] > x[j]*x[j]);
				if (peak_true == false)
					break;
			}
		}
		else if (peak_true == false && x[i] == x[i + 1]){
			peak_true = true;
			for (j = i - 3; j < i; j++){
				peak_true = peak_true*(x[i]*x[i] > x[j]*x[j]);
				if (peak_true == false)
					break;
			}
		}

		if (peak_true == true){
			fpBuffer[i] = (x[i] / 327.68)*(x[i] / 327.68);
			for (k = prev; k <= i; k++){
				fpBuffer[k] = fpBuffer[prev] + ((fpBuffer[i] - fpBuffer[prev]) / (i*1.0 - prev*1.0))*(k - prev);
			}
			prev = i+1;
			i = i + 2;
		}
	}	//end of FOR loop

	fpBuffer[arraysize - 1] = (x[arraysize - 1] / 327.68)*(x[arraysize - 1] / 327.68);
	for (k = prev; k < arraysize; k++){
		fpBuffer[k] = fpBuffer[prev] + ((fpBuffer[i] - fpBuffer[prev]) / (i*1.0 - prev*1.0))*(k - prev);
	}

	return fpBuffer;
}

/**7. This function takes a SHORT array x, 
	  its length and the smoothing size as input and
	  smoothens the signal stored in array
	  by replacing each element x[i] 
	  by the average of 2*Smoothing+1 elements 
	  distributed uniformly around it.
*/
void smoothen_by_averaging(short *x, int arraysize, int Smoothing){

	if (Smoothing < 0)
		return;

	short *copy = (short *)malloc(sizeof(short)*arraysize);
	int i, j;

	for (i = 0; i < arraysize; i++){
		copy[i] = x[i];
	}

	for (i = Smoothing; i < arraysize - Smoothing; i++){
		x[i] = 0;
		for (j = i - Smoothing; j < i + Smoothing; j++){
			x[i] = x[i] + copy[j];
		}
		x[i] = x[i] / (2 * Smoothing + 1);
	}
	return;
}


/**8. Takes a SHORT array and its size as input
	  and returns a bool array of same size
	  which is true ( or 1 ) at those indices 
	  where there is a zero crossing.
	  At all other indices value is false ( or 0 ).
*/
bool *get_zero_crossings(short *x, int arraysize){

	int i = 0, j = 0, k = 0, count = 0, prev = 0;
	bool peak_true;
	bool *bpKnot;
	bpKnot = (bool *)malloc(sizeof(bool)*arraysize);

	for (i = 0; i < arraysize; i++){
		bpKnot[i] = false;
	}

	count = 0;
	bpKnot[0] = true;

	for (i = 0; i < arraysize - 1; i++){
		//Test for the zero crossing
		if (x[i] * x[i + 1] <= 0 && (x[i] != 0 || x[i + 1] != 0)){
			//x[i] is the zero crossing
			bpKnot[i] = true;
		}
	}	//end of FOR loop

	return bpKnot;
}	

/**9. Takes a SHORT array and its arraysize as input 
	  and returns an array of size arraysize - 1
	  containing the 'left' discrete derivatives of the sequence
*/
short *get_derivative(short *x, int arraysize){
	short *derivative = (short *)malloc(sizeof(short)*(arraysize - 1));
	int i, j;

	for (i = 0; i < arraysize - 1; i++){
		derivative[i] = x[i + 1] - x[i];
	}

	return derivative;
}

/**10. This function takes two SHORT arrays:
		a) the input signal: x[n]		(which is zero for every n<0)
		b) the inpulse response signal: h[n]	(which is zero for every n<0)
	   And then it returns a DOUBLE array containing y[n] = 0.0001 times x[n](convolved with)h[n]
	   which is zero everywhere except from n=0 to n = x_size+h_size-1
*/
double *get_discrete_convolution(short *x, int x_size,short *h, int h_size){

	int n = 0, k = 0;
	double *y = (double *)malloc(sizeof(double)*(x_size + h_size - 1));

	for (n = 0; n < x_size + h_size - 1; n++){
		y[n] = 0;
		for (k = max(0,n-x_size+1); k <= min(n,h_size - 1); k++){
			y[n] = y[n] + (h[k]/100.0) * (x[n - k]/100.0);
		}
	}

	return y;
}




