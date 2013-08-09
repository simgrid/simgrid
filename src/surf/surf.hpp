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

extern tmgr_history_t history;

using namespace std;

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
 * Model *
 *********/
class Model {
public:
  Model(string name) {
    m_name = name;
    m_resOnCB = m_resOffCB= 0;
    m_actSuspendCB = m_actCancelCB = m_actResumeCB = 0;
  }
  virtual ~Model() {}
  ResourcePtr createResource(string name);
  ActionPtr createAction(double _cost, bool _failed);

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
  xbt_swag_t runningActionSet; /**< Actions in state SURF_ACTION_RUNNING */
  xbt_swag_t failedActionSet; /**< Actions in state SURF_ACTION_FAILED */
  xbt_swag_t doneActionSet; /**< Actions in state SURF_ACTION_DONE */

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
  Resource(ModelPtr model, string name, xbt_dict_t properties):
	  m_name(name),m_running(true),p_model(model),m_properties(properties) {};
  virtual ~Resource() {};

  bool isOn();
  void turnOn();
  void turnOff();
  void setName(string name);
  string getName();
  ModelPtr getModel() {return p_model;};
  void printModel() { std::cout << p_model->getName() << "<<plop"<<std::endl;};
 
protected:
  ModelPtr p_model;
  string m_name;
  xbt_dict_t m_properties;

private:
  bool m_running;  
};

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
	 m_cost(cost),p_model(model),m_failed(failed)
  {
    m_priority = m_start = m_finish = m_maxDuration = -1.0;
    m_start = 10;//surf_get_clock();
    m_suspended = false;
  };
  virtual ~Action() {};
  
  virtual e_surf_action_state_t getState()=0; /**< Return the state of an action */
  virtual void setState(e_surf_action_state_t state)=0; /**< Change an action state*/
  virtual double getStartTime()=0; /**< Return the start time of an action */
  virtual double getFinishTime()=0; /**< Return the finish time of an action */
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


protected:
  ModelPtr p_model;  
  int    m_cost;
  bool   m_failed, m_suspended;
  double m_priority;
  double m_maxDuration;
  double m_start, m_finish;

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
