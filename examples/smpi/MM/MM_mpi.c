/*
 * Block Matrix Multiplication example
 *
 * Authors: Quintin Jean-Noël
 */


#include "topo.h"
#include "param.h"
#include "Matrix_init.h"
#include "2.5D_MM.h"
#include "xbt/log.h"

/*int sched_setaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask);
  int sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t *mask);
 */
#include <mpi.h>
#include <math.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>

 XBT_LOG_NEW_DEFAULT_CATEGORY(MM_mpi,
                             "Messages specific for this msg example");



int main(int argc, char ** argv)
{

  size_t    m   = 1024 , n   = 1024 , k = 1024;
  size_t    NB_Block = 16;
  size_t    Block_size = k/NB_Block ;
  size_t    NB_groups = 1, group = 0, key = 0;
  /* x index on M
     y index on N
     Z index on K */



  int myrank;
  int NB_proc;
  size_t row, col, size_row, size_col; //description: vitual processor topology
  row = 0;
  col = 0;

  MPI_Init(&argc, &argv);

  /* Find out my identity in the default communicator */

  MPI_Comm_rank ( MPI_COMM_WORLD, &myrank );
  MPI_Comm_size ( MPI_COMM_WORLD, &NB_proc );

  if(NB_proc != 1)
    for (size_col=NB_proc/2; NB_proc%size_col; size_col--);
  else
    size_col = 1;

  size_row = NB_proc/size_col;
  if (size_row > size_col){
    size_col = size_row;
    size_row = NB_proc/size_col;
  }


  // for the degub
#if DEBUG_MPI
  size_t loop=1;
  while(loop==1);
#endif

  int opt, display = 0;
  char *conf_file = NULL;
  optind = 1;

  //get the parameter from command line
  while ((opt = getopt(argc, argv, "hdf:r:c:M:N:K:B:G:g:k:P:")) != -1) {
    switch(opt) {
      case 'h':
        XBT_INFO(
                    "Usage: mxm_cblas_test [options]\n"
                    "	-M I	M size (default: %zu)\n"
                    "	-N I	N size (default: %zu)\n"
                    "	-K I	K size (default: %zu)\n"
                    "	-B I	Block size on the k dimension(default: %zu)\n"
                    "	-G I	Number of processor groups(default: %zu)\n"
                    "	-g I	group index(default: %zu)\n"
                    "	-k I	group rank(default: %zu)\n"
                    "	-r I	processor row size (default: %zu)\n"
                    "	-c I	processor col size (default: %zu)\n"
                    " -f {Filename} provide the file with the configuration\n"
                    "	-d  display the configuration file on the stderr\n"
                    "	-h	help\n",
                    m, n, k, Block_size, NB_groups, group, key, row, col);
        return 0;
      case 'M':
        m = atoi(optarg);
        break;
      case 'N':
        n   = atoi(optarg);
        break;
      case 'K':
        k  = atoi(optarg);
        break;
      case 'B':
        Block_size = atoi(optarg);
        break;
      case 'G':
        NB_groups = atoi(optarg);
        break;
      case 'g':
        group = atoi(optarg);
        break;
      case 'k':
        key = atoi(optarg);
        break;
      case 'r':
        size_row = atoi(optarg);
        break;
      case 'c':
        size_col = atoi(optarg);
        break;
        /*case 'P':
          str_mask = strdup(optarg);
          break;*/
      case 'f':
        conf_file = strdup(optarg);
        break;
      case 'd':
        display = 1;
        break;
    }
  }
  if( display == 1 ){
    print_conf(MPI_COMM_WORLD, myrank,NULL,NULL);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    exit(0);
  }

  char **conf;
  //char ***conf_all;
  if(conf_file == NULL){
    conf = get_conf(MPI_COMM_WORLD, "default_conf", -1);
  }else{
    conf = get_conf(MPI_COMM_WORLD, conf_file, -1);
    //conf_all = get_conf_all(conf_file);
  }
  if(conf == NULL){
        XBT_INFO(
                "No configuration for me inside the file\n");
  }else{


    if(NB_groups !=1){
      /* Get my group number from the config file */
      group = (size_t)atoi(conf[4]);
    }
  }






  // Defined the device if we use the GPU
  //TODO explain parameters


  two_dot_five( m, k, n, Block_size, group, key,
               size_row, size_col,  NB_groups);

  // close properly the pragram
end:
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}
