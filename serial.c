#include <stdio.h>
#include <time.h>
#define N 100000
#define M 100000
int main()
{
    int* X;
    int* Y;
    int** A;
    clock_t start_time, end_time;
	float time_in_seconds = 0;
	start_time = clock();
	//DINAMICKA ALOKACIJA
	A = (int**)malloc(N * sizeof(int*));
	for(int i=0; i<N; i++) {
		A[i] = (int*)malloc(M * sizeof(int));
	}
	X = (int*)malloc(N * sizeof(int));
	Y = (int*)malloc(M * sizeof(int));
    for(int i=0; i<N;i++) {
        X[i] = i;
    }
    for(int i=0; i<M;i++) {
        Y[i] = i;
    }
    for(int i=0; i<N;i++) {
        for(int j=0; j<M; j++) {
            A[i][j] = X[i] * Y[j];
        }
    }
    end_time = clock();
	time_in_seconds = (float)(end_time - start_time) / CLOCKS_PER_SEC;
	printf("Serial execution time: %f seconds.\n", time_in_seconds);
	return 0;
}
