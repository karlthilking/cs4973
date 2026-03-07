#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>

int M0, N0, M1, N1;
int *A, *B, *C;

int
main(void)
{
    srand(time(NULL));

    M0 = (rand() % 4) + 1;
    N0 = (rand() % 4) + 1;
    M1 = N0;
    N1 = (rand() % 4) + 1;
    
    A = malloc(sizeof(int) * M0 * N0);
    B = malloc(sizeof(int) * M1 * N1);
    C = malloc(sizeof(int) * M0 * N1);

    for (int i = 0; i < (M0 * N0); ++i)
        *(A + i) = rand() % 512;
    for (int i = 0; i < (M1 * N1); ++i)
        *(B + i) = rand() % 512;
    
    memset(C, 0, sizeof(int) * M0 * N1);
    for (int i = 0; i < M0; ++i) {
        for (int k = 0; k < M1; ++k) {
            for (int j = 0; j < N1; ++j) {
                /*
                 *  Read value at A[i, k]
                 *  Read value at B[k, j]
                 *  Read value at C[i, j]
                 *  Multiply A[i, k], B[k, j]
                 *  Add C[i, j], C[i, j], A[i, k] * B[k, j]
                 *  Write result to C[i, j]
                 *
                 *  Pseudo Assembly:
                 *      a0 = i, a1 = j, a2 = k
                 *      s1 = A, s2 = B, s3 = C
                 *      s4 = M0, s5 = M1, s6 = N1
                 *      a3 = A[i, k], a4 = B[k, j], a5 = C[i, j]
                 *
                 *      mv      t0, a0
                 *      mul     t0, t0, M0
                 *      add     t0, t0, a2
                 *      muli    t0, t0, 4
                 *      add     a3, s1, t0  # address of A[i, k]
                 *      ...
                 *      ...
                 *      ...
                 *      lw      t0, 0(a3)   # A[i, k]
                 *      lw      t1, 0(a4)   # B[k, j]
                 *      lw      t2, 0(a5)   # C[i, j]
                 *      mul     t0, t0, t1  # A[i, j] * B[k, j]
                 *      add     t2, t2, t0  # C[i, j] += A[i, j] * B[k, j]
                 *      sw      t2, 0(a5)   
                 * 
                 *      3 reads, 1 write
                 */
                *(C+(i*M0)+j) += *(A+(i*M0)+k) * *(B+(k*M1)+j);

            }
        }
    }

    exit(0);
}
