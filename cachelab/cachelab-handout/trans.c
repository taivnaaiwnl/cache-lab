/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
//22B1NUM6769 Taivanbat Bayarsaikhan 

#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]);

void transpose_32bit(int M, int N, int A[N][M], int B[M][N]);
void transpose_64bit(int M, int N, int A[N][M], int B[M][N]);
void transpose_61bit(int M, int N, int A[N][M], int B[M][N]);

void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    if (M == 32 && N == 32) {
        transpose_32bit(M, N, A, B);
    } else if (M == 64 && N == 64) {
        transpose_64bit(M, N, A, B);
    } else if (M == 61 && N == 67) {
        transpose_61bit(M, N, A, B);
    } else {
        int i, j, tmp;
        for (i = 0; i < N; i++) {
            for (j = 0; j < M; j++) {
                tmp = A[i][j];
                B[j][i] = tmp;
            }
        }
    }
}

void transpose_32bit(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, ii, jj;
    int tmp;
    int block_size = 8;
    
    for (ii = 0; ii < N; ii += block_size) {
        for (jj = 0; jj < M; jj += block_size) {
            for (i = ii; i < ii + block_size && i < N; i++) {
                for (j = jj; j < jj + block_size && j < M; j++) {
                    if (i != j) {
                        B[j][i] = A[i][j];
                    } else {
                        tmp = A[i][j];
                        B[j][i] = tmp;
                    }
                }
            }
        }
    }
}

void transpose_64bit(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, k;
    int v0, v1, v2, v3, v4, v5, v6, v7;
    
    for (i = 0; i < N; i += 8) {
        for (j = 0; j < M; j += 8) {
            for (k = 0; k < 4; k++) {
                v0 = A[i+k][j];
                v1 = A[i+k][j+1];
                v2 = A[i+k][j+2];
                v3 = A[i+k][j+3];
                v4 = A[i+k][j+4];
                v5 = A[i+k][j+5];
                v6 = A[i+k][j+6];
                v7 = A[i+k][j+7];
                
                B[j][i+k] = v0;
                B[j+1][i+k] = v1;
                B[j+2][i+k] = v2;
                B[j+3][i+k] = v3;
                B[j][i+k+4] = v4;
                B[j+1][i+k+4] = v5;
                B[j+2][i+k+4] = v6;
                B[j+3][i+k+4] = v7;
            }
            
            for (k = 0; k < 4; k++) {
                v0 = A[i+4+k][j];
                v1 = A[i+4+k][j+1];
                v2 = A[i+4+k][j+2];
                v3 = A[i+4+k][j+3];
                v4 = A[i+4+k][j+4];
                v5 = A[i+4+k][j+5];
                v6 = A[i+4+k][j+6];
                v7 = A[i+4+k][j+7];
                
                B[j+4][i+k] = v0;
                B[j+5][i+k] = v1;
                B[j+6][i+k] = v2;
                B[j+7][i+k] = v3;
                B[j+4][i+k+4] = v4;
                B[j+5][i+k+4] = v5;
                B[j+6][i+k+4] = v6;
                B[j+7][i+k+4] = v7;
            }
        }
    }
}

void transpose_61bit(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, ii, jj;
    int block_size = 16;
    
    for (ii = 0; ii < N; ii += block_size) {
        for (jj = 0; jj < M; jj += block_size) {
            for (i = ii; i < ii + block_size && i < N; i++) {
                for (j = jj; j < jj + block_size && j < M; j++) {
                    B[j][i] = A[i][j];
                }
            }
        }
    }
}

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}