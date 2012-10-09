#include <mpi.h>
double Summa(
                     double *a, double *b, double *c,
                     size_t lda, size_t ldb, size_t ldc,
                     size_t m, size_t k_a, size_t k_b, size_t n,
                     size_t Block_size, size_t start, size_t end,
                     size_t row, size_t col, size_t size_row, size_t size_col,
                     double *a_local, double *b_local,
                     MPI_Datatype Block_a, MPI_Datatype Block_a_local,
                     MPI_Datatype Block_b,
                     MPI_Comm row_comm, MPI_Comm col_comm, int subs);

