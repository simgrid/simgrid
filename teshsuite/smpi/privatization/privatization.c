
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>


static int myvalue = 0;
static void test_opts(int argc, char* const argv[])
{
  int found = 0;
  static struct option long_options[] = {
  {(char*)"long",     no_argument, 0,  0 },
  {0,         0,                 0,  0 }
  };
  while (1) {
    int ret = getopt_long_only(argc, argv, "s", long_options, NULL);
    if(ret==-1)
      break;

    switch (ret) {
      case 0:
      case 's':
        found ++;
      break;
      default:
        printf("option %s", long_options[0].name);
      break;
    }
  }
  if (found!=2){
    printf("(smpi_)getopt_long_only failed ! \n");
  }
}
int main(int argc, char **argv)
{
    int me;

    MPI_Init(&argc, &argv);
    /* test getopt_long function */
    test_opts(argc, argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &me);

    MPI_Barrier(MPI_COMM_WORLD);

    myvalue = me;

    MPI_Barrier(MPI_COMM_WORLD);

    if(myvalue!=me)
      printf("Privatization error - %d != %d\n", myvalue, me);
    MPI_Finalize();
    return 0;
}
