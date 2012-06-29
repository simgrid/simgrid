#include "mc/mc.h"
#include "msg/msg.h"

int main (int argc, char **argv){
  MSG_init(&argc, argv);
  MC_test_snapshot_comparison();
  return 0;
}
