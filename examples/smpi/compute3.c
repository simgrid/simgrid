#include <stdio.h>

int main(int argc, char *argv[])
{
  int i;
  double d;
  MPI_Init(&argc, &argv);
  d = 2.0;
  SMPI_DO_ONCE {
    for (i = 0; i < atoi(argv[1]); i++) {
      if (d < 10000) {
        d = d * d;
      } else {
        d = 2;
      }
    }
    printf("%d %f\n", i, d);
  }
  SMPI_DO_ONCE {
    for (i = 0; i < 2 * atoi(argv[1]); i++) {
      if (d < 10000) {
        d = d * d;
      } else {
        d = 2;
      }
    }
    printf("%d %f\n", i, d);
  }
  MPI_Finalize();
  return 0;
}
