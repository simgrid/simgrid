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

  void *new_LDStats(int count, int complete){
    return new GreedyLB::LDStats(count, complete);
  }

#ifdef __cplusplus
} //extern "C"
#endif
