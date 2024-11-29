// swig -c++ -java -Wall -I../../../include -package org.simgrid.s4u -outdir org/simgrid/s4u simgrid_java.i

// https://jornvernee.github.io/java/panama-ffi/panama/jni/native/2021/09/13/debugging-unsatisfiedlinkerrors.html

%module(directors="1") simgrid
%{
#include "simgrid/s4u.hpp"
#include <boost/shared_ptr.hpp>

using namespace simgrid::s4u;

/* SWIG code does not compile with all our flags */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wunused-variable"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(java);
%}
%include <typemaps.i>
%include <stl.i>

// Convert (int* argc, char** argv) into String[] (it's int* not int),
//   and also std::vector<std::string> from/to String[]
%include <defs/pargc_argv.i> 

// Random stuff
#define XBT_PUBLIC
#define XBT_ATTRIB_NORETURN
#define DOXYGEN
%ignore "get_impl";
%ignore "get_name";
%ignore "get_properties";
%rename(get_name) "get_cname";

%ignore "wait";
%rename(wait) "await";

%apply long {aid_t};
%apply signed long {ssize_t};
%apply signed long {sg_size_t};
%apply unsigned long {uint64_t};
#define XBT_DECLARE_ENUM_CLASS(EnumType, ...)                                                                          \
  enum class EnumType;                                                                                                 \
  static constexpr char const* to_c_str(EnumType value)                                                                \
  {                                                                                                                    \
    constexpr std::array<const char*, _XBT_COUNT_ARGS(__VA_ARGS__)> names{{_XBT_STRINGIFY_ARGS(__VA_ARGS__)}};         \
    return names[static_cast<int>(value)];                                                                             \
  }                                                                                                                    \
  enum class EnumType { __VA_ARGS__ } /* defined here to handle trailing semicolon */

%ignore "on_creation_cb";
%ignore "on_suspend_cb";
%ignore "on_sleep_cb";
%ignore "on_wake_up_cb";
%ignore "on_host_change_cb";
%ignore "on_termination_cb";
%ignore "on_destruction_cb";
%ignore "on_resume_cb";

/// Take care of our intrusive pointers

%include <boost_intrusive_ptr.i>
%ignore intrusive_ptr_add_ref;
%ignore intrusive_ptr_release;
%ignore get_refcount;

%define %sg_intrusive(Klass) // A macro for each of our intrusives
  #define Klass##Ptr boost::intrusive_ptr< simgrid::s4u::Klass > /* Our C++ code uses an alias that disturbs SWIG */
  %intrusive_ptr(simgrid::s4u::Klass) // Basic handling
  %apply simgrid::s4u::Klass {Klass} ; // Deal with aliasing within namespaces
%enddef

%sg_intrusive(Activity)
%sg_intrusive(Comm)
%sg_intrusive(Exec)
%sg_intrusive(Io)

%sg_intrusive(Actor)
%sg_intrusive(Disk)
%sg_intrusive(Host)
%sg_intrusive(VirtualMachine)
%sg_intrusive(Mailbox)

%sg_intrusive(Barrier)
%sg_intrusive(ConditionVariable)
%sg_intrusive(Mutex)
%sg_intrusive(Semaphore)

///////// The kind of magic we need to declare the body of actors in Java

#include <functional>
%include <defs/std_function.i>
%std_function(ActorCMain, void, int, char**);

%init %{
  JavaVM* simgrid_cached_jvm = NULL;
  extern bool do_install_signal_handlers;

  JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    simgrid_cached_jvm = jvm;
    setlocale(LC_NUMERIC,"C");
    do_install_signal_handlers = false;
    return JNI_VERSION_1_2;
  }

  #include "src/kernel/context/ContextJava.hpp"
  static struct SimGridJavaInit { SimGridJavaInit() { 
    simgrid::kernel::context::ContextFactory::initializer = []() {
      XBT_INFO("Using regular java threads.");
      return new simgrid::kernel::context::JavaContextFactory();
    };

  } } sgJavaInit;
%}

%feature("director") ActorMain;

%inline %{
struct ActorMain {
  virtual void run() = 0;
  virtual ~ActorMain() = default;
};
%}

%ignore Actor::set_stacksize;
%ignore simgrid::s4u::Actor::create;
%rename(create) simgrid::s4u::Actor::create_java;

%include <simgrid/s4u/Actor.hpp>
%extend simgrid::s4u::Actor {
  static ActorPtr create_java(const std::string& name, s4u::Host* host, ActorMain* code) {
    return Actor::create(name, host, [code](){code->run();}); 
  }
}

/////////////////// Load each class, along with the local rewritings

%include <simgrid/s4u/Activity.hpp>

%include <simgrid/s4u/Barrier.hpp>

%ignore set_copy_data_callback; // Needs kernel::activity::CommImpl*
%ignore simgrid::s4u::Comm::send; // Needs kernel::actor::ActorImpl*
%ignore simgrid::s4u::Comm::recv; // Needs kernel::actor::ActorImpl*
%include <simgrid/s4u/Comm.hpp>

%include <simgrid/s4u/ConditionVariable.hpp>

%ignore set_read_bandwidth_profile;
%ignore set_write_bandwidth_profile;
%ignore set_latency_profile;
%ignore set_state_profile;
%ignore on_io_state_change_cb;
%ignore set_sharing_policy;
%include <simgrid/s4u/Disk.hpp>

