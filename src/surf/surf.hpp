//using namespace generic;

#ifndef SURF_MODEL_H_
#define SURF_MODEL_H_

#include <xbt.h>
#include <string>
#include <vector>
#include <memory>
#include <boost/smart_ptr.hpp>
#include <boost/function.hpp>
#include <boost/functional/factory.hpp>
#include <boost/bind.hpp>
#include "surf/trace_mgr.h"
#include "xbt/lib.h"
#include "surf/surf_routing.h"
#include "simgrid/platf_interface.h"

extern tmgr_history_t history;
#define NO_MAX_DURATION -1.0

using namespace std;

// TODO: put in surf_private.hpp
extern xbt_dict_t watched_hosts_lib;

/** \ingroup SURF_simulation
 *  \brief Return the current time
 *
 *  Return the current time in millisecond.
 */

/*********
 * Utils *
 *********/

#ifdef __cplusplus
extern "C" {
#endif
XBT_PUBLIC(double) surf_get_clock(void);
XBT_PUBLIC(void) surf_watched_hosts(void);
#ifdef __cplusplus
}
#endif

XBT_PUBLIC(int)  SURF_CPU_LEVEL;    //Surf cpu level

/***********
 * Classes *
 ***********/
class Model;
typedef Model* ModelPtr;

class Resource;
typedef Resource* ResourcePtr;
typedef boost::function<void (ResourcePtr r)> ResourceCallback;
			
class Action;
typedef Action* ActionPtr;
typedef boost::function<void (ActionPtr a)> ActionCallback;

/*********
 * Trace *
 *********/
/* For the trace and trace:connect tag (store their content till the end of the parsing) */
XBT_PUBLIC_DATA(xbt_dict_t) traces_set_list;
XBT_PUBLIC_DATA(xbt_dict_t) trace_connect_list_host_avail;
XBT_PUBLIC_DATA(xbt_dict_t) trace_connect_list_power;
XBT_PUBLIC_DATA(xbt_dict_t) trace_connect_list_link_avail; 
XBT_PUBLIC_DATA(xbt_dict_t) trace_connect_list_bandwidth; 
XBT_PUBLIC_DATA(xbt_dict_t) trace_connect_list_latency;


/*********
 * Model *
 *********/
XBT_PUBLIC_DATA(xbt_dynar_t) model_list;

class Model {
public:
  Model(string name) {
    m_name = name;
    m_resOnCB = m_resOffCB= 0;
    m_actSuspendCB = m_actCancelCB = m_actResumeCB = 0;
  }
  virtual ~Model() {
    xbt_swag_free(p_readyActionSet);
    xbt_swag_free(p_runningActionSet);
    xbt_swag_free(p_failedActionSet);
    xbt_swag_free(p_doneActionSet);
  }
  ResourcePtr createResource(string name);
  ActionPtr createAction(double _cost, bool _failed);
  double shareResources(double now);
  void updateActionsState(double now, double delta);

  string getName() {return m_name;};

  void addTurnedOnCallback(ResourceCallback rc);
  void notifyResourceTurnedOn(ResourcePtr r);

  void addTurnedOffCallback(ResourceCallback rc);  
  void notifyResourceTurnedOff(ResourcePtr r);

  void addActionCancelCallback(ActionCallback ac);
  void notifyActionCancel(ActionPtr a);
  void addActionResumeCallback(ActionCallback ac);
  void notifyActionResume(ActionPtr a);
  void addActionSuspendCallback(ActionCallback ac);  
  void notifyActionSuspend(ActionPtr a);

  xbt_swag_t p_readyActionSet; /**< Actions in state SURF_ACTION_READY */
  xbt_swag_t p_runningActionSet; /**< Actions in state SURF_ACTION_RUNNING */
  xbt_swag_t p_failedActionSet; /**< Actions in state SURF_ACTION_FAILED */
  xbt_swag_t p_doneActionSet; /**< Actions in state SURF_ACTION_DONE */

protected:
  std::vector<ActionPtr> m_failedActions, m_runningActions;

private:
  string m_name;
  ResourceCallback m_resOnCB, m_resOffCB;
  ActionCallback m_actCancelCB, m_actSuspendCB, m_actResumeCB;
};

/************
 * Resource *
 ************/
class Resource {
public:
  Resource(ModelPtr model, const char *name, xbt_dict_t properties):
	  m_name(name),m_running(true),p_model(model),m_properties(properties) {};
  virtual ~Resource() {};

  void updateState(tmgr_trace_event_t event_type, double value, double date);

  //private
  bool isUsed();
  //TODOupdateActionState();
  //TODOupdateResourceState();
  //TODOfinilize();

  bool isOn();
  void turnOn();
  void turnOff();
  void setName(string name);
  string getName();
  ModelPtr getModel() {return p_model;};
  void printModel() { std::cout << p_model->getName() << "<<plop"<<std::endl;};
  void *p_resource;
  e_surf_resource_state_t m_stateCurrent;

protected:
  ModelPtr p_model;
  const char *m_name;
  xbt_dict_t m_properties;

private:
  bool m_running;  
};

