#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#define N 100000
#define M 100000
int main(int argc, char** argv) {
	int process_rank, process_count;
	double start_time, end_time;
	float time_in_seconds = 0;
	int* X;
	int* Y;
    int** result = NULL; //KONACNA REZULTANTNA MATRICA -> ALOCIRA SE SAMO U MASTER THREAD
    int* pre_result = NULL;
	int* recvPtr;
    int sizes[2];//DIMENZIJE CIJELE KONACNE MATRICE
    int subSizes[2];//DIMENZIJE BLOKA MATRICE SVAKOG PROCESA
    int startCoords[2];//POCETNE (x,y)  KOORDINATE BLOKA MATRICE U ODNOSU NA CIJELU KONACNU MATRICU
	MPI_Datatype recvBlock, recvMagicBlock;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &process_count);
	MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);
	MPI_Barrier(MPI_COMM_WORLD); //CEKAJ DOK SVI PROCESI BUDU SPREMNI ZA RELEVANTNIJE MJERENJE VREMENA
	start_time = MPI_Wtime();
	if(process_rank == 0) {
		//ALOCIRAJ VARIJABLE
		X = (int*)malloc(N *sizeof(int));
		Y = (int*)malloc(M * sizeof(int));
		//MPI -> 2D NIZ KONTINUIRANO ALOCIRAN U MEMORIJI(BITNO ZAHJEV MPI-a)
      	pre_result = (int*) malloc(N * M * sizeof(int));
      	result = (int**) malloc(N * sizeof(int*));
		for (int i = 0; i < N; i++) {
			result[i] = &(pre_result[i * M]);
		}
		sizes[0] = N;
		sizes[1] = M;
		subSizes[0] = N/process_count;
		subSizes[1] = M;
		startCoords[0] = 0;
		startCoords[1] = 0;
		//PODIJELI MATRICU NA VELICINE BLOKOVA KOJE RACUNA SVAKI PROCES I TIME DEFINIRAJ NOVI TIP KOJI CE PREDSTAVLJAT TAJ BLOK(BITNO KASNIJE ZA SKUPLJANJE PODATAKA U JEDINSTVENU REZULTANTNU MATRICU)
		MPI_Type_create_subarray(2, sizes, subSizes, startCoords, MPI_ORDER_C, MPI_INT, &recvBlock);

		// DEFINIRAJ SVAKO KOLIKO STUPACA DOLAZIMO DO NOVOG BLOKA U BAJTOVIMA
		MPI_Type_create_resized(recvBlock, 0, M * sizeof(int), &recvMagicBlock);

		MPI_Type_commit(&recvMagicBlock);
		recvPtr = &result[0][0];
		
		//POPUNI VEKTORE
		for (int i = 0; i < N; i++) {
			X[i] = i;
		}
		for (int i = 0; i < M; i++) {
			Y[i] = i;
		}
	}
	//BUFFERI ZA PRIJENOS PODATAKA IZMEDU PROCESA
	//X DIO JE FIKSAN I NE SALJE SE RAZMJENJUJE SE IZMEDU PROCESA, Y SE RAZMJENJUJE
	int* X_buffer = (int*)malloc((N / process_count) * sizeof(int));
	int* Y_buffer_receive = (int*)malloc((M / process_count) * sizeof(int));
	int* Y_buffer_send = (int*)malloc((M / process_count) * sizeof(int));
	//REZULTAT = MATRICA (n,m) -> SVAKI PROCES CE IZRACUNAT (N / process_count) REDAKA MATRICE I SVE STUPCE 
	//MPI -> 2D NIZ KONTINUIRANO ALOCIRAN U MEMORIJI
	int* pre_result_matrix_block = (int*)malloc((N / process_count) * M * sizeof(int));
	int** result_matrix_block = (int**)malloc((N / process_count) * sizeof(int*));
	for (int i = 0; i < (N / process_count); i++) {
		result_matrix_block[i] = &(pre_result_matrix_block[i * M]);
	}
	//RASPODIJELI X I Y BLOKOVE VEKTORA U PROCESE NA JEDNAKE VELICINE OVISNE O BROJU PROCESA
	//SVAKI PROCES IMA SVOJE POCETNE DIJELOVE X I Y VEKTOR
	MPI_Scatter((void*)X, N / process_count, MPI_INT,(void*) X_buffer,N / process_count, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Scatter((void*)Y, M / process_count, MPI_INT,(void*) Y_buffer_send,M / process_count, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	//KRENI S RACUNANJEM BLOKOVA MATRICA I ROTACIJOM BLOKOVA
	//BROJ ITERACIJA -> BROJ PROCESA
	for (int i = 0; i < process_count; i++) {
		//POSALJI TRENUTNI BLOK IDUCEM PROCESU(+1) -> MAX INDEKS = process_count - 1 -> MODUL process_count
		int next_process_rank = (process_rank + 1) % process_count;
		MPI_Send((void*)Y_buffer_send, M / process_count, MPI_INT, next_process_rank, 0, MPI_COMM_WORLD);
		int from_process_rank = process_rank - 1;
		if (from_process_rank < 0) {
			from_process_rank = process_count - 1;//dobiva od zadnjeg procesa
 		}		
		//IZRACUNAJ DIO MATRICE -> PUNIMO STUPCE OD result_matrix_block
		//POCETNI INDEKS STUPCA = ZA PRVI PROCES KRECEMO OD PRVOG BLOKA I IDEMO SVE DO ZADNJEG
		//ZA DRUGIU PROCES KRECEMO OD DRUGOG BLOKA I ZAVRSAVAMO S PRVIM
		//KOD TRECEG KRECEMO S TRECIM I ZAVRSAVAMO S DRUGIM itd
		//INDEKS BLOKA = (process_rank + i) % process_count
		//SIRINA SVAKOG BLOKA MATRICE = M / process_count

		int result_matrix_block_start_index = (process_rank + i) % process_count;
		for (int j = 0; j < N / process_count; j++) {
			for (int k = 0; k < M / process_count; k++) {
				result_matrix_block[j][result_matrix_block_start_index * (M / process_count) + k] = X_buffer[j] * Y_buffer_send[k];
			}
		}
		//PRIMI PAKET OD PRETHODNOG PROCESA -> MIN INDEKS = 0 -> 0 -1 = -1 A TO JE ADTIVNI INVERZ OD 1 -> (MODUL-1
		MPI_Recv((void*)Y_buffer_receive, M / process_count, MPI_INT, from_process_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		//PROSLIJEDIVANJE PRIMLJENOG BLOKA DRUGOM PROCESU
		Y_buffer_send =  Y_buffer_receive;
	}
	MPI_Barrier(MPI_COMM_WORLD);
	int np = process_count;
  	int recvCounts[np]; //BROJ BLOKOVA KOJE CE SVAKI PROCES POSLATI A GLAVNI PROCES PRIMITI -> SVAKI PROCES SALJE 1 BLOK
	int displacements[np];//LOKACIJE BLOKOVA RELATIVNE U ODNOSU NA GLAVNU MATRICU
  	for (int i = 0; i < np; i++) {
		recvCounts[i] = 1;
	}
  	int index = 0;
  	for (int p_row = 0; p_row < process_count; p_row++) {
    	for (int p_column = 0; p_column < 1; p_column++) {//1 -> SVAKI BLOK MATRICE SE PROTEZE KROZ SVE STUPCE KONACNE MATRICE
			displacements[index++] = p_column  +  p_row *( (N / process_count) * 1);
		}
	}
  	// SKUPI SVE BLOKOVE MATRICA IZ PROCESA U KONACNU REZULTANTNU MATRICU U ISPRAVNOM REDOSLIJEDU -> GLAVNI REZULTAT SE NALAZI SAMO U MASTER PROCESU
  	MPI_Gatherv(&result_matrix_block[0][0], (N / process_count)* M, MPI_INT, recvPtr, recvCounts, displacements, recvMagicBlock, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	end_time = MPI_Wtime();
	MPI_Finalize();
	if(process_rank == 0) {
		printf("Parallel %d processes time elapsed: %f\n",process_count, end_time - start_time);
	}
	return 0;
}
