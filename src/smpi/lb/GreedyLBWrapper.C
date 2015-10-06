#include <vector>
#include "GreedyLB.h"
#include "GreedyLBWrapper.h"

#ifdef __cplusplus
extern "C"{
#endif

  GreedyLB *new_GreedyLB()
  {
    return new GreedyLB();
  }

  void GreedyLB_work(GreedyLB *LB, void *stats)
  {
    GreedyLB::LDStats *s = (GreedyLB::LDStats *) stats;
    LB->work(s);
  }

#ifdef __cplusplus
} //extern "C"
#endif
