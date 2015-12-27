#include<stdio.h>
#include<stdlib.h>

double determinant_of_matrix(double **, int);
double **transpose_of_matrix(double **a, int rows, int columns);
double **adjoint_of_matrix(double **a, int n);
double **inverse_of_matrix(double **a, int n);
double **add_matrices(double **a, double factor1,double **b,double factor2, int rows, int columns);
double **multiply_matrices(double **a, int rows1, int columns1, double **b, int rows2, int columns2 );
void multiply_matrix_by_scalar(double **a, double value, int rows, int columns);

double **inverse_of_matrix(double **a, int n){

	double **b, **c;
	double det;
	int i, j;

	det = determinant_of_matrix(a, n);
	c = adjoint_of_matrix(a, n);
	b = transpose_of_matrix(c, n, n);
	free(c);

	for (i = 0; i<n; i++){
		for (j = 0; j<n; j++){
			b[i][j] = b[i][j] / det;
		}
	}

	return b;
}

double determinant_of_matrix(double **a, int n){

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
		det = det + (double)((int)xpowery(-1, i)*determinant_of_matrix(temp, (n - 1))*a[i][0]);
	}
	//free(temp);
	return det;
}

double **transpose_of_matrix(double **a, int rows, int columns){
	double **b;
	int i, j, k;
	int transposedRows = columns;
	int transposedColumns = rows;
	b = (double **)malloc(sizeof(double *)*transposedRows);
	for (i = 0; i<transposedRows; i++){
		b[i] = (double *)malloc(sizeof(double)*transposedColumns);
	}

	for (i = 0; i<transposedRows; i++){
		for (j = 0; j<transposedColumns; j++){
			b[i][j] = a[j][i];
		}
	}

	return b;
}

double **adjoint_of_matrix(double **a, int n){
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
			b[i][j] = (int)xpowery(-1, (i + j))*determinant_of_matrix(temp, (n - 1));
		}
	}
	//	free(temp);
	return b;
}

double **add_matrices(double **a, double factor1, double **b, double factor2, int rows, int columns){
	double **result;
	int i = 0, j = 0;
	result = (double **)malloc(sizeof(double *)*rows);
	for (i = 0; i<rows; i++){
		result[i] = (double *)malloc(sizeof(double)*columns);
	}
	for (i = 0; i < rows; i++){
		for (j = 0; j < columns; j++){
			result[i][j] = factor1*a[i][j] + factor2*b[i][j];
		}
	}
	return result;
}

double **multiply_matrices(double **a, int rows1, int columns1, double **b, int rows2, int columns2){
	double **result;
	int i = 0, j = 0,k=0;

	
	if (columns1 != rows2)
		return result;		//multiplication invalid. Return NULL 

	result = (double **)malloc(sizeof(double *)*rows1);
	for (i = 0; i < rows1; i++){
		result[i] = (double *)malloc(sizeof(double)*columns2);
	}

	for (i = 0; i < rows1; i++){
		for (j = 0; j < columns2; j++){
			result[i][j] = 0;
			for (k = 0; k < columns1; k++){
				result[i][j] = result[i][j] + a[i][k] * b[k][j];
			}
		}
	}

	return result;
}

void multiply_matrix_by_scalar(double **a, double value, int rows, int columns){
	int i = 0, j = 0;
	for (i = 0; i < rows; i++){
		for (j = 0; j < columns; j++){
			a[i][j] = a[i][j] * value;
		}
	}
}
