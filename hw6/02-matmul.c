#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#define N 64

static int A[N * N], B[N *N], C[N * N];

int
main(int argc, char *argv[])
{
    const int n_iterations = (argc > 1) ? atoi(argv[1]) : 1 << 10;
    
    srand(time(NULL));
    for (int i = 0; i < (N * N); ++i)
        *(A + i) = rand() % 512;
    for (int i = 0; i < (N * N); ++i)
        *(B + i) = rand() % 512;

    memset(C, 0, sizeof(int) * N * N);
    
    for (int iter = 0; iter < n_iterations; ++iter) {
        for (int i = 0; i < N; ++i) {
            for (int k = 0; k < N; ++k) {
                for (int j = 0; j < N; ++j) {
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
                     *      mul     t0, t0, t1  # A[i, j]*B[k, j]
                     *      add     t2, t2, t0  # C[i, j] += A[i, j]*B[k, j]
                     *      sw      t2, 0(a5)   
                     * 
                     *      3 reads, 1 write
                     */
                    *(C+(i*N)+j) += *(A+(i*N)+k) * *(B+(k*N)+j);

                }
            }
        }
    }
    exit(0);
}
