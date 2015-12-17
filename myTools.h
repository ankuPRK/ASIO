#include<stdlib.h>

double Determinant(double **, int);
double **Transpose(double **a, int n);
double **Adjoint(double **a, int n);
double **Inverse(double **a, int n);
long double xpowery(double x, int n);
char *int_to_string(long long int);

char *int_to_string(long long int x){
	char *pcNums;
	long long int nTemp;
	int i, nLength = 0;

	if (x == 0){
		pcNums = (char *)malloc(sizeof(char)*(1 + 1));
		pcNums[0] = '0';
		pcNums[1] = '\0';
	}else if (x > 0){
		nTemp = x;
		while (nTemp > 0){
			nLength++;
			nTemp = nTemp / 10;
		}
		nTemp = x;
		pcNums = (char *)malloc(sizeof(char)*(nLength + 1));

		for (i = 0; i < nLength; i++){

			*(pcNums + nLength - i - 1) = (nTemp % 10) + 48;
			nTemp = nTemp / 10;
		}
		pcNums[nLength] = '\0';
	}
	else{

		nTemp = x*(-1);
		while (nTemp > 0){
			nLength++;
			nTemp = nTemp / 10;
		}
		nTemp = x*(-1);
		pcNums = (char *)malloc(sizeof(char)*(nLength + 2));
		pcNums[0] = '-';
		for (i = 1; i <= nLength; i++){

			*(pcNums + nLength - i + 1) = (nTemp % 10) + 48;
			nTemp = nTemp / 10;
		}
		pcNums[nLength + 1] = '\0';

	}

	return pcNums;
}


long double xpowery(double x, int n){
	int i = 0;
	long double ans = 1;
	for (i = 0; i<n; i++){
		ans = ans*x;
	}
	return ans;
}

double **Inverse(double **a, int n){

	double **b, **c;
	double det;
	int i, j;

	det = Determinant(a, n);
	c = Adjoint(a, n);
	b = Transpose(c, n);
	free(c);

	for (i = 0; i<n; i++){
		for (j = 0; j<n; j++){
			b[i][j] = b[i][j] / det;
		}
	}

	return b;
}

double Determinant(double **a, int n){

	int i, j, i2, iTemp, jTemp, flag = 0;
	double **temp;
	double det = 0;
	if (n == 1)
		return a[0][0];
	temp = (double **)malloc(sizeof(double *)*(n - 1));
	for (i = 0; i<n; i++){
		temp[i] = (double *)malloc(sizeof(double)*(n - 1));
	}
	//i is row number, j is column number;
	for (i = 0; i<n; i++){
		//fill the matrix temp
		iTemp = 0;
		jTemp = 0;
		flag = 0;
		for (iTemp = 0; iTemp<(n - 1); iTemp++){
			for (jTemp = 0; jTemp<(n - 1); jTemp++){
				if (iTemp == i){
					flag = 1;
				}
				temp[iTemp][jTemp] = a[iTemp + flag][jTemp + 1];
			}
		}
		det = det + (double)((int)xpowery(-1, i)*Determinant(temp, (n - 1))*a[i][0]);
	}
	//free(temp);
	return det;
}

double **Transpose(double **a, int n){
	double **b;
	int i, j, k;

	b = (double **)malloc(sizeof(double *)*n);
	for (i = 0; i<n; i++){
		b[i] = (double *)malloc(sizeof(double)*n);
	}

	for (i = 0; i<n; i++){
		for (j = 0; j<n; j++){
			b[i][j] = a[j][i];
		}
	}

	return b;
}

double **Adjoint(double **a, int n){
	double **b;
	double **temp;
	int i, j, iTemp, jTemp, flagi, flagj;

	b = (double **)malloc(sizeof(double *)*n);
	for (i = 0; i<n; i++){
		b[i] = (double *)malloc(sizeof(double)*n);
	}
	if (n == 1){
		b[0][0] = 1;
		return b;
	}

	temp = (double **)malloc(sizeof(double *)*(n - 1));
	for (i = 0; i<n; i++){
		temp[i] = (double *)malloc(sizeof(double)*(n - 1));
	}

	for (i = 0; i<n; i++){
		for (j = 0; j<n; j++){
			//fill the temp matrix
			iTemp = 0;
			jTemp = 0;
			flagi = 0;
			for (iTemp = 0; iTemp<(n - 1); iTemp++){
				flagj = 0;
				for (jTemp = 0; jTemp<(n - 1); jTemp++){
					if (iTemp == i){
						flagi = 1;
					}
					if (jTemp == j){
						flagj = 1;
					}
					temp[iTemp][jTemp] = a[iTemp + flagi][jTemp + flagj];
				}
			}
			b[i][j] = (int)xpowery(-1, (i + j))*Determinant(temp, (n - 1));
		}
	}
	//	free(temp);
	return b;
}
