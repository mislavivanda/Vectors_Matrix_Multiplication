#include <stdio.h>
#include <time.h>
#include <pthread.h>
#define N_THREADS 8
#define N 100000
#define M 100000
int* X;
int* Y;
int** A;
//SVAKI THREAD RACUNA DIO MATRICE KOJI JE DIMENZIJA (N / N_THREADS, M)
void* multiply_blocks(void* thread_index) {
	long thread_index_long = (long)thread_index; //casting
	int row_offset_index = thread_index_long * N / N_THREADS;
	for (int i = row_offset_index; i < row_offset_index + N / N_THREADS; i++) {
		for (int j = 0; j < M; j++) {
			A[i][j] = X[i] * Y[j];
		}
	}
}
int main(int argc, char** argv) { 
	int status;
	pthread_t threads[N_THREADS];
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
	//INICIJALIZACIJA VEKTORA
	for (int i = 0; i < N; i++) {
		X[i] = i;
	}
	for (int j = 0; j < M; j++) {
		Y[j] = j;
	}
	for (int i = 0; i < N_THREADS; i++) {
		//VRACA 0 AKO JE SVE OK
		status = pthread_create(&threads[i], NULL, multiply_blocks, (void*)i);
		if (status) {
			printf("Error returned from pthread_create()\n");
		}
	}
	for( int i = 0; i < N_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
	end_time = clock();
	time_in_seconds = (float)(end_time - start_time) / CLOCKS_PER_SEC;
	printf("Shared memory with %d threads time: %f seconds.\n", N_THREADS, time_in_seconds);
	return 0;
}
