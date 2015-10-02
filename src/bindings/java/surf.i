/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* File : example.i */
%module(directors="1") Surf

%include "arrays_java.i"
%include "std_string.i"
%include "surfdoc.i"

%pragma(java) jniclassimports=%{
import org.simgrid.NativeLib;
%}
%pragma(java) jniclasscode=%{
  static {
    if (System.getProperty("os.name").toLowerCase().startsWith("win"))
        NativeLib.nativeInit("winpthread-1");
    NativeLib.nativeInit("simgrid");
    NativeLib.nativeInit("surf-java");
    Runtime.getRuntime().addShutdownHook(
      new Thread() {
        public void run() {
          Thread.currentThread().setName( "Destroyer" );
          Surf.clean();
        }
      }
    );
  }
%}

%{
#include "src/surf/surf_interface.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/trace_mgr_private.h"
#include "src/bindings/java/surf_swig.hpp"
#include "src/xbt/dict_private.h"

typedef struct lmm_constraint *lmm_constraint_t;
typedef xbt_dynar_t DoubleDynar;
%}

%wrapper %{
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jobject JNICALL Java_org_simgrid_surf_SurfJNI_getAction(JNIEnv *env, jclass cls, jlong jarg1) {
  Action * action = (Action *)jarg1;
  jobject res;
  CpuAction *cpu_action = dynamic_cast<CpuAction*>(action);
  if (cpu_action) {
    SwigDirector_CpuAction *dir_cpu_action = dynamic_cast<SwigDirector_CpuAction*>(cpu_action);
    if (dir_cpu_action) {
      res = dir_cpu_action->swig_get_self(env);\
    } else {
      jclass clss = env->FindClass("org/simgrid/surf/CpuAction");\
      jmethodID constru = env->GetMethodID(clss, "<init>", "()V");\
      res = env->NewObject(clss, constru);\
      res = env->NewGlobalRef(res);\
    }
  } else {
    jclass clss = env->FindClass("org/simgrid/surf/Action");\
    jmethodID constru = env->GetMethodID(clss, "<init>", "()V");\
    res = env->NewObject(clss, constru);\
    res = env->NewGlobalRef(res);\
  }
  return res;
}

#define GETDIRECTOR(NAME) \
JNIEXPORT jobject JNICALL Java_org_simgrid_surf_SurfJNI_get## NAME ## Director(JNIEnv *env, jclass cls, jlong jarg1)\
{\
  NAME * arg1 = (NAME*)jarg1;\
  SwigDirector_ ##NAME *director = dynamic_cast<SwigDirector_ ##NAME *>(arg1);\
  jobject res;\
  if (director) {\
    res = director->swig_get_self(env);\
  } else {\
    jclass clss = env->FindClass("org/simgrid/surf/NAME");\
    jmethodID constru = env->GetMethodID(clss, "<init>", "()V");\
    res = env->NewObject(clss, constru);\
    res = env->NewGlobalRef(res);\
  }\
  return res;\
}

GETDIRECTOR(CpuModel)
GETDIRECTOR(Cpu)
GETDIRECTOR(CpuAction)

#ifdef __cplusplus
}
#endif

%}

%typemap(freearg) char* name {
}

/* Handle xbt_dynar_t of Link */
JAVA_ARRAYSOFCLASSES(Action);
%apply Action[] {ActionArrayPtr};
%typemap(jstype) ActionArrayPtr "Action[]"
%typemap(javain) ActionArrayPtr "Action.cArrayUnwrap($javainput)"
%typemap(javaout) ActionArrayPtr {
  long[] cArray = $jnicall;
  Action[] arrayWrapper = new Action[cArray.length];
  for (int i=0; i<cArray.length; i++)
    arrayWrapper[i] = (Action) Surf.getAction(cArray[i]);
  return arrayWrapper;
  //   return Action.cArrayWrap($jnicall, $owner);
}
%typemap(out) ActionArrayPtr {
  long l = 0;
  for(ActionList::iterator it($1->begin()), itend($1->end()); it != itend ; ++it) {
    l++;
  }
  $result = jenv->NewLongArray(l);
  jlong *elts = jenv->GetLongArrayElements($result, NULL);
  l = 0;
  for(ActionList::iterator it($1->begin()), itend($1->end()); it != itend ; ++it) {
    elts[l++] = (jlong)static_cast<Action*>(&*it);
  }
  jenv->ReleaseLongArrayElements($result, elts, 0);
}

