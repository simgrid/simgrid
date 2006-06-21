#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"

int main(int argc, char **argv) {
  
  /* No deployment file
  if (argc < 3) {
     printf ("Usage: %s platform_file deployment_file\n", argv[0]);
     printf ("example: %s msg_platform.xml msg_deployment.xml\n", argv[0]);
     exit(1);
  }
  */

  if (argc < 2) {
     printf ("Usage: %s platform_file\n", argv[0]);
     printf ("example: %s msg_platform.xml\n", argv[0]);
     exit(1);
  }

  /* initialisation of SD */
  SD_init(&argc, argv);

  /* creation of the environment */
  char * platform_file = argv[1];
  SD_create_environment(platform_file);

  /* creation of the tasks and their dependencies */
  

  /* let's launch the simulation! */
  SD_simulate(100);

  SD_clean();
  return 0;
}
