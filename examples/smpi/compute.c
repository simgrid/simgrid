#include <stdio.h>

int main(int argc, char *argv[])
{
  int i;
  double d;
  MPI_Init(&argc, &argv);
  d = 2.0;
  for (i = 0; i < atoi(argv[1]); i++) {
    if (d < 10000) {
      d = d * d;
    } else {
      d = 2;
    }
  }
  printf("%d %f\n", i, d);
  MPI_Comm_rank(MPI_COMM_WORLD, &i);
  MPI_Finalize();
  return 0;
}
