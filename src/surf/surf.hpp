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
#include "surf/surf.h"
#include "surf/surf_private.h"

extern tmgr_history_t history;
#define NO_MAX_DURATION -1.0

using namespace std;

/** \ingroup SURF_simulation
 *  \brief Return the current time
 *
 *  Return the current time in millisecond.
 */

/*********
 * Utils *
 *********/

/* user-visible parameters */
extern double sg_tcp_gamma;
extern double sg_sender_gap;
extern double sg_latency_factor;
extern double sg_bandwidth_factor;
extern double sg_weight_S_parameter;
extern int sg_network_crosstraffic;
#ifdef HAVE_GTNETS
extern double sg_gtnets_jitter;
extern int sg_gtnets_jitter_seed;
#endif
extern xbt_dynar_t surf_path;

#ifdef __cplusplus
extern "C" {
#endif
XBT_PUBLIC(double) surf_get_clock(void);
XBT_PUBLIC(void) surf_watched_hosts(void);
#ifdef __cplusplus
}
#endif

extern double sg_sender_gap;
XBT_PUBLIC(int)  SURF_CPU_LEVEL;    //Surf cpu level

int __surf_is_absolute_file_path(const char *file_path);

/***********
 * Classes *
 ***********/
//class Model;
typedef Model* ModelPtr;

//class Resource;
typedef Resource* ResourcePtr;
typedef boost::function<void (ResourcePtr r)> ResourceCallback;
			
//class Action;
typedef Action* ActionPtr;
typedef boost::function<void (ActionPtr a)> ActionCallback;

//class ActionLmm;
typedef ActionLmm* ActionLmmPtr;

enum heap_action_type{
  LATENCY = 100,
  MAX_DURATION,
  NORMAL,
  NOTSET
};

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
  Model(string name);
  virtual ~Model();

  ResourcePtr createResource(string name);
  ActionPtr createAction(double _cost, bool _failed);
  virtual double shareResources(double now);
  virtual double shareResourcesLazy(double now);
  virtual double shareResourcesFull(double now);
  double shareResourcesMaxMin(xbt_swag_t running_actions,
                                      lmm_system_t sys,
                                      void (*solve) (lmm_system_t));
  virtual void updateActionsState(double now, double delta);
  virtual void updateActionsStateLazy(double now, double delta);
  virtual void updateActionsStateFull(double now, double delta);

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

  lmm_system_t p_maxminSystem;
  e_UM_t p_updateMechanism;
  xbt_swag_t p_modifiedSet;
  xbt_heap_t p_actionHeap;
  int m_selectiveUpdate;

  xbt_swag_t p_readyActionSet; /**< Actions in state SURF_ACTION_READY */
  xbt_swag_t p_runningActionSet; /**< Actions in state SURF_ACTION_RUNNING */
  xbt_swag_t p_failedActionSet; /**< Actions in state SURF_ACTION_FAILED */
  xbt_swag_t p_doneActionSet; /**< Actions in state SURF_ACTION_DONE */
  string m_name;

protected:
  std::vector<ActionPtr> m_failedActions, m_runningActions;

private:
  ResourceCallback m_resOnCB, m_resOffCB;
  ActionCallback m_actCancelCB, m_actSuspendCB, m_actResumeCB;
};

/************
 * Resource *
 ************/

/**
 * Resource which have a metric handled by a maxmin system
 */
typedef struct {
  double scale;
  double peak;
  tmgr_trace_event_t event;
} s_surf_metric_t;

class Resource {
public:
  Resource();
  Resource(ModelPtr model, const char *name, xbt_dict_t properties);
  virtual ~Resource() {};

  virtual void updateState(tmgr_trace_event_t event_type, double value, double date)=0;

  //private
  virtual bool isUsed()=0;
  //FIXME:updateActionState();
  //FIXME:updateResourceState();
  //FIXME:finilize();

  bool isOn();
  void turnOn();
  void turnOff();
  void setName(string name);
  const char *getName();
  virtual xbt_dict_t getProperties();

  ModelPtr getModel() {return p_model;};
  virtual e_surf_resource_state_t getState();
  void printModel() { std::cout << p_model->getName() << "<<plop"<<std::endl;};
  void *p_resource;
  const char *m_name;
  xbt_dict_t m_properties;
  ModelPtr p_model;
  e_surf_resource_state_t p_stateCurrent;

protected:

private:
  bool m_running;  
};

