#include <gras.h>

int client(int argc, char *argv[]) {
  gras_init(&argc,argv);

  /* Your own code for the client process */
  
  gras_exit();
  return 0;
}

int server(int argc, char *argv[]) {
  gras_init(&argc,argv);

  /* Your own code for the server process */
  
  gras_exit();
  return 0;
}