static inline void *surf_cpu_resource_priv(const void *host) {
  return xbt_lib_get_level((xbt_dictelm_t)host, SURF_CPU_LEVEL);
}
/*static inline void *surf_workstation_resource_priv(const void *host){
  return xbt_lib_get_level((xbt_dictelm_t)host, SURF_WKS_LEVEL); 
}        
static inline void *surf_storage_resource_priv(const void *host){
  return xbt_lib_get_level((xbt_dictelm_t)host, SURF_STORAGE_LEVEL);
}*/

static inline void *surf_cpu_resource_by_name(const char *name) {
  return xbt_lib_get_elm_or_null(host_lib, name);
}
/*static inline void *surf_workstation_resource_by_name(const char *name){
  return xbt_lib_get_elm_or_null(host_lib, name);
}
static inline void *surf_storage_resource_by_name(const char *name){
  return xbt_lib_get_elm_or_null(storage_lib, name);*/

/**********
 * Action *
 **********/

/** \ingroup SURF_actions
 *  \brief Action states
 *
 *  Action states.
 *
 *  \see surf_action_t, surf_action_state_t
 */
extern const char *surf_action_state_names[6];

typedef enum {
  SURF_ACTION_READY = 0,        /**< Ready        */
  SURF_ACTION_RUNNING,          /**< Running      */
  SURF_ACTION_FAILED,           /**< Task Failure */
  SURF_ACTION_DONE,             /**< Completed    */
  SURF_ACTION_TO_FREE,          /**< Action to free in next cleanup */
  SURF_ACTION_NOT_IN_THE_SYSTEM
                                /**< Not in the system anymore. Why did you ask ? */
} e_surf_action_state_t;

typedef enum {
  UM_FULL,
  UM_LAZY,
  UM_UNDEFINED
} e_UM_t;

class Action {
public:
  Action(ModelPtr model, double cost, bool failed):
	 m_cost(cost), p_model(model), m_failed(failed),
	 m_refcount(1), m_priority(1.0), m_maxDuration(NO_MAX_DURATION),
	 m_start(surf_get_clock()), m_finish(-1.0)
  {
    m_priority = m_start = m_finish = m_maxDuration = -1.0;
    m_start = 10;//surf_get_clock();
    m_suspended = false;
  };
  virtual ~Action() {};
  
  e_surf_action_state_t getState(); /**< get the state*/
  void setState(e_surf_action_state_t state); /**< Change state*/
  double getStartTime(); /**< Return the start time of an action */
  double getFinishTime(); /**< Return the finish time of an action */
  void setData(void* data);

  virtual int unref()=0;     /**< Specify that we don't use that action anymore. Returns true if the action was destroyed and false if someone still has references on it. */
  virtual void cancel()=0;     /**< Cancel a running action */
  virtual void recycle()=0;     /**< Recycle an action */
  
  void suspend();     /**< Suspend an action */
  void resume();     /**< Resume a suspended action */
  bool isSuspended();     /**< Return whether an action is suspended */
  void setMaxDuration(double duration);     /**< Set the max duration of an action*/
  void setPriority(double priority);     /**< Set the priority of an action */
#ifdef HAVE_TRACING
  void setCategory(const char *category); /**< Set the category of an action */
#endif
  double getRemains();     /**< Get the remains of an action */
#ifdef HAVE_LATENCY_BOUND_TRACKING
  int getLatencyLimited();     /**< Return 1 if action is limited by latency, 0 otherwise */
#endif
  xbt_swag_t p_stateSet;

  double m_priority; /**< priority (1.0 by default) */
  bool m_failed;
  bool m_suspended;  
  double m_start; /**< start time  */
  double m_finish; /**< finish time : this is modified during the run and fluctuates until the task is completed */
  double m_remains; /**< How much of that cost remains to be done in the currently running task */
  #ifdef HAVE_LATENCY_BOUND_TRACKING
  int m_latencyLimited;               /**< Set to 1 if is limited by latency, 0 otherwise */
  #endif
  double m_maxDuration; /*< max_duration (may fluctuate until the task is completed) */  
protected:
  ModelPtr p_model;  
  int    m_cost;
  int    m_refcount;
  void *p_data; /**< for your convenience */
#ifdef HAVE_TRACING
  char *p_category;               /**< tracing category for categorized resource utilization monitoring */
#endif

private:
  int resourceUsed(void *resource_id);
  /* Share the resources to the actions and return in how much time
     the next action may terminate */
  double shareResources(double now);
  /* Update the actions' state */
  void updateActionsState(double now, double delta);
  void updateResourceState(void *id, tmgr_trace_event_t event_type,
                                 double value, double time);
  void finalize(void);

  lmm_system_t p_maxminSystem;
  e_UM_t p_updateMechanism;
  xbt_swag_t p_modifiedSet;
  xbt_heap_t p_actionHeap;
  int m_selectiveUpdate;
};

#endif /* SURF_MODEL_H_ */
