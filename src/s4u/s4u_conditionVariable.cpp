#include <mutex>

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
void s4u::ConditionVariable::wait(std::unique_lock<Mutex>& lock) {
  simcall_cond_wait(cond_, lock.mutex()->mutex_);
}
  
void s4u::ConditionVariable::wait_for(std::unique_lock<Mutex>& lock, double timeout) {
  simcall_cond_wait_timeout(cond_, lock.mutex()->mutex_, timeout);
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

 