class ResourceLmm: virtual public Resource {
public:
  ResourceLmm() {};
  ResourceLmm(surf_model_t model, const char *name, xbt_dict_t props,
                           lmm_system_t system,
                           double constraint_value,
                           tmgr_history_t history,
                           e_surf_resource_state_t state_init,
                           tmgr_trace_t state_trace,
                           double metric_peak,
                           tmgr_trace_t metric_trace);
  lmm_constraint_t p_constraint;
  tmgr_trace_event_t p_stateEvent;
  s_surf_metric_t p_power;
};

/**********
 * Action *
 **********/

class Action {
public:
  Action();
  Action(ModelPtr model, double cost, bool failed);
  virtual ~Action();
  
  s_xbt_swag_hookup_t p_stateHookup;

  e_surf_action_state_t getState(); /**< get the state*/
  virtual void setState(e_surf_action_state_t state); /**< Change state*/
  double getStartTime(); /**< Return the start time of an action */
  double getFinishTime(); /**< Return the finish time of an action */
  void setData(void* data);

  void ref();
  virtual int unref();     /**< Specify that we don't use that action anymore. Returns true if the action was destroyed and false if someone still has references on it. */
  virtual void cancel();     /**< Cancel a running action */
  virtual void recycle();     /**< Recycle an action */
  
  virtual void suspend()=0;     /**< Suspend an action */
  virtual void resume()=0;     /**< Resume a suspended action */
  virtual bool isSuspended()=0;     /**< Return whether an action is suspended */
  virtual void setMaxDuration(double duration)=0;     /**< Set the max duration of an action*/
  virtual void setPriority(double priority)=0;     /**< Set the priority of an action */
#ifdef HAVE_TRACING
  void setCategory(const char *category); /**< Set the category of an action */
#endif
  virtual double getRemains();     /**< Get the remains of an action */
#ifdef HAVE_LATENCY_BOUND_TRACKING
  int getLatencyLimited();     /**< Return 1 if action is limited by latency, 0 otherwise */
#endif

  xbt_swag_t p_stateSet;

  double m_priority; /**< priority (1.0 by default) */
  bool m_failed;
  double m_start; /**< start time  */
  double m_finish; /**< finish time : this is modified during the run and fluctuates until the task is completed */
  double m_remains; /**< How much of that cost remains to be done in the currently running task */
  #ifdef HAVE_LATENCY_BOUND_TRACKING
  int m_latencyLimited;               /**< Set to 1 if is limited by latency, 0 otherwise */
  #endif
  double m_maxDuration; /*< max_duration (may fluctuate until the task is completed) */  
  char *p_category;               /**< tracing category for categorized resource utilization monitoring */  
  int    m_cost;
  void *p_data; /**< for your convenience */
protected:
  ModelPtr p_model;  
  int    m_refcount;
#ifdef HAVE_TRACING
#endif
  e_UM_t p_updateMechanism;

private:
  int resourceUsed(void *resource_id);
  /* Share the resources to the actions and return in how much time
     the next action may terminate */
  double shareResources(double now);
  /* Update the actions' state */
  void updateActionsState(double now, double delta);
  void updateResourceState(void *id, tmgr_trace_event_t event_type,
                                 double value, double time);

  lmm_system_t p_maxminSystem;
  xbt_swag_t p_modifiedSet;
  xbt_heap_t p_actionHeap;
  int m_selectiveUpdate;
};

//FIXME:REMOVE
void surf_action_lmm_update_index_heap(void *action, int i);

class ActionLmm: virtual public Action {
public:
  ActionLmm() : m_suspended(false) {
	p_actionListHookup.prev = 0;
	p_actionListHookup.next = 0;
  };
  ActionLmm(ModelPtr model, double cost, bool failed) : m_suspended(false) {
	p_actionListHookup.prev = 0;
	p_actionListHookup.next = 0;
  };

  virtual void updateRemainingLazy(double now);
  void heapInsert(xbt_heap_t heap, double key, enum heap_action_type hat);
  void heapRemove(xbt_heap_t heap);
  double getRemains();     /**< Get the remains of an action */
  void updateIndexHeap(int i);

  virtual int unref();
  void cancel();
  void suspend();
  void resume();
  bool isSuspended();
  void setMaxDuration(double duration);
  void setPriority(double priority);
  void gapRemove();

  lmm_variable_t p_variable;
  s_xbt_swag_hookup_t p_actionListHookup;
  int m_indexHeap;
  double m_lastUpdate;
  double m_lastValue;
  enum heap_action_type m_hat;
  int m_suspended;
};

#endif /* SURF_MODEL_H_ */
