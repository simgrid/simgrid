/*!
 * Classical Block Matrix Multiplication example
 *
 * Authors: Quintin Jean-Noël
 */
#include "Matrix_init.h"
#include "Summa.h"
#include "timer.h"
#include "xbt/log.h"
 XBT_LOG_NEW_DEFAULT_CATEGORY(MM_Summa,
                             "Messages specific for this msg example");

inline double Summa(
                     double *a, double *b, double *c,
                     size_t lda, size_t ldb, size_t ldc,
                     size_t m, size_t k_a, size_t k_b, size_t n,
                     size_t Block_size, size_t start, size_t end,
                     size_t row, size_t col, size_t size_row, size_t size_col,
                     double *a_local, double *b_local,
                     MPI_Datatype Block_a, MPI_Datatype Block_a_local,
                     MPI_Datatype Block_b,
                     MPI_Comm row_comm, MPI_Comm col_comm, int subs)
{
  double *B_a     , *B_b     ; //matrix blocks
  size_t err;
  double alpha = 1, beta = 1;  //C := alpha * a * b + beta * c
  size_t B_proc_col, B_proc_row; // Number of bloc(row or col) on one processor
  B_proc_col =  k_b / Block_size;  // Number of block on one processor
  B_proc_row = k_a / Block_size; // Number of block on one processor

  //size_t lda = k_a, ldb = n, ldc = n;
  size_t lda_local = lda;
  size_t ldb_local = ldb;


  double time, computation_time = 0, communication_time = 0;
  struct timespec start_time, end_time; //time mesure
  struct timespec start_time_intern, end_time_intern; //time mesure




  get_time(&start_time);

  /*-------------Distributed Matrix Multiplication algorithm-----------------*/
  size_t iter;
  for( iter = start; iter < end; iter++ ){
    size_t pivot_row, pivot_col, pos_a, pos_b;
#ifdef CYCLIC
    // pivot row on processor layer
    pivot_row = (iter % size_col);
    pivot_col = (iter % size_row);
    //position of the block
    if(subs == 1){
      pos_a = (size_t)((iter - start) / size_row) * Block_size;
      pos_b = (size_t)((iter - start) / size_col) * ldb * Block_size;
    }else{
      pos_a = (size_t)(iter / size_row) * Block_size;
      pos_b = (size_t)(iter / size_col) * ldb * Block_size;
    }
#else
    // pivot row on processor layer
    pivot_row = (size_t)(iter / B_proc_col) % size_col;
    pivot_col = (size_t)(iter / B_proc_row) % size_row;
    //position of the block
    if(subs == 1){
      pos_a = ((iter - start) % B_proc_row) * Block_size;
      pos_b = ((iter - start) % B_proc_col) * ldb * Block_size;
    }else{
      pos_a = (iter % B_proc_row) * Block_size;
      pos_b = (iter % B_proc_col) * ldb * Block_size;
    }
#endif
    XBT_DEBUG( "pivot: %zu, iter: %zu, B_proc_col: %zu, "
                "size_col:%zu, size_row: %zu\n",
                pivot_row, iter, B_proc_row,size_col,size_row);
    MPI_Barrier(row_comm);
    MPI_Barrier(col_comm);

    get_time(&start_time_intern);
    //Broadcast the row
    if(size_row > 1){
      MPI_Datatype * Block;
      if( pivot_col != col ){
        B_a = a_local;
        lda_local = Block_size;
        XBT_DEBUG("recieve B_a %zu,%zu \n",m , Block_size);
        Block = &Block_a_local;
      }else{
        B_a = a + pos_a;
        lda_local = lda;
        XBT_DEBUG("sent B_a %zu,%zu \n",m , Block_size);
        Block = &Block_a;
      }
      err = MPI_Bcast(B_a, 1, *Block, pivot_col, row_comm);
      if (err != MPI_SUCCESS) {
        perror("Error Bcast A\n");
        return -1;
      }
    }else{
      B_a = a + pos_a;
      XBT_DEBUG("position of B_a: %zu \n", pos_a);
    }

    //Broadcast the col
    if(size_col > 1){
      if( pivot_row == row ){
        B_b = b + pos_b;
        XBT_DEBUG("sent B_b Block_size: %zu, pos:%zu \n",
                    ldb, pos_b);
      }else{
        B_b = b_local;
        XBT_DEBUG("recieve B_b %zu,%zu \n", Block_size,n);
      }
      err = MPI_Bcast(B_b, 1, Block_b, pivot_row, col_comm );
      if (err != MPI_SUCCESS) {
        perror("Error Bcast B\n");
        MPI_Finalize();
        exit(-1);
      }
    }else{
      B_b = b + pos_b;
      XBT_DEBUG("position of B_b: %zu \n", pos_b);
    }
    get_time(&end_time_intern);
    communication_time += get_timediff(&start_time_intern,&end_time_intern);

    MPI_Barrier(row_comm);
    MPI_Barrier(col_comm);
    get_time(&start_time_intern);
    XBT_DEBUG("execute Gemm number: %zu\n", iter);
    //We have recieved a line of block and a colomn
   //              cblas_dgemm( CblasRowMajor, CblasNoTrans, CblasNoTrans,
   //               m, n, Block_size, alpha, B_a, lda_local, B_b, ldb_local,
   //               beta, c, ldc );
    int i, j, k;
    for(i = 0; i < m; i++)
      for(j = 0; j < n; j++)
        for(k = 0; k < Block_size; k++)
          c[i*ldc+j] += B_a[j*lda_local+k]*B_b[k*ldb_local+j];

    get_time(&end_time_intern);
    computation_time += get_timediff(&start_time_intern,&end_time_intern);

  }
  MPI_Barrier(row_comm);
  MPI_Barrier(col_comm);

  get_time(&end_time);
  time = get_timediff(&start_time,&end_time);
  printf("communication time: %le nanoseconds, "
         "computation time: %le nanoseconds\n",
         communication_time, computation_time);


  return time;
}
