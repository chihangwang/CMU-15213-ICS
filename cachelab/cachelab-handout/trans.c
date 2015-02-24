/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * Name: Chih-Ang Wang
 * Andrew ID: chihangw
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    REQUIRES(M > 0);
    REQUIRES(N > 0);

    // blocking technique by row_block, col_block. For finer granularity
    // use additional mini_row_block, mini_col_block
    int i, j, row_block, col_block, mini_row_block, mini_col_block;
    // diagonal index
    int diag = 0;
    // diagonal value
    int temp = 0;

    if (N == 32 && M == 32) 
    {
        for (col_block = 0; col_block < M; col_block += 8) 
        {
            for (row_block = 0; row_block < N; row_block += 8) 
            {
                for (i = row_block; i < row_block + 8; i++) 
                {
                    for (j = col_block; j < col_block + 8; j++) 
                    {
                        if (i != j)// not diagonal, transpose directly
                        {
                            B[j][i] = A[i][j];
                        }
                         else {// diagonal element, defer the storage
                            temp = A[i][j];
                            diag = i;
                        }
                    }

                    if (row_block == col_block) 
                    {
                        B[diag][diag] = temp;
                    }
                }   
            }
        }
    } 

    else if (N == 64 && M == 64) 
    {
       for (col_block = 0; col_block < M; col_block += 8)
       {
          for (row_block = 0; row_block < N; row_block += 8)
	  {
             for (mini_col_block = col_block; mini_col_block < col_block + 8; \
							 mini_col_block += 4)
	     {
                for (mini_row_block = row_block; mini_row_block < row_block + 8;\
							  mini_row_block += 4)
		{
                    for (i = mini_row_block; i < mini_row_block + 4; i++)
		    {
                        for (j = mini_col_block; j < mini_col_block + 4; j++)
			{
                            if (i != j)
			    {   // diagonal will cause conflict miss
                                B[j][i] = A[i][j];
                            }
			    else {
				// if not diagonal element, transpose directly
                                diag = i;
				temp = A[i][j];
                            }
                        }
			if (mini_col_block == mini_row_block)
			{
                            B[diag][diag] = temp;
			}
                    }
                 }
              }
           }
        }
    } 
    else {

        for (col_block = 0; col_block < M; col_block += 16) 
        {
            for (row_block = 0; row_block < N; row_block += 16) 
            {       
                for (i = row_block; (i < row_block + 16) && (i < N); i++) 
                {
                    for (j = col_block; (j < col_block + 16) && (j < M); j++) 
                    {
                        if (i != j) 
                        {
                            B[j][i] = A[i][j];
                        }
                        else 
                        {
                            temp = A[i][j];
                            diag = i;
                        }
                    }

                    if (row_block == col_block) {
                        B[diag][diag] = temp;
                    }
                }
            }
        }
    }

    ENSURES(is_transpose(M, N, A, B));
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
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
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
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