class ActionList {
public:
  //void push_front(Action &action);
  //void push_back(Action &action);
%extend {
  ActionArrayPtr getArray(){
    return $self;
  }
}
};

/* Handle xbt_dynar_t of Link */
JAVA_ARRAYSOFCLASSES(Link);
%apply Link[] {LinkDynar};
%typemap(jstype) LinkDynar "Link[]"
%typemap(javain) LinkDynar "Link.cArrayUnwrap($javainput)"
%typemap(javaout) LinkDynar {
     return Link.cArrayWrap($jnicall, $owner);
}
%typemap(out) LinkDynar {
  long l = xbt_dynar_length($1);
  $result = jenv->NewLongArray(l);
  unsigned i;
  Link *link;
  jlong *elts = jenv->GetLongArrayElements($result, NULL);
  xbt_dynar_foreach($1, i, link) {
    elts[i] = (jlong)link;
  }
  jenv->ReleaseLongArrayElements($result, elts, 0);
  xbt_dynar_free(&$1);
}

%nodefault DoubleDynar;
%typemap(jni) DoubleDynar "jdoubleArray"
%rename(DoubleDynar) Double[];
%typemap(jtype) DoubleDynar "double[]"
%typemap(jstype) DoubleDynar "double[]"
%typemap(out) DoubleDynar {
  long l = xbt_dynar_length($1);
  $result = jenv->NewDoubleArray(l);
  double *lout = (double *)xbt_dynar_to_array($1);
  jenv->SetDoubleArrayRegion($result, 0, l, (jdouble*)lout);
  free(:);
}
%typemap(javadirectorin) DoubleDynar "$jniinput"
%typemap(directorin,descriptor="[D") DoubleDynar %{
  long l = xbt_dynar_length($1);
  $input = jenv->NewDoubleArray(l);
  double *lout = (double *)xbt_dynar_to_array($1);
  jenv->SetDoubleArrayRegion($input, 0, l, (jdouble*)lout);
  free(lout);
%}
%typemap(javain) DoubleDynar "$javainput"
%typemap(javaout) DoubleDynar {return  $jnicall}

/* Allow to subclass Plugin and send java object to C++ code */
%feature("director") Plugin;

%native(getAction) jobject getAction(jlong jarg1);
%native(getCpuModelDirector) jobject getCpuModelDirector(jlong jarg1);
%typemap(javaout) CpuModel * {
  long cPtr = $jnicall;
  return (CpuModel)Surf.getCpuModelDirector(cPtr);
}
%native(getCpuDirector) jobject getCpuDirector(jlong jarg1);
%typemap(javaout) Cpu * {
  long cPtr = $jnicall;
  return (Cpu)Surf.getCpuDirector(cPtr);
}
%native(getCpuActionDirector) jobject getCpuActionDirector(jlong jarg1);
%typemap(javaout) CpuAction * {
  long cPtr = $jnicall;
  return (CpuAction)Surf.getCpuDirector(cPtr);
}

%include "src/bindings/java/surf_swig.hpp"

%rename tmgr_trace TmgrTrace;
%nodefaultctor tmgr_trace;
struct tmgr_trace {
  //enum e_trace_type type;
  /*union {
    struct {
      xbt_dynar_t event_list;
    } s_list;
    struct {
      probabilist_event_generator_t event_generator[2];
      int is_state_trace;
      int next_event;
    } s_probabilist;
  };*/
  %extend {
  }
};

%rename tmgr_trace_event TmgrTraceEvent;
%nodefaultctor tmgr_trace_event;
struct tmgr_trace_event {
  //tmgr_trace_t trace;
  //unsigned int idx;
  //void *model;
  //int free_me;
  %extend {
    unsigned int getIdx() {return 0;}//$self->idx;}
  }
};

%nodefaultctor Model;
class Model {
public:
  Model();
  
  virtual double shareResources(double now);
  virtual double shareResourcesLazy(double now);
  virtual double shareResourcesFull(double now);

  virtual void updateActionsState(double now, double delta);
  virtual void updateActionsStateLazy(double now, double delta);
  virtual void updateActionsStateFull(double now, double delta);

