#ifndef __GREEDYLBWRAPPER_H
#define __GREEDYLBWRAPPER_H

#ifdef __cplusplus
extern "C"{
#endif

  typedef struct GreedyLB GreedyLB;
  typedef void LDStats;

  GreedyLB *new_GreedyLB();
  
  void GreedyLB_work(GreedyLB *LB, void *stats);

  void *new_LDStats(int count, int complete);

#ifdef __cplusplus
}
#endif


#endif
