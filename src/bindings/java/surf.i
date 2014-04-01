/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* File : example.i */
%module(directors="1") Surf

%include "arrays_java.i"

%pragma(java) jniclassimports=%{
import org.simgrid.NativeLib;
%}
%pragma(java) jniclasscode=%{
  static {
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
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "src/bindings/java/surf_swig.hpp"
#include "src/xbt/dict_private.h"
typedef struct lmm_constraint *lmm_constraint_t;
%}

/* Handle xbt_dynar_t of NetworkLink */
JAVA_ARRAYSOFCLASSES(NetworkLink);
%apply NetworkLink[] {NetworkLinkDynar};
%typemap(jstype) NetworkLinkDynar "NetworkLink[]"
%typemap(javain) NetworkLinkDynar "NetworkLink.cArrayUnwrap($javainput)"

%typemap(javaout) NetworkLinkDynar {
     return NetworkLink.cArrayWrap($jnicall, $owner);
}
%typemap(out) NetworkLinkDynar {
  long l = xbt_dynar_length($1);
  $result = jenv->NewLongArray(l);
  unsigned i;
  NetworkLink *link;
  jlong *elts = jenv->GetLongArrayElements($result, NULL);
  xbt_dynar_foreach($1, i, link) {
    elts[i] = (jlong)link;
  }
  jenv->ReleaseLongArrayElements($result, elts, 0);
  xbt_dynar_free(&$1);
}

/* Allow to subclass Plugin and send java object to C++ code */
%feature("director") Plugin;

%include "src/bindings/java/surf_swig.hpp"

%nodefaultctor Model;
class Model {
public:
  const char *getName();
};

class Resource {
public:
  Resource();
  const char *getName();
  virtual bool isUsed()=0;
  lmm_constraint *getConstraint();
  s_xbt_dict *getProperties();
};

class Cpu : public Resource {
public:
  Cpu();
  ~Cpu();
  double getCurrentPowerPeak();
};

class NetworkLink : public Resource {
public:
  NetworkLink();
  ~NetworkLink();
  double getBandwidth();
  void updateBandwidth(double value, double date=surf_get_clock());
  double getLatency();
  void updateLatency(double value, double date=surf_get_clock());
};

class Action {
public:
  Model *getModel();
  lmm_variable *getVariable();
  double getBound();
  void setBound(double bound);
};

class CpuAction : public Action {
public:
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