  virtual ActionList *getRunningActionSet();

  virtual void addTraces()=0;
};

%feature("director") CpuModel;
class CpuModel : public Model {
public:
  CpuModel();
  virtual ~CpuModel();
  virtual Cpu *createCpu(const char *name, DoubleDynar power_peak,
                              int pstate, double power_scale,
                              tmgr_trace *power_trace, int core,
                              e_surf_resource_state_t state_initial,
                              tmgr_trace *state_trace,
                              s_xbt_dict *cpu_properties)=0;
};

class Resource {
public:
  Resource();
  const char *getName();
  virtual bool isUsed()=0;
  Model *getModel();

  virtual e_surf_resource_state_t getState();
  lmm_constraint *getConstraint();
  s_xbt_dict *getProperties();
  virtual void updateState(tmgr_trace_event *event_type, double value, double date)=0;
};

%feature("director") Cpu;
class Cpu : public Resource {
public:
  Cpu(Model *model, const char *name, s_xbt_dict *props,
    lmm_constraint *constraint, int core, double powerPeak, double powerScale);
  Cpu(Model *model, const char *name, s_xbt_dict *props,
    int core, double powerPeak, double powerScale);
  virtual ~Cpu();
  virtual double getCurrentPowerPeak();
  virtual CpuAction *execute(double size)=0;
  virtual CpuAction *sleep(double duration)=0;
  virtual int getCore();
  virtual double getSpeed(double load);
  virtual double getAvailableSpeed();
  virtual double getPowerPeakAt(int pstate_index)=0;
  virtual int getNbPstates()=0;
  virtual void setPstate(int pstate_index)=0;
  virtual int  getPstate()=0;
  void setState(e_surf_resource_state_t state);
};

class Link : public Resource {
public:
  Link();
  ~Link();
  double getBandwidth();
  void updateBandwidth(double value, double date=surf_get_clock());
  double getLatency();
  void updateLatency(double value, double date=surf_get_clock());
};

%nodefaultctor Action;
class Action {
public:
  Action(Model *model, double cost, bool failed);
  virtual ~Action();
  Model *getModel();
  lmm_variable *getVariable();
  e_surf_action_state_t getState();
  bool isSuspended();
  double getBound();
  void setBound(double bound);
  void updateRemains(double delta);
  virtual double getRemains();
  virtual void setPriority(double priority);
  virtual void setState(e_surf_action_state_t state);
};

%nodefaultctor CpuAction;
%feature("director") CpuAction;
class CpuAction : public Action {
public:
CpuAction(Model *model, double cost, bool failed);
%extend {
  Cpu *getCpu() {return getActionCpu($self);}
}
};

%nodefaultctor NetworkAction;
class NetworkAction : public Action {
public:
%extend {
  double getLatency() {return $self->m_latency;}
}
};


%nodefaultctor RoutingEdge;
class RoutingEdge {
public:
  virtual char *getName()=0;
};

%rename lmm_constraint LmmConstraint;
struct lmm_constraint {
%extend {
  double getUsage() {return lmm_constraint_get_usage($self);}
}
};

%rename lmm_variable LmmVariable;
struct lmm_variable {
%extend {
  double getValue() {return lmm_variable_getvalue($self);}
}
};

%rename s_xbt_dict XbtDict;
struct s_xbt_dict {
%extend {
  char *getValue(char *key) {return (char*)xbt_dict_get_or_null($self, key);}
}
};

%rename e_surf_action_state_t ActionState;
typedef enum {
  SURF_ACTION_READY = 0,        /**< Ready        */
  SURF_ACTION_RUNNING,          /**< Running      */
  SURF_ACTION_FAILED,           /**< Task Failure */
  SURF_ACTION_DONE,             /**< Completed    */
  SURF_ACTION_TO_FREE,          /**< Action to free in next cleanup */
  SURF_ACTION_NOT_IN_THE_SYSTEM
                                /**< Not in the system anymore. Why did you ask ? */
} e_surf_action_state_t;

%rename e_surf_resource_state_t ResourceState;
typedef enum {
  SURF_RESOURCE_ON = 1,                   /**< Up & ready        */
  SURF_RESOURCE_OFF = 0                   /**< Down & broken     */
} e_surf_resource_state_t;