%ignore simgrid::s4u::Engine::register_function(const std::string& name, const kernel::actor::ActorCodeFactory& factory);
%ignore simgrid::s4u::Engine::register_default(const kernel::actor::ActorCodeFactory& factory);
%ignore get_all_netpoints;
%ignore netpoint_by_name;
%ignore split_duplex_link_by_name;
%ignore netpoint_by_name_or_null;
%ignore add_model;
%ignore get_all_models;
%ignore set_default_comm_data_copy_callback;
%include <simgrid/s4u/Engine.hpp>
%extend simgrid::s4u::Engine {
  static void critical(const char* msg) {
    XBT_CRITICAL("%s", msg);
  }
  static void error(const char* msg) {
    XBT_ERROR("%s", msg);
  }
  static void warn(const char* msg) {
    XBT_WARN("%s", msg);
  }
  static void info(const char* msg) {
    XBT_INFO("%s", msg);
  }
  static void verbose(const char* msg) {
    XBT_VERB("%s", msg);
  }
  static void debug(const char* msg) {
    XBT_DEBUG("%s", msg);
  }
  %proxycode %{
	/** Example launcher. You can use it or provide your own launcher, as you wish */
	public static void main(String[]args) {
  
		/* initialize the SimGrid simulation. Must be done before anything else */
		Engine e = Engine.get_instance(args);

		if (args.length < 2) {
			e.info("Usage: org.simgrid.s4u.Engine platform_file deployment_file");
			System.exit(1);
		}

		/* Load the platform and deploy the application */
		e.load_platform(args[0]);
		e.load_deployment(args[1]);
		/* Execute the simulation */
		e.run();
	}

	/* Class initializer, to initialize various JNI stuff */
	static {
		org.simgrid.s4u.NativeLib.nativeInit();
	}
  %}
}

%include <simgrid/s4u/Exec.hpp>
%include <simgrid/s4u/Io.hpp>

%ignore simgrid::s4u::Host::Host;
%include <simgrid/s4u/Host.hpp>

%ignore set_bandwidth_profile;
%ignore on_communication_state_change_cb;
%ignore set_sharing_policy;
%ignore NonLinearResourceCb;
%ignore SplitDuplexLink;
%include <simgrid/s4u/Link.hpp>

%ignore get_netpoint;
%ignore get_gateway;
%ignore set_gateway;
%ignore add_component;
%ignore add_route;
%ignore add_bypass_route;
%ignore get_network_model;
%ignore create_router;
%ignore create_split_duplex_link;
%ignore extract_xbt_graph;
%include <simgrid/s4u/NetZone.hpp>

// convert void* from/to Object. We store the objects' pointers as long in the C++ world
%typemap(jstype) void*payload "Object" // Give the Java type corresponding to void*
%typemap(jtype) void*payload "Object" // Give the corresponding JNI type in Java (the Java type representing Java Objects)
%typemap(jni) void*payload "jlong" // Give the corresponding JNI type in C++ (the C++ type representing Java Objects)

%typemap(javain) void*payload "$javainput" // From Java to JNI
%typemap(in) void*payload { // From JNI to C++
  $1 = (void*)$input;
}
%typemap(out) void*payload { // From C++ to JNI
  $result = (jlong)$1;
}
%typemap(javaout) void*payload { // From JNI to Java
    return $jnicall;
}

%typemap(in,numinputs=0) JNIEnv *jenv "$1 = jenv;" // Pass the JNI environment to extensions asking for it
%ignore simgrid::s4u::Mailbox::front;
%ignore simgrid::s4u::Mailbox::iprobe;
%ignore simgrid::s4u::Mailbox::get_unique;

%ignore simgrid::s4u::Mailbox::put; // Forget about the classical C++ Mailbox::put in the generated bindings
%rename(put) simgrid::s4u::Mailbox::put_java; // Use the extension provided below as a new Mailbox::put for the Java code
%ignore simgrid::s4u::Mailbox::get;
%rename(get) simgrid::s4u::Mailbox::get_java;
%include <simgrid/s4u/Mailbox.hpp>
%extend simgrid::s4u::Mailbox { // Here is the code of the extensions. These functions will be injected in the generated class
  void put_java(JNIEnv *jenv, jobject payload, uint64_t simulated_size_in_bytes) {
    self->put(jenv->NewGlobalRef(payload), simulated_size_in_bytes);
  }
  void put_java(JNIEnv *jenv, jobject payload, uint64_t simulated_size_in_bytes, double timeout) {
    self->put(jenv->NewGlobalRef(payload), simulated_size_in_bytes, timeout);
  }
  jobject get_java(JNIEnv *jenv) {
    jobject glob = self->get<_jobject>();
    auto loc = jenv->NewLocalRef(glob);
    jenv->DeleteGlobalRef(glob);

    return loc;
  }
}

%ignore simgrid::s4u::MessageQueue::front;
%include <simgrid/s4u/MessageQueue.hpp>

%include <simgrid/s4u/Mutex.hpp>
%include <simgrid/s4u/Semaphore.hpp>

%ignore simgrid::s4u::VmHostExt;
%ignore get_vm_impl;
%ignore simgrid::s4u::VirtualMachine::VirtualMachine;
%include <simgrid/s4u/VirtualMachine.hpp>

%init %{
   /* Restore our paranoid compilation flags*/
   #pragma GCC diagnostic pop 
%}