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
using namespace simgrid;
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

%feature("director") Actor;
%feature("director") simgrid::s4u::Host;

%ignore "on_creation_cb";
%ignore "on_suspend_cb";
%ignore "on_resume_cb";
%ignore "on_sleep_cb";
%ignore "on_wake_up_cb";
%ignore "on_host_change_cb";

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

%sg_intrusive(Actor)
%sg_intrusive(Disk)
%sg_intrusive(Host)
%sg_intrusive(VirtualMachine)
%sg_intrusive(Mailbox)

%sg_intrusive(Barrier)
%sg_intrusive(ConditionVariable)
%sg_intrusive(Mutex)
%sg_intrusive(Semaphore)


#include <functional>
%include <defs/std_function.i>
//%std_function(ActorCMain, void, int, char**);

///////////// Initialize the java bindings when the library is loaded within the JVM
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

%feature("director") BooleanCallback;
%inline %{
struct BooleanCallback {
  virtual void run(bool b) = 0;
  virtual ~BooleanCallback() = default;
};
%}

%feature("director") ActorCallback;
%inline %{
struct ActorCallback {
  virtual void run(simgrid::s4u::Actor* a) = 0;
  virtual ~ActorCallback() = default;
};
%}


///////// The kind of magic we need to declare the body of actors in Java
%feature("director") ActorMain;

%inline %{
struct ActorMain {
  virtual void run() = 0;
  virtual ~ActorMain() = default;
};
%}
%extend ActorMain {
  /** Block the current actor sleeping for that amount of seconds */
  void sleep_for(double duration) { simgrid::s4u::this_actor::sleep_for(duration); }
  /** Block the current actor sleeping until the specified timestamp */
  void sleep_until(double wakeup_time) { simgrid::s4u::this_actor::sleep_until(wakeup_time); }
  /** Block the current actor, computing the given amount of flops */
  void execute(double flop) { simgrid::s4u::this_actor::execute(flop); }
  /** Block the current actor, computing the given amount of flops at the given priority.
   *  An execution of priority 2 computes twice as fast as an execution at priority 1. */
  void execute(double flop, double priority) { simgrid::s4u::this_actor::execute(flop, priority); }
  /** Block the current actor until the built multi-thread execution completes. */
  void thread_execute(s4u::Host* host, double flop_amounts, int thread_count)
    { simgrid::s4u::this_actor::thread_execute(host, flop_amounts, thread_count); }

  /** Initialize a sequential execution that must then be started manually */
  //boost::intrusive_ptr<simgrid::s4u::Exec>
  simgrid::s4u::ExecPtr exec_init(double flops_amounts) { return simgrid::s4u::this_actor::exec_init(flops_amounts); }
  /** Initialize a parallel execution that must then be started manually */
  //ExecPtr exec_init(const std::vector<s4u::Host*>& hosts, const std::vector<double>& flops_amounts,
  //                  const std::vector<double>& bytes_amounts);

  /** Initialize and start a sequential execution */
  //boost::intrusive_ptr<simgrid::s4u::Exec>
  ExecPtr exec_async(double flops_amounts) { return simgrid::s4u::this_actor::exec_async(flops_amounts); }

  /** Returns the actor ID of the current actor. */
  long get_pid() { return simgrid::s4u::this_actor::get_pid(); }

  /** Returns the ancestor's actor ID of the current actor. */
  long get_ppid() { return simgrid::s4u::this_actor::get_ppid(); }

  /** Returns the name of the current actor. */
  std::string get_name() { return simgrid::s4u::this_actor::get_name(); }

  /** Returns the name of the host on which the current actor is running. */
  simgrid::s4u::Host* get_host() { return simgrid::s4u::this_actor::get_host(); }
  /** Migrate the current actor to a new host. */
  void set_host(s4u::Host* new_host)  { simgrid::s4u::this_actor::set_host(new_host); }
  /** Suspend the current actor, that is blocked until resume()ed by another actor. */
  void suspend() { return simgrid::s4u::this_actor::suspend(); }
  /** Yield the current actor. */
  void yield() { return simgrid::s4u::this_actor::yield(); }
  /** kill the current actor. */
  void exit() { return simgrid::s4u::this_actor::exit(); }

  static void on_termination_cb(ActorCallback* code) {
    XBT_CRITICAL("Install on termination");
    Actor::on_termination_cb([code](s4u::Actor const& a){XBT_CRITICAL("Term %p %s", &a, a.get_cname());code->run((Actor*)&a);});
  }
  static void on_destruction_cb(ActorCallback* code) {
    Actor::on_destruction_cb([code](s4u::Actor const& a){XBT_CRITICAL("Dtor %p %s", &a, a.get_cname());code->run(&const_cast<Actor&>(a));});
  }
}

%ignore simgrid::s4u::this_actor::is_maestro;
%ignore simgrid::s4u::this_actor::sleep_for;
%ignore simgrid::s4u::this_actor::sleep_until;
%ignore simgrid::s4u::this_actor::execute;
%ignore simgrid::s4u::this_actor::thread_execute;
%ignore simgrid::s4u::this_actor::exec_init;
%ignore simgrid::s4u::this_actor::exec_async;
%ignore simgrid::s4u::this_actor::get_pid;
%ignore simgrid::s4u::this_actor::get_ppid;
%ignore simgrid::s4u::this_actor::get_name;
%ignore simgrid::s4u::this_actor::get_cname;
%ignore simgrid::s4u::this_actor::get_host;
%ignore simgrid::s4u::this_actor::set_host;
%ignore simgrid::s4u::this_actor::suspend;
%ignore simgrid::s4u::this_actor::yield;
%ignore simgrid::s4u::this_actor::exit;


%ignore Actor::set_stacksize;
%ignore simgrid::s4u::Actor::create;
%rename(create) simgrid::s4u::Actor::create_java;

%include <simgrid/s4u/Actor.hpp>
%extend simgrid::s4u::Actor {
  static ActorPtr create_java(const std::string& name, s4u::Host* host, ActorMain* code) {
    return Actor::create(name, host, [code](){code->run();}); 
  }
}

// Mapping this_actor::on_exit
%ignore simgrid::s4u::this_actor::on_exit;
%ignore simgrid::s4u::Actor::on_termination_cb;
%extend ActorMain {
  void on_exit(BooleanCallback* code) {
    simgrid::s4u::this_actor::on_exit([code](bool b){code->run(b);}); 
  }
}
/////////////////// Load each class, along with the local rewritings

%ignore simgrid::s4u::Activity::start;

%sg_intrusive(Activity)

// "Class Activity_T<X> might be abstract, no constructors generated"
#pragma SWIG nowarn=403
%intrusive_ptr(simgrid::s4u::Activity_T< simgrid::s4u::Comm >);
%intrusive_ptr(simgrid::s4u::Activity_T< simgrid::s4u::Exec >);
%intrusive_ptr(simgrid::s4u::Activity_T< simgrid::s4u::Io >);

%include <simgrid/s4u/Activity.hpp>

%template(InternalActivityComm) simgrid::s4u::Activity_T< simgrid::s4u::Comm >;
%template(InternalActivityExec) simgrid::s4u::Activity_T< simgrid::s4u::Exec >;
%template(InternalctivityIo) simgrid::s4u::Activity_T< simgrid::s4u::Io >;

%sg_intrusive(Comm)
%sg_intrusive(Exec)
%sg_intrusive(Io)
//#pragma SWIG nowarn=+403 // Reactivate this warning

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
  static void die(const char* msg) {
    xbt_die("%s", msg);
  }
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