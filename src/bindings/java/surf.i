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
  NetworkLink **lout = (NetworkLink **)xbt_dynar_to_array($1);
  jenv->SetLongArrayRegion($result, 0, l, (const jlong*)lout);
  free(lout);
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
