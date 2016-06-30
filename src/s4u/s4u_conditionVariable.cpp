#include <exception>
#include <mutex>

#include <xbt/log.hpp>

#include "simgrid/s4u/conditionVariable.hpp"
#include "simgrid/simix.h"

namespace simgrid {
namespace s4u {

ConditionVariable::ConditionVariable()  : cond_(simcall_cond_init()){
    
}

ConditionVariable::~ConditionVariable() {
  SIMIX_cond_unref(cond_);
}

/**
 * Wait functions
 */
void ConditionVariable::wait(std::unique_lock<Mutex>& lock) {
  simcall_cond_wait(cond_, lock.mutex()->mutex_);
}

std::cv_status s4u::ConditionVariable::wait_for(std::unique_lock<Mutex>& lock, double timeout) {
  try {
    simcall_cond_wait_timeout(cond_, lock.mutex()->mutex_, timeout);
    return std::cv_status::no_timeout;
  }
  catch (xbt_ex& e) {

    // If the exception was a timeout, we have to take the lock again:
    if (e.category == timeout_error) {
      try {
        lock.mutex()->lock();
        return std::cv_status::timeout;
      }
      catch (...) {
        std::terminate();
      }
    }

    // Another exception: should we reaquire the lock?
    std::terminate();
  }
  catch (...) {
    std::terminate();
  }
}

std::cv_status ConditionVariable::wait_until(std::unique_lock<Mutex>& lock, double timeout_time)
{
  double now = SIMIX_get_clock();
  double timeout;
  if (timeout_time < now)
    timeout = 0.0;
  else
    timeout = timeout_time - now;
  return this->wait_for(lock, timeout);
}
  
/**
 * Notify functions
 */
void ConditionVariable::notify_one() {
   simcall_cond_signal(cond_);
}
 
void ConditionVariable::notify_all() {
  simcall_cond_broadcast(cond_);
}

}
}
