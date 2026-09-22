// Farha Ferdous
// CSCI 340 Project 2

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

// structure to pass arguments to threads
// pointers to each matrix, dimensions of matrices, first thru last rows to compute
typedef struct {
    int **A;
    int **B;
    int **C;
    int N;
    int start_row;
    int end_row;
} ThreadArgs;

// --function for worker threads--
void *multiply_rows(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    
    // looping through assigned rows
    for (int i = args->start_row; i < args->end_row; i++) {
        for (int j = 0; j < args->N; j++) { // looping thru the columns
            args->C[i][j] = 0;
            for (int k = 0; k < args->N; k++) { // dot product of row i of A and column j of B
                args->C[i][j] += args->A[i][k] * args->B[k][j];
            }
        }
    }
    
    pthread_exit(NULL);
}

// --reading matrix from file--
void read_matrix(FILE *file, int **matrix, int N) {
    // reading the N by N elements from file
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (fscanf(file, "%d", &matrix[i][j]) != 1) {  // checking one by one
                fprintf(stderr, "Error reading matrix data\n");
                exit(1);
            }
        }
    }
}

// --write matrix to file--
void write_matrix(FILE *file, int **matrix, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fprintf(file, "%d\n", matrix[i][j]);  // checking one by one
        }
    }
}

int main(int argc, char *argv[]) {
    // (1) checking command line arguments
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <numThreads> <N> <filename for A> <filename for B> <filename for C>\n", argv[0]);
        return 1;
    }

    // parsing threads
    int numThreads = atoi(argv[1]);
    // making surethread count is from 1-8
    if (numThreads < 1 || numThreads > 8) {
        fprintf(stderr, "Number of threads must be between 1 and 8\n");
        return 1;
    }

    // parsing matric dimension
    int N = atoi(argv[2]);
    char *fileA = argv[3];
    char *fileB = argv[4];
    char *fileC = argv[5];

    // (2) allocating space for matrices A, B, and C
    int **A = (int **)malloc(N * sizeof(int *));
    int **B = (int **)malloc(N * sizeof(int *));
    int **C = (int **)malloc(N * sizeof(int *));
    
    // checking for successful allocation
    if (A == NULL || B == NULL || C == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    // allocating space for each row
    for (int i = 0; i < N; i++) {
        A[i] = (int *)malloc(N * sizeof(int));
        B[i] = (int *)malloc(N * sizeof(int));
        C[i] = (int *)malloc(N * sizeof(int));
        if (A[i] == NULL || B[i] == NULL || C[i] == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
    }

    // (3a) reading matrices A from file and close the file
    FILE *fpA = fopen(fileA, "r");
    if (fpA == NULL) {
        fprintf(stderr, "Can't open file %s\n", fileA);
        exit(1);
    }
    read_matrix(fpA, A, N);
    fclose(fpA);

    // (3a) reading matrix B from file and close the file
    FILE *fpB = fopen(fileB, "r");
    if (fpB == NULL) {
        fprintf(stderr, "Can't open file %s\n", fileB);
        exit(1);
    }
    read_matrix(fpB, B, N);
    fclose(fpB);

    // (4) start timer (use gettimeofday)
    struct timeval start, end;
    gettimeofday(&start, NULL);

    // (5) create worker threads to compute the result (matrix C)
    pthread_t threads[numThreads];
    ThreadArgs threadArgs[numThreads];
    // calculating work distrobutoion 
    int rows_per_thread = N / numThreads;  // rows per thread
    int remaining_rows = N % numThreads;  // xtra rows
    int current_row = 0;  // tracks next row to assign

    // crwating and launching threads
    for (int i = 0; i < numThreads; i++) {
        threadArgs[i].A = A;
        threadArgs[i].B = B;
        threadArgs[i].C = C;
        threadArgs[i].N = N;
        threadArgs[i].start_row = current_row;
        
        // dividing up remaining rows to the first threads
        int extra_row = (i < remaining_rows) ? 1 : 0;
        threadArgs[i].end_row = current_row + rows_per_thread + extra_row;
        
        current_row = threadArgs[i].end_row;
        
        // creating thread
        if (pthread_create(&threads[i], NULL, multiply_rows, &threadArgs[i])) {
            fprintf(stderr, "Error creating thread\n");
            exit(1);
        }
    }

    // (6) wait for the termination of the worker threads
    for (int i = 0; i < numThreads; i++) {
        if (pthread_join(threads[i], NULL)) {
            fprintf(stderr, "Error joining thread\n");
            exit(1);
        }
    }

    // (7) stop the timer (use gettimeofday)
    gettimeofday(&end, NULL);
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double elapsed = seconds + microseconds * 1e-6;

    // (8) print the total execution time 
    printf("Execution time: %.6f seconds\n", elapsed);

    // (9) write matrix C to the output file
    FILE *fpC = fopen(fileC, "w");
    if (fpC == NULL) {
        fprintf(stderr, "Can't create file %s\n", fileC);
        exit(1);
    }
    write_matrix(fpC, C, N);
    fclose(fpC);

    // (10) free up allocated spaces for matrices A, B, and C
    for (int i = 0; i < N; i++) {
        free(A[i]);
        free(B[i]);
        free(C[i]);
    }
    free(A);
    free(B);
    free(C);

    return 0;
}