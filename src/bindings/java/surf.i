/* File : example.i */
%module(directors="1") Surf

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
};

class Action {
public:
  Model *getModel();
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


%rename lmm_constraint LmmConstraint;
struct lmm_constraint {
%extend {
  double getUsage() {return lmm_constraint_get_usage($self);}
}
};

%rename s_xbt_dict XbtDict;
struct s_xbt_dict {
%extend {
  char *getValue(char *key) {return (char*)xbt_dict_get_or_null($self, key);}
}
};

