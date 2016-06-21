#include <exception>
#include <mutex>

#include <xbt/log.hpp>

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
  
std::cv_status s4u::ConditionVariable::wait_for(std::unique_lock<Mutex>& lock, double timeout) {
  try {
    simcall_cond_wait_timeout(cond_, lock.mutex()->mutex_, timeout);
    return std::cv_status::timeout;
  }
  catch (xbt_ex& e) {
    if (e.category == timeout_error) {
      // We have to take the lock:
      try {
        lock.mutex()->lock();
      }
      catch (...) {
        std::terminate();
      }
      return std::cv_status::timeout;
    }
    std::terminate();
  }
  catch (...) {
    std::terminate();
  }
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

 
