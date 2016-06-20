#include "simgrid/s4u/conditionVariable.hpp"
#include "simgrid/simix.h"

using namespace simgrid;

s4u::ConditionVariable::ConditionVariable()  : cond_(simcall_cond_init()){
    
}

s4u::ConditionVariable::~ConditionVariable() {
  SIMIX_cond_unref(cond_);
}

/**
 * Wait functions
 */
void s4u::ConditionVariable::wait(s4u::Mutex *mutex) {
  simcall_cond_wait(cond_, mutex->mutex_);
}
  
void s4u::ConditionVariable::wait_for(s4u::Mutex *mutex, double timeout) {
  simcall_cond_wait_timeout(cond_, mutex->mutex_, timeout);
}
  
/**
 * Notify functions
 */
void s4u::ConditionVariable::notify() { 
   simcall_cond_signal(cond_);
}
 
void s4u::ConditionVariable::notify_all() {
  simcall_cond_broadcast(cond_);
}

 
