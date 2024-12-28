/* simgrid-java.cpp - Native code of the Java bindings                      */

/* Copyright (c) 2024-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/forward.h"
#define SWIG_VERSION 0x040201

#include "xbt/base.h"
#include <jni.h>
#include <stdlib.h>
#include <string.h>

/* Support for throwing Java exceptions */
typedef enum {
  SWIG_JavaOutOfMemoryError = 1,
  SWIG_JavaIOException,
  SWIG_JavaRuntimeException,
  SWIG_JavaIndexOutOfBoundsException,
  SWIG_JavaArithmeticException,
  SWIG_JavaIllegalArgumentException,
  SWIG_JavaNullPointerException,
  SWIG_JavaDirectorPureVirtual,
  SWIG_JavaUnknownError,
  SWIG_JavaIllegalStateException,
} SWIG_JavaExceptionCodes;

typedef struct {
  SWIG_JavaExceptionCodes code;
  const char* java_exception;
} SWIG_JavaExceptions_t;

XBT_ATTRIB_UNUSED static void SWIG_JavaThrowException(JNIEnv* jenv, SWIG_JavaExceptionCodes code, const char* msg)
{
  jclass excep;
  static const SWIG_JavaExceptions_t java_exceptions[] = {
      {SWIG_JavaOutOfMemoryError, "java/lang/OutOfMemoryError"},
      {SWIG_JavaIOException, "java/io/IOException"},
      {SWIG_JavaRuntimeException, "java/lang/RuntimeException"},
      {SWIG_JavaIndexOutOfBoundsException, "java/lang/IndexOutOfBoundsException"},
      {SWIG_JavaArithmeticException, "java/lang/ArithmeticException"},
      {SWIG_JavaIllegalArgumentException, "java/lang/IllegalArgumentException"},
      {SWIG_JavaNullPointerException, "java/lang/NullPointerException"},
      {SWIG_JavaDirectorPureVirtual, "java/lang/RuntimeException"},
      {SWIG_JavaUnknownError, "java/lang/UnknownError"},
      {SWIG_JavaIllegalStateException, "java/lang/IllegalStateException"},
      {(SWIG_JavaExceptionCodes)0, "java/lang/UnknownError"}};
  const SWIG_JavaExceptions_t* except_ptr = java_exceptions;

  while (except_ptr->code != code && except_ptr->code)
    except_ptr++;

  jenv->ExceptionClear();
  excep = jenv->FindClass(except_ptr->java_exception);
  if (excep)
    jenv->ThrowNew(excep, msg);
}

/* SWIG Errors applicable to all language modules, values are reserved from -1 to -99 */
#define SWIG_UnknownError -1
#define SWIG_IOError -2
#define SWIG_RuntimeError -3
#define SWIG_IndexError -4
#define SWIG_TypeError -5
#define SWIG_DivisionByZero -6
#define SWIG_OverflowError -7
#define SWIG_SyntaxError -8
#define SWIG_ValueError -9
#define SWIG_SystemError -10
#define SWIG_AttributeError -11
#define SWIG_MemoryError -12
#define SWIG_NullReferenceError -13
/* -----------------------------------------------------------------------------
 * director.swg
 *
 * This file contains support for director classes so that Java proxy
 * methods can be called from C++.
 * ----------------------------------------------------------------------------- */

#if defined(DEBUG_DIRECTOR_OWNED) || defined(DEBUG_DIRECTOR_EXCEPTION) || defined(DEBUG_DIRECTOR_THREAD_NAME)
#include <iostream>
#endif

#include <exception>

#if defined(SWIG_JAVA_USE_THREAD_NAME)

#if !defined(SWIG_JAVA_GET_THREAD_NAME)
namespace Swig {
static int GetThreadName(char* name, size_t len);
}

#if defined(__linux__)

#include <sys/prctl.h>
static int Swig::GetThreadName(char* name, size_t len)
{
  (void)len;
#if defined(PR_GET_NAME)
  return prctl(PR_GET_NAME, (unsigned long)name, 0, 0, 0);
#else
  (void)name;
  return 1;
#endif
}

#elif defined(__unix__) || defined(__APPLE__)

#include <pthread.h>
static int Swig::GetThreadName(char* name, size_t len)
{
  return pthread_getname_np(pthread_self(), name, len);
}

#else

static int Swig::GetThreadName(char* name, size_t len)
{
  (void)len;
  (void)name;
  return 1;
}
#endif

#endif

#endif

#if defined(SWIG_JAVA_DETACH_ON_THREAD_END)
#include <pthread.h>
#endif

namespace Swig {

/* Java object wrapper */
class JObjectWrapper {
public:
  JObjectWrapper() : jthis_(nullptr), weak_global_(true) {}

  ~JObjectWrapper()
  {
    jthis_       = nullptr;
    weak_global_ = true;
  }

  bool set(JNIEnv* jenv, jobject jobj, bool mem_own, bool weak_global)
  {
    if (!jthis_) {
      weak_global_ = weak_global || !mem_own; // hold as weak global if explicitly requested or not owned
      if (jobj)
        jthis_ = weak_global_ ? jenv->NewWeakGlobalRef(jobj) : jenv->NewGlobalRef(jobj);
#if defined(DEBUG_DIRECTOR_OWNED)
      std::cout << "JObjectWrapper::set(" << jobj << ", " << (weak_global ? "weak_global" : "global_ref") << ") -> "
                << jthis_ << std::endl;
#endif
      return true;
    } else {
#if defined(DEBUG_DIRECTOR_OWNED)
      std::cout << "JObjectWrapper::set(" << jobj << ", " << (weak_global ? "weak_global" : "global_ref")
                << ") -> already set" << std::endl;
#endif
      return false;
    }
  }

  jobject get(JNIEnv* jenv) const
  {
#if defined(DEBUG_DIRECTOR_OWNED)
    std::cout << "JObjectWrapper::get(";
    if (jthis_)
      std::cout << jthis_;
    else
      std::cout << "null";
    std::cout << ") -> return new local ref" << std::endl;
#endif
    return (jthis_ ? jenv->NewLocalRef(jthis_) : jthis_);
  }

  void release(JNIEnv* jenv)
  {
#if defined(DEBUG_DIRECTOR_OWNED)
    std::cout << "JObjectWrapper::release(" << jthis_ << "): " << (weak_global_ ? "weak global ref" : "global ref")
              << std::endl;
#endif
    if (jthis_) {
      if (weak_global_) {
        if (jenv->IsSameObject(jthis_, nullptr) == JNI_FALSE)
          jenv->DeleteWeakGlobalRef((jweak)jthis_);
      } else
        jenv->DeleteGlobalRef(jthis_);
    }

    jthis_       = nullptr;
    weak_global_ = true;
  }

  /* Only call peek if you know what you are doing wrt to weak/global references */
  jobject peek() { return jthis_; }

  /* Java proxy releases ownership of C++ object, C++ object is now
     responsible for destruction (creates NewGlobalRef to pin Java proxy) */
  void java_change_ownership(JNIEnv* jenv, jobject jself, bool take_or_release)
  {
    if (take_or_release) { /* Java takes ownership of C++ object's lifetime. */
      if (!weak_global_) {
        jenv->DeleteGlobalRef(jthis_);
        jthis_       = jenv->NewWeakGlobalRef(jself);
        weak_global_ = true;
      }
    } else {
      /* Java releases ownership of C++ object's lifetime */
      if (weak_global_) {
        jenv->DeleteWeakGlobalRef((jweak)jthis_);
        jthis_       = jenv->NewGlobalRef(jself);
        weak_global_ = false;
      }
    }
  }

#if defined(SWIG_JAVA_DETACH_ON_THREAD_END)
  static void detach(void* jvm) { static_cast<JavaVM*>(jvm)->DetachCurrentThread(); }

  static void make_detach_key() { pthread_key_create(&detach_key_, detach); }

  /* thread-local key to register a destructor */
  static pthread_key_t detach_key_;
#endif

private:
  /* pointer to Java object */
  jobject jthis_;
  /* Local or global reference flag */
  bool weak_global_;
};

#if defined(SWIG_JAVA_DETACH_ON_THREAD_END)
pthread_key_t JObjectWrapper::detach_key_;
#endif

/* Local JNI reference deleter */
class LocalRefGuard {
  JNIEnv* jenv_;
  jobject jobj_;

  // non-copyable
  LocalRefGuard(const LocalRefGuard&);
  LocalRefGuard& operator=(const LocalRefGuard&);

public:
  LocalRefGuard(JNIEnv* jenv, jobject jobj) : jenv_(jenv), jobj_(jobj) {}
  ~LocalRefGuard()
  {
    if (jobj_)
      jenv_->DeleteLocalRef(jobj_);
  }
};

/* director base class */
class Director {
  /* pointer to Java virtual machine */
  JavaVM* swig_jvm_;

protected:
#if defined(_MSC_VER) && (_MSC_VER < 1300)
  class JNIEnvWrapper;
  friend class JNIEnvWrapper;
#endif
  /* Utility class for managing the JNI environment */
  class JNIEnvWrapper {
    const Director* director_;
    JNIEnv* jenv_;
    int env_status;

  public:
    JNIEnvWrapper(const Director* director) : director_(director), jenv_(nullptr), env_status(0)
    {
#if defined(__ANDROID__)
      JNIEnv** jenv = &jenv_;
#else
      void** jenv = (void**)&jenv_;
#endif
      env_status = director_->swig_jvm_->GetEnv((void**)&jenv_, JNI_VERSION_1_2);
      JavaVMAttachArgs args;
      args.version = JNI_VERSION_1_2;
      args.group   = nullptr;
      args.name    = nullptr;
#if defined(SWIG_JAVA_USE_THREAD_NAME)
      char
          thread_name[64]; // MAX_TASK_COMM_LEN=16 is hard-coded in the Linux kernel and MacOS has MAXTHREADNAMESIZE=64.
      if (Swig::GetThreadName(thread_name, sizeof(thread_name)) == 0) {
        args.name = thread_name;
#if defined(DEBUG_DIRECTOR_THREAD_NAME)
        std::cout << "JNIEnvWrapper: thread name: " << thread_name << std::endl;
      } else {
        std::cout << "JNIEnvWrapper: Couldn't set Java thread name" << std::endl;
#endif
      }
#endif
#if defined(SWIG_JAVA_ATTACH_CURRENT_THREAD_AS_DAEMON)
      // Attach a daemon thread to the JVM. Useful when the JVM should not wait for
      // the thread to exit upon shutdown. Only for jdk-1.4 and later.
      director_->swig_jvm_->AttachCurrentThreadAsDaemon(jenv, &args);
#else
      director_->swig_jvm_->AttachCurrentThread(jenv, &args);
#endif

#if defined(SWIG_JAVA_DETACH_ON_THREAD_END)
      // At least on Android 6, detaching after every call causes a memory leak.
      // Instead, register a thread desructor and detach only when the thread ends.
      // See https://developer.android.com/training/articles/perf-jni#threads
      static pthread_once_t once = PTHREAD_ONCE_INIT;

      pthread_once(&once, JObjectWrapper::make_detach_key);
      pthread_setspecific(JObjectWrapper::detach_key_, director->swig_jvm_);
#endif
    }
    ~JNIEnvWrapper()
    {
#if !defined(SWIG_JAVA_DETACH_ON_THREAD_END) && !defined(SWIG_JAVA_NO_DETACH_CURRENT_THREAD)
      // Some JVMs, eg jdk-1.4.2 and lower on Solaris have a bug and crash with the DetachCurrentThread call.
      // However, without this call, the JVM hangs on exit when the thread was not created by the JVM and creates a
      // memory leak.
      if (env_status == JNI_EDETACHED)
        director_->swig_jvm_->DetachCurrentThread();
#endif
    }
    JNIEnv* getJNIEnv() const { return jenv_; }
  };

  struct SwigDirectorMethod {
    const char* name;
    const char* desc;
    jmethodID methid;
    SwigDirectorMethod(JNIEnv* jenv, jclass baseclass, const char* name, const char* desc) : name(name), desc(desc)
    {
      methid = jenv->GetMethodID(baseclass, name, desc);
    }
  };

  /* Java object wrapper */
  JObjectWrapper swig_self_;

  /* Disconnect director from Java object */
  void swig_disconnect_director_self(const char* disconn_method)
  {
    JNIEnvWrapper jnienv(this);
    JNIEnv* jenv = jnienv.getJNIEnv();
    jobject jobj = swig_self_.get(jenv);
    LocalRefGuard ref_deleter(jenv, jobj);
#if defined(DEBUG_DIRECTOR_OWNED)
    std::cout << "Swig::Director::disconnect_director_self(" << jobj << ")" << std::endl;
#endif
    if (jobj && jenv->IsSameObject(jobj, nullptr) == JNI_FALSE) {
      jmethodID disconn_meth = jenv->GetMethodID(jenv->GetObjectClass(jobj), disconn_method, "()V");
      if (disconn_meth) {
#if defined(DEBUG_DIRECTOR_OWNED)
        std::cout << "Swig::Director::disconnect_director_self upcall to " << disconn_method << std::endl;
#endif
        jenv->CallVoidMethod(jobj, disconn_meth);
      }
    }
  }

  jclass swig_new_global_ref(JNIEnv* jenv, const char* classname)
  {
    jclass clz = jenv->FindClass(classname);
    return clz ? (jclass)jenv->NewGlobalRef(clz) : nullptr;
  }

public:
  Director(JNIEnv* jenv) : swig_jvm_((JavaVM*)nullptr), swig_self_()
  {
    /* Acquire the Java VM pointer */
    jenv->GetJavaVM(&swig_jvm_);
  }

  virtual ~Director()
  {
    JNIEnvWrapper jnienv(this);
    JNIEnv* jenv = jnienv.getJNIEnv();
    swig_self_.release(jenv);
  }

  bool swig_set_self(JNIEnv* jenv, jobject jself, bool mem_own, bool weak_global)
  {
    return swig_self_.set(jenv, jself, mem_own, weak_global);
  }

  jobject swig_get_self(JNIEnv* jenv) const { return swig_self_.get(jenv); }

  // Change C++ object's ownership, relative to Java
  void swig_java_change_ownership(JNIEnv* jenv, jobject jself, bool take_or_release)
  {
    swig_self_.java_change_ownership(jenv, jself, take_or_release);
  }
};

// Zero initialized bool array
template <size_t N> class BoolArray {
  bool array_[N];

public:
  BoolArray() { memset(array_, 0, sizeof(array_)); }
  bool& operator[](size_t n) { return array_[n]; }
  bool operator[](size_t n) const { return array_[n]; }
};

// Utility classes and functions for exception handling.

// Simple holder for a Java string during exception handling, providing access to a c-style string
class JavaString {
public:
  JavaString(JNIEnv* jenv, jstring jstr) : jenv_(jenv), jstr_(jstr), cstr_(nullptr)
  {
    if (jenv_ && jstr_)
      cstr_ = (const char*)jenv_->GetStringUTFChars(jstr_, nullptr);
  }

  ~JavaString()
  {
    if (jenv_ && jstr_ && cstr_)
      jenv_->ReleaseStringUTFChars(jstr_, cstr_);
  }

  const char* c_str(const char* null_string = "null JavaString") const { return cstr_ ? cstr_ : null_string; }

private:
  // non-copyable
  JavaString(const JavaString&);
  JavaString& operator=(const JavaString&);

  JNIEnv* jenv_;
  jstring jstr_;
  const char* cstr_;
};

// Helper class to extract the exception message from a Java throwable
class JavaExceptionMessage {
public:
  JavaExceptionMessage(JNIEnv* jenv, jthrowable throwable)
      : message_(jenv, exceptionMessageFromThrowable(jenv, throwable))
  {
  }

  // Return a C string of the exception message in the jthrowable passed in the constructor
  // If no message is available, null_string is return instead
  const char* message(const char* null_string = "Could not get exception message in JavaExceptionMessage") const
  {
    return message_.c_str(null_string);
  }

private:
  // non-copyable
  JavaExceptionMessage(const JavaExceptionMessage&);
  JavaExceptionMessage& operator=(const JavaExceptionMessage&);

  // Get exception message by calling Java method Throwable.getMessage()
  static jstring exceptionMessageFromThrowable(JNIEnv* jenv, jthrowable throwable)
  {
    jstring jmsg = nullptr;
    if (jenv && throwable) {
      jenv->ExceptionClear(); // Cannot invoke methods with any pending exceptions
      jclass throwclz = jenv->GetObjectClass(throwable);
      if (throwclz) {
        // All Throwable classes have a getMessage() method, so call it to extract the exception message
        jmethodID getMessageMethodID = jenv->GetMethodID(throwclz, "getMessage", "()Ljava/lang/String;");
        if (getMessageMethodID)
          jmsg = (jstring)jenv->CallObjectMethod(throwable, getMessageMethodID);
      }
      if (jmsg == nullptr && jenv->ExceptionCheck())
        jenv->ExceptionClear();
    }
    return jmsg;
  }

  JavaString message_;
};

// C++ Exception class for handling Java exceptions thrown during a director method Java upcall
class DirectorException : public std::exception {
public:
  // Construct exception from a Java throwable
  DirectorException(JNIEnv* jenv, jthrowable throwable)
      : jenv_(jenv), throwable_(throwable), classname_(nullptr), msg_(nullptr)
  {

    // Call Java method Object.getClass().getName() to obtain the throwable's class name (delimited by '/')
    if (jenv && throwable) {
      jenv->ExceptionClear(); // Cannot invoke methods with any pending exceptions
      jclass throwclz = jenv->GetObjectClass(throwable);
      if (throwclz) {
        jclass clzclz = jenv->GetObjectClass(throwclz);
        if (clzclz) {
          jmethodID getNameMethodID = jenv->GetMethodID(clzclz, "getName", "()Ljava/lang/String;");
          if (getNameMethodID) {
            jstring jstr_classname = (jstring)(jenv->CallObjectMethod(throwclz, getNameMethodID));
            // Copy strings, since there is no guarantee that jenv will be active when handled
            if (jstr_classname) {
              JavaString jsclassname(jenv, jstr_classname);
              const char* classname = jsclassname.c_str(nullptr);
              if (classname)
                classname_ = copypath(classname);
            }
          }
        }
      }
    }

    JavaExceptionMessage exceptionmsg(jenv, throwable);
    msg_ = copystr(exceptionmsg.message(nullptr));
  }

  // More general constructor for handling as a java.lang.RuntimeException
  DirectorException(const char* msg)
      : jenv_(nullptr), throwable_(nullptr), classname_(nullptr), msg_(msg ? copystr(msg) : nullptr)
  {
  }

  ~DirectorException() throw()
  {
    delete[] classname_;
    delete[] msg_;
  }

  const char* what() const throw() { return msg_ ? msg_ : "Unspecified DirectorException message"; }

  // Reconstruct and raise/throw the Java Exception that caused the DirectorException
  // Note that any error in the JNI exception handling results in a Java RuntimeException
  void throwException(JNIEnv* jenv) const
  {
    if (jenv) {
      if (jenv == jenv_ && throwable_) {
        // Throw original exception if not already pending
        jthrowable throwable = jenv->ExceptionOccurred();
        if (throwable && jenv->IsSameObject(throwable, throwable_) == JNI_FALSE) {
          jenv->ExceptionClear();
          throwable = nullptr;
        }
        if (!throwable)
          jenv->Throw(throwable_);
      } else {
        // Try and reconstruct original exception, but original stacktrace is not reconstructed
        jenv->ExceptionClear();

        jmethodID ctorMethodID = nullptr;
        jclass throwableclass  = nullptr;
        if (classname_) {
          throwableclass = jenv->FindClass(classname_);
          if (throwableclass)
            ctorMethodID = jenv->GetMethodID(throwableclass, "<init>", "(Ljava/lang/String;)V");
        }

        if (ctorMethodID) {
          jenv->ThrowNew(throwableclass, what());
        } else {
          SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, what());
        }
      }
    }
  }

  // Deprecated - use throwException
  void raiseJavaException(JNIEnv* jenv) const { throwException(jenv); }

  // Create and throw the DirectorException
  static void raise(JNIEnv* jenv, jthrowable throwable) { throw DirectorException(jenv, throwable); }

private:
  static char* copypath(const char* srcmsg)
  {
    char* target = copystr(srcmsg);
    for (char* c = target; *c; ++c) {
      if ('.' == *c)
        *c = '/';
    }
    return target;
  }

  static char* copystr(const char* srcmsg)
  {
    char* target = nullptr;
    if (srcmsg) {
      size_t msglen = strlen(srcmsg) + 1;
      target        = new char[msglen];
      strncpy(target, srcmsg, msglen);
    }
    return target;
  }

  JNIEnv* jenv_;
  jthrowable throwable_;
  const char* classname_;
  const char* msg_;
};

// Helper method to determine if a Java throwable matches a particular Java class type
// Note side effect of clearing any pending exceptions
static bool ExceptionMatches(JNIEnv* jenv, jthrowable throwable, const char* classname)
{
  bool matches = false;

  if (throwable && jenv && classname) {
    // Exceptions need to be cleared for correct behavior.
    // The caller of ExceptionMatches should restore pending exceptions if desired -
    // the caller already has the throwable.
    jenv->ExceptionClear();

    jclass clz = jenv->FindClass(classname);
    if (clz) {
      jclass classclz              = jenv->GetObjectClass(clz);
      jmethodID isInstanceMethodID = jenv->GetMethodID(classclz, "isInstance", "(Ljava/lang/Object;)Z");
      if (isInstanceMethodID) {
        matches = jenv->CallBooleanMethod(clz, isInstanceMethodID, throwable) != 0;
      }
    }

#if defined(DEBUG_DIRECTOR_EXCEPTION)
    if (jenv->ExceptionCheck()) {
      // Typically occurs when an invalid classname argument is passed resulting in a ClassNotFoundException
      JavaExceptionMessage exc(jenv, jenv->ExceptionOccurred());
      std::cout << "Error: ExceptionMatches: class '" << classname << "' : " << exc.message() << std::endl;
    }
#endif
  }
  return matches;
}
} // namespace Swig

namespace Swig {
namespace {
jclass jclass_simgridJNI = NULL;
jmethodID director_method_ids[3];
} // namespace
} // namespace Swig

#ifdef __cplusplus
#include <utility>
/* SwigValueWrapper is described in swig.swg */
template <typename T> class SwigValueWrapper {
  struct SwigSmartPointer {
    T* ptr;
    SwigSmartPointer(T* p) : ptr(p) {}
    ~SwigSmartPointer() { delete ptr; }
    SwigSmartPointer& operator=(SwigSmartPointer& rhs)
    {
      T* oldptr = ptr;
      ptr       = 0;
      delete oldptr;
      ptr     = rhs.ptr;
      rhs.ptr = 0;
      return *this;
    }
    void reset(T* p)
    {
      T* oldptr = ptr;
      ptr       = 0;
      delete oldptr;
      ptr = p;
    }
  } pointer;
  SwigValueWrapper& operator=(const SwigValueWrapper<T>& rhs);
  SwigValueWrapper(const SwigValueWrapper<T>& rhs);

public:
  SwigValueWrapper() : pointer(0) {}
  SwigValueWrapper& operator=(const T& t)
  {
    SwigSmartPointer tmp(new T(t));
    pointer = tmp;
    return *this;
  }
#if __cplusplus >= 201103L
  SwigValueWrapper& operator=(T&& t)
  {
    SwigSmartPointer tmp(new T(std::move(t)));
    pointer = tmp;
    return *this;
  }
  operator T&&() const { return std::move(*pointer.ptr); }
#else
  operator T&() const { return *pointer.ptr; }
#endif
  T* operator&() const { return pointer.ptr; }
  static void reset(SwigValueWrapper& t, T* p) { t.pointer.reset(p); }
};

/*
 * SwigValueInit() is a generic initialisation solution as the following approach:
 *
 *       T c_result = T();
 *
 * doesn't compile for all types for example:
 *
 *       unsigned int c_result = unsigned int();
 */
template <typename T> T SwigValueInit()
{
  return T();
}

#endif

#include "simgrid/s4u.hpp"
#include <boost/shared_ptr.hpp>

using namespace simgrid::s4u;

/* Don't whine about the missing declarations in this file, they are useless anyway in JNI */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-declarations"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(java);
using namespace simgrid;

#include <stdexcept>
#include <typeinfo>

#include <string>

#include <stdexcept>
#include <vector>

#include <map>
#include <stdexcept>

static void SWIG_JavaException(JNIEnv* jenv, int code, const char* msg)
{
  SWIG_JavaExceptionCodes exception_code = SWIG_JavaUnknownError;
  switch (code) {
    case SWIG_MemoryError:
      exception_code = SWIG_JavaOutOfMemoryError;
      break;
    case SWIG_IOError:
      exception_code = SWIG_JavaIOException;
      break;
    case SWIG_SystemError:
    case SWIG_RuntimeError:
      exception_code = SWIG_JavaRuntimeException;
      break;
    case SWIG_OverflowError:
    case SWIG_IndexError:
      exception_code = SWIG_JavaIndexOutOfBoundsException;
      break;
    case SWIG_DivisionByZero:
      exception_code = SWIG_JavaArithmeticException;
      break;
    case SWIG_SyntaxError:
    case SWIG_ValueError:
    case SWIG_TypeError:
      exception_code = SWIG_JavaIllegalArgumentException;
      break;
    case SWIG_UnknownError:
    default:
      exception_code = SWIG_JavaUnknownError;
      break;
  }
  SWIG_JavaThrowException(jenv, exception_code, msg);
}

#include <stdexcept>
#include <typeinfo>

#include <utility>

#include <functional>
#include <iostream>

struct BooleanCallback {
  virtual void run(bool b)   = 0;
  virtual ~BooleanCallback() = default;
};

struct ActorCallback {
  virtual void run(simgrid::s4u::Actor* a) = 0;
  virtual ~ActorCallback()                 = default;
};

template <class T> struct SWIG_intrusive_deleter {
  void operator()(T* p)
  {
    if (p)
      intrusive_ptr_release(p);
  }
};

struct SWIG_null_deleter {
  void operator()(void const*) const {}
};
#define SWIG_NO_NULL_DELETER_0 , SWIG_null_deleter()
#define SWIG_NO_NULL_DELETER_1

struct ActorMain {
  virtual void run()   = 0;
  virtual ~ActorMain() = default;
};

static void ActorMain_sleep_for(ActorMain* self, double duration)
{
  simgrid::s4u::this_actor::sleep_for(duration);
}
static void ActorMain_sleep_until(ActorMain* self, double wakeup_time)
{
  simgrid::s4u::this_actor::sleep_until(wakeup_time);
}
static void ActorMain_thread_execute(ActorMain* self, s4u::Host* host, double flop_amounts, int thread_count)
{
  simgrid::s4u::this_actor::thread_execute(host, flop_amounts, thread_count);
}
static long ActorMain_get_pid(ActorMain* self)
{
  return simgrid::s4u::this_actor::get_pid();
}
static long ActorMain_get_ppid(ActorMain* self)
{
  return simgrid::s4u::this_actor::get_ppid();
}
static void ActorMain_set_host(ActorMain* self, s4u::Host* new_host)
{
  simgrid::s4u::this_actor::set_host(new_host);
}
static void ActorMain_suspend(ActorMain* self)
{
  return simgrid::s4u::this_actor::suspend();
}
static void ActorMain_yield(ActorMain* self)
{
  return simgrid::s4u::this_actor::yield();
}
static void ActorMain_exit(ActorMain* self)
{
  return simgrid::s4u::this_actor::exit();
}
static void ActorMain_on_termination_cb(ActorCallback* code)
{
  XBT_CRITICAL("Install on termination");
  Actor::on_termination_cb([code](s4u::Actor const& a) {
    XBT_CRITICAL("Term %p %s", &a, a.get_cname());
    code->run((Actor*)&a);
  });
}
static void ActorMain_on_destruction_cb(ActorCallback* code)
{
  Actor::on_destruction_cb([code](s4u::Actor const& a) {
    XBT_CRITICAL("Dtor %p %s", &a, a.get_cname());
    code->run(&const_cast<Actor&>(a));
  });
}
static boost::intrusive_ptr<simgrid::s4u::Actor>
simgrid_s4u_Actor_create_java(std::string const& name, simgrid::s4u::Host* host, ActorMain* code)
{
  return Actor::create(name, host, [code]() { code->run(); });
}
static void simgrid_s4u_Engine_die(char const* msg)
{
  xbt_die("%s", msg);
}
static void simgrid_s4u_Engine_critical(char const* msg)
{
  XBT_CRITICAL("%s", msg);
}
static void simgrid_s4u_Engine_error(char const* msg)
{
  XBT_ERROR("%s", msg);
}
static void simgrid_s4u_Engine_warn(char const* msg)
{
  XBT_WARN("%s", msg);
}
static void simgrid_s4u_Engine_info(char const* msg)
{
  XBT_INFO("%s", msg);
}
static void simgrid_s4u_Engine_verbose(char const* msg)
{
  XBT_VERB("%s", msg);
}
static void simgrid_s4u_Engine_debug(char const* msg)
{
  XBT_DEBUG("%s", msg);
}

/* ---------------------------------------------------
 * C++ director class methods
 * --------------------------------------------------- */

#include "simgrid-java.hpp"

SwigDirector_BooleanCallback::SwigDirector_BooleanCallback(JNIEnv* jenv) : BooleanCallback(), Swig::Director(jenv) {}

void SwigDirector_BooleanCallback::run(bool b)
{
  JNIEnvWrapper swigjnienv(this);
  JNIEnv* jenv     = swigjnienv.getJNIEnv();
  jobject swigjobj = (jobject)NULL;
  jboolean jb;

  if (!swig_override[0]) {
    SWIG_JavaThrowException(JNIEnvWrapper(this).getJNIEnv(), SWIG_JavaDirectorPureVirtual,
                            "Attempted to invoke pure virtual method BooleanCallback::run.");
    return;
  }
  swigjobj = swig_get_self(jenv);
  if (swigjobj && jenv->IsSameObject(swigjobj, NULL) == JNI_FALSE) {
    jb = (jboolean)b;
    jenv->CallStaticVoidMethod(Swig::jclass_simgridJNI, Swig::director_method_ids[0], swigjobj, jb);
    jthrowable swigerror = jenv->ExceptionOccurred();
    if (swigerror) {
      Swig::DirectorException::raise(jenv, swigerror);
    }

  } else {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null upcall object in BooleanCallback::run ");
  }
  if (swigjobj)
    jenv->DeleteLocalRef(swigjobj);
}

SwigDirector_BooleanCallback::~SwigDirector_BooleanCallback()
{
  swig_disconnect_director_self("swigDirectorDisconnect");
}

void SwigDirector_BooleanCallback::swig_connect_director(JNIEnv* jenv, jobject jself, jclass jcls, bool swig_mem_own,
                                                         bool weak_global)
{
  static jclass baseclass = swig_new_global_ref(jenv, "org/simgrid/s4u/BooleanCallback");
  if (!baseclass)
    return;
  static SwigDirectorMethod methods[] = {SwigDirectorMethod(jenv, baseclass, "run", "(Z)V")};

  if (swig_set_self(jenv, jself, swig_mem_own, weak_global)) {
    bool derived = (jenv->IsSameObject(baseclass, jcls) ? false : true);
    for (int i = 0; i < 1; ++i) {
      swig_override[i] = false;
      if (derived) {
        jmethodID methid = jenv->GetMethodID(jcls, methods[i].name, methods[i].desc);
        swig_override[i] = methods[i].methid && (methid != methods[i].methid);
        jenv->ExceptionClear();
      }
    }
  }
}

SwigDirector_ActorCallback::SwigDirector_ActorCallback(JNIEnv* jenv) : ActorCallback(), Swig::Director(jenv) {}

void SwigDirector_ActorCallback::run(simgrid::s4u::Actor* a)
{
  JNIEnvWrapper swigjnienv(this);
  JNIEnv* jenv     = swigjnienv.getJNIEnv();
  jobject swigjobj = (jobject)NULL;
  jlong ja         = 0;

  if (!swig_override[0]) {
    SWIG_JavaThrowException(JNIEnvWrapper(this).getJNIEnv(), SWIG_JavaDirectorPureVirtual,
                            "Attempted to invoke pure virtual method ActorCallback::run.");
    return;
  }
  swigjobj = swig_get_self(jenv);
  if (swigjobj && jenv->IsSameObject(swigjobj, NULL) == JNI_FALSE) {
    *((simgrid::s4u::Actor**)&ja) = (simgrid::s4u::Actor*)a;
    jenv->CallStaticVoidMethod(Swig::jclass_simgridJNI, Swig::director_method_ids[1], swigjobj, ja);
    jthrowable swigerror = jenv->ExceptionOccurred();
    if (swigerror) {
      Swig::DirectorException::raise(jenv, swigerror);
    }

  } else {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null upcall object in ActorCallback::run ");
  }
  if (swigjobj)
    jenv->DeleteLocalRef(swigjobj);
}

SwigDirector_ActorCallback::~SwigDirector_ActorCallback()
{
  swig_disconnect_director_self("swigDirectorDisconnect");
}

void SwigDirector_ActorCallback::swig_connect_director(JNIEnv* jenv, jobject jself, jclass jcls, bool swig_mem_own,
                                                       bool weak_global)
{
  static jclass baseclass = swig_new_global_ref(jenv, "org/simgrid/s4u/ActorCallback");
  if (!baseclass)
    return;
  static SwigDirectorMethod methods[] = {SwigDirectorMethod(jenv, baseclass, "run", "(Lorg/simgrid/s4u/Actor;)V")};

  if (swig_set_self(jenv, jself, swig_mem_own, weak_global)) {
    bool derived = (jenv->IsSameObject(baseclass, jcls) ? false : true);
    for (int i = 0; i < 1; ++i) {
      swig_override[i] = false;
      if (derived) {
        jmethodID methid = jenv->GetMethodID(jcls, methods[i].name, methods[i].desc);
        swig_override[i] = methods[i].methid && (methid != methods[i].methid);
        jenv->ExceptionClear();
      }
    }
  }
}

SwigDirector_ActorMain::SwigDirector_ActorMain(JNIEnv* jenv) : ActorMain(), Swig::Director(jenv) {}

void SwigDirector_ActorMain::run()
{
  JNIEnvWrapper swigjnienv(this);
  JNIEnv* jenv     = swigjnienv.getJNIEnv();
  jobject swigjobj = (jobject)NULL;

  if (!swig_override[0]) {
    SWIG_JavaThrowException(JNIEnvWrapper(this).getJNIEnv(), SWIG_JavaDirectorPureVirtual,
                            "Attempted to invoke pure virtual method ActorMain::run.");
    return;
  }
  swigjobj = swig_get_self(jenv);
  if (swigjobj && jenv->IsSameObject(swigjobj, NULL) == JNI_FALSE) {
    jenv->CallStaticVoidMethod(Swig::jclass_simgridJNI, Swig::director_method_ids[2], swigjobj);
    jthrowable swigerror = jenv->ExceptionOccurred();
    if (swigerror) {
      Swig::DirectorException::raise(jenv, swigerror);
    }

  } else {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null upcall object in ActorMain::run ");
  }
  if (swigjobj)
    jenv->DeleteLocalRef(swigjobj);
}

SwigDirector_ActorMain::~SwigDirector_ActorMain()
{
  swig_disconnect_director_self("swigDirectorDisconnect");
}

void SwigDirector_ActorMain::swig_connect_director(JNIEnv* jenv, jobject jself, jclass jcls, bool swig_mem_own,
                                                   bool weak_global)
{
  static jclass baseclass = swig_new_global_ref(jenv, "org/simgrid/s4u/ActorMain");
  if (!baseclass)
    return;
  static SwigDirectorMethod methods[] = {SwigDirectorMethod(jenv, baseclass, "run", "()V")};

  if (swig_set_self(jenv, jself, swig_mem_own, weak_global)) {
    bool derived = (jenv->IsSameObject(baseclass, jcls) ? false : true);
    for (int i = 0; i < 1; ++i) {
      swig_override[i] = false;
      if (derived) {
        jmethodID methid = jenv->GetMethodID(jcls, methods[i].name, methods[i].desc);
        swig_override[i] = methods[i].methid && (methid != methods[i].methid);
        jenv->ExceptionClear();
      }
    }
  }
}

#ifdef __cplusplus
extern "C" {
#endif

/* *****************************************************************************************************************
 * The many JNI calls follow. The 4 first parameters are always the same:
 *  - JNIEnv* jenv: Access to the JVM in case you want some services (as always in JNI)
 *  - jclass jcls: not sure, it's not really used here but it's requested in JNI
 *  - jlong cthis: pointer to the C++ object embedded in an integer (as in SWIG directors)
 *  - jobject jthis: pointer to the Java (as in SWIG directors)
 *
 * Then, all classical parameters to the method follow, using stupid naming conventions inherited from swig.
 * *****************************************************************************************************************/

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_BooleanCallback_1run(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis, jboolean jarg2)
{
  BooleanCallback* arg1 = *(BooleanCallback**)&cthis;
  bool arg2             = jarg2 ? true : false;
  (arg1)->run(arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1BooleanCallback(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  BooleanCallback* arg1 = *(BooleanCallback**)&cthis;
  delete arg1;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_new_1BooleanCallback(JNIEnv* jenv, jclass jcls)
{
  BooleanCallback* result      = (BooleanCallback*)new SwigDirector_BooleanCallback(jenv);
  jlong jresult                = 0;
  *(BooleanCallback**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_BooleanCallback_1director_1connect(JNIEnv* jenv, jclass jcls,
                                                                                           jobject jself, jlong objarg,
                                                                                           jboolean jswig_mem_own,
                                                                                           jboolean jweak_global)
{
  BooleanCallback* obj                   = *((BooleanCallback**)&objarg);
  SwigDirector_BooleanCallback* director = static_cast<SwigDirector_BooleanCallback*>(obj);
  director->swig_connect_director(jenv, jself, jenv->GetObjectClass(jself), (jswig_mem_own == JNI_TRUE),
                                  (jweak_global == JNI_TRUE));
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_BooleanCallback_1change_1ownership(JNIEnv* jenv, jclass jcls,
                                                                                           jobject jself, jlong objarg,
                                                                                           jboolean jtake_or_release)
{
  BooleanCallback* obj                   = *((BooleanCallback**)&objarg);
  SwigDirector_BooleanCallback* director = dynamic_cast<SwigDirector_BooleanCallback*>(obj);
  if (director) {
    director->swig_java_change_ownership(jenv, jself, jtake_or_release ? true : false);
  }
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorCallback_1run(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis, jlong jarg2, jobject jarg2_)
{
  ActorCallback* arg1                               = (ActorCallback*)0;
  simgrid::s4u::Actor* arg2                         = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg2 = 0;

  arg1 = *(ActorCallback**)&cthis;

  // plain pointer
  smartarg2 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&jarg2;
  arg2      = (simgrid::s4u::Actor*)(smartarg2 ? smartarg2->get() : 0);

  (arg1)->run(arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1ActorCallback(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  ActorCallback* arg1 = *(ActorCallback**)&cthis;
  delete arg1;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_new_1ActorCallback(JNIEnv* jenv, jclass jcls)
{
  jlong jresult         = 0;
  ActorCallback* result      = (ActorCallback*)new SwigDirector_ActorCallback(jenv);
  *(ActorCallback**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorCallback_1director_1connect(JNIEnv* jenv, jclass jcls,
                                                                                         jobject jself, jlong objarg,
                                                                                         jboolean jswig_mem_own,
                                                                                         jboolean jweak_global)
{
  ActorCallback* obj                   = *((ActorCallback**)&objarg);
  SwigDirector_ActorCallback* director = static_cast<SwigDirector_ActorCallback*>(obj);
  director->swig_connect_director(jenv, jself, jenv->GetObjectClass(jself), (jswig_mem_own == JNI_TRUE),
                                  (jweak_global == JNI_TRUE));
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorCallback_1change_1ownership(JNIEnv* jenv, jclass jcls,
                                                                                         jobject jself, jlong objarg,
                                                                                         jboolean jtake_or_release)
{
  ActorCallback* obj                   = *((ActorCallback**)&objarg);
  SwigDirector_ActorCallback* director = dynamic_cast<SwigDirector_ActorCallback*>(obj);
  if (director) {
    director->swig_java_change_ownership(jenv, jself, jtake_or_release ? true : false);
  }
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1run(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                       jobject jthis)
{
  ActorMain* arg1 = *(ActorMain**)&cthis;
  (arg1)->run();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1ActorMain(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  ActorMain* arg1 = *(ActorMain**)&cthis;
  delete arg1;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1sleep_1for(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis, jdouble jarg2)
{
  ActorMain* arg1 = *(ActorMain**)&cthis;
  double arg2     = (double)jarg2;
  ActorMain_sleep_for(arg1, arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1sleep_1until(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis, jdouble jarg2)
{
  ActorMain* arg1 = *(ActorMain**)&cthis;
  double arg2     = (double)jarg2;
  ActorMain_sleep_until(arg1, arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1execute_1_1SWIG_10(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis,
                                                                                      jdouble jarg2)
{
  double flop = (double)jarg2;
  simgrid::s4u::this_actor::execute(flop);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1execute_1_1SWIG_11(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis,
                                                                                      jdouble jarg2, jdouble jarg3)
{
  double flop     = (double)jarg2;
  double priority = (double)jarg3;

  simgrid::s4u::this_actor::execute(flop, priority);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1thread_1execute(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis,
                                                                                   jlong jarg2, jdouble jarg3,
                                                                                   jint jarg4)
{
  ActorMain* arg1 = *(ActorMain**)&cthis;
  s4u::Host* arg2 = *(s4u::Host**)&jarg2;
  double arg3     = (double)jarg3;
  int arg4        = (int)jarg4;
  ActorMain_thread_execute(arg1, arg2, arg3, arg4);
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1exec_1init(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jdouble flops_amounts)
{
  auto result = simgrid::s4u::this_actor::exec_init(flops_amounts);
  intrusive_ptr_add_ref(result.get());
  return (jlong)result.get();
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1exec_1async(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis, jdouble flops_amounts)
{
  auto result = simgrid::s4u::this_actor::exec_async(flops_amounts);
  intrusive_ptr_add_ref(result.get());
  return (jlong)result.get();
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1get_1pid(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jobject jthis)
{
  jint jresult    = 0;
  ActorMain* arg1 = (ActorMain*)0;
  long result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(ActorMain**)&cthis;
  result  = (long)ActorMain_get_pid(arg1);
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1get_1ppid(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis)
{
  jint jresult    = 0;
  ActorMain* arg1 = (ActorMain*)0;
  long result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(ActorMain**)&cthis;
  result  = (long)ActorMain_get_ppid(arg1);
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1get_1host(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis)
{
  return (jlong)simgrid::s4u::this_actor::get_host();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1set_1host(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis, jlong jarg2)
{
  ActorMain* arg1 = (ActorMain*)0;
  s4u::Host* arg2 = (s4u::Host*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(ActorMain**)&cthis;
  arg2 = *(s4u::Host**)&jarg2;
  ActorMain_set_host(arg1, arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1suspend(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  ActorMain* arg1 = (ActorMain*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(ActorMain**)&cthis;
  ActorMain_suspend(arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1yield(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                         jobject jthis)
{
  ActorMain* arg1 = (ActorMain*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(ActorMain**)&cthis;
  ActorMain_yield(arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1exit(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis)
{
  ActorMain* arg1 = (ActorMain*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(ActorMain**)&cthis;
  ActorMain_exit(arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1on_1termination_1cb(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis)
{
  ActorCallback* arg1 = *(ActorCallback**)&cthis;
  ActorMain_on_termination_cb(arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1on_1destruction_1cb(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis)
{
  ActorCallback* arg1 = (ActorCallback*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(ActorCallback**)&cthis;
  ActorMain_on_destruction_cb(arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1on_1exit(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jobject jthis, jlong jarg2, jobject jarg2_)
{
  auto code = (BooleanCallback*)jarg2;

  simgrid::s4u::this_actor::on_exit([code](bool b) { code->run(b); });
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_new_1ActorMain(JNIEnv* jenv, jclass jcls)
{
  jlong jresult     = 0;
  ActorMain* result = 0;

  (void)jenv;
  (void)jcls;
  result                 = (ActorMain*)new SwigDirector_ActorMain(jenv);
  *(ActorMain**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1director_1connect(JNIEnv* jenv, jclass jcls,
                                                                                     jobject jself, jlong objarg,
                                                                                     jboolean jswig_mem_own,
                                                                                     jboolean jweak_global)
{
  ActorMain* obj = *((ActorMain**)&objarg);
  (void)jcls;
  SwigDirector_ActorMain* director = static_cast<SwigDirector_ActorMain*>(obj);
  director->swig_connect_director(jenv, jself, jenv->GetObjectClass(jself), (jswig_mem_own == JNI_TRUE),
                                  (jweak_global == JNI_TRUE));
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ActorMain_1change_1ownership(JNIEnv* jenv, jclass jcls,
                                                                                     jobject jself, jlong objarg,
                                                                                     jboolean jtake_or_release)
{
  ActorMain* obj                   = *((ActorMain**)&objarg);
  SwigDirector_ActorMain* director = dynamic_cast<SwigDirector_ActorMain*>(obj);
  (void)jcls;
  if (director) {
    director->swig_java_change_ownership(jenv, jself, jtake_or_release ? true : false);
  }
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_parallel_1execute(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                          jlong jarg2, jlong jarg3)
{
  std::vector<simgrid::s4u::Host*>* arg1 = 0;
  std::vector<double>* arg2              = 0;
  std::vector<double>* arg3              = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::vector<simgrid::s4u::Host*>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::vector< simgrid::s4u::Host * > const & is null");
    return;
  }
  arg2 = *(std::vector<double>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::vector< double > const & is null");
    return;
  }
  arg3 = *(std::vector<double>**)&jarg3;
  if (!arg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::vector< double > const & is null");
    return;
  }
  simgrid::s4u::this_actor::parallel_execute((std::vector<simgrid::s4u::Host*> const&)*arg1,
                                             (std::vector<double> const&)*arg2, (std::vector<double> const&)*arg3);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_on_1exit(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(bool)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(bool)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::function< void (bool) > const & is null");
    return;
  }
  simgrid::s4u::this_actor::on_exit((std::function<void(bool)> const&)*arg1);
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1self(JNIEnv* jenv, jclass jcls)
{
  jlong jresult               = 0;
  simgrid::s4u::Actor* result = 0;

  (void)jenv;
  (void)jcls;
  result = (simgrid::s4u::Actor*)simgrid::s4u::Actor::self();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Actor>(result, SWIG_intrusive_deleter<simgrid::s4u::Actor>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Actor>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1on_1this_1suspend_1cb(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis,
                                                                                     jlong jarg2)
{
  simgrid::s4u::Actor* arg1                             = (simgrid::s4u::Actor*)0;
  std::function<void(simgrid::s4u::Actor const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::Actor const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Actor const &) > const & is null");
    return;
  }
  (arg1)->on_this_suspend_cb((std::function<void(simgrid::s4u::Actor const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1on_1this_1resume_1cb(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis,
                                                                                    jlong jarg2)
{
  simgrid::s4u::Actor* arg1                             = (simgrid::s4u::Actor*)0;
  std::function<void(simgrid::s4u::Actor const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::Actor const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Actor const &) > const & is null");
    return;
  }
  (arg1)->on_this_resume_cb((std::function<void(simgrid::s4u::Actor const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1on_1this_1sleep_1cb(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis,
                                                                                   jlong jarg2)
{
  simgrid::s4u::Actor* arg1                             = (simgrid::s4u::Actor*)0;
  std::function<void(simgrid::s4u::Actor const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::Actor const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Actor const &) > const & is null");
    return;
  }
  (arg1)->on_this_sleep_cb((std::function<void(simgrid::s4u::Actor const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1on_1this_1wake_1up_1cb(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis,
                                                                                      jlong jarg2)
{
  simgrid::s4u::Actor* arg1                             = (simgrid::s4u::Actor*)0;
  std::function<void(simgrid::s4u::Actor const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::Actor const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Actor const &) > const & is null");
    return;
  }
  (arg1)->on_this_wake_up_cb((std::function<void(simgrid::s4u::Actor const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1on_1this_1host_1change_1cb(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jobject jthis,
                                                                                          jlong jarg2)
{
  simgrid::s4u::Actor* arg1                                          = (simgrid::s4u::Actor*)0;
  std::function<void(simgrid::s4u::Actor const&, Host const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1                  = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::Actor const&, Host const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Actor const &,Host const &) > const & is null");
    return;
  }
  (arg1)->on_this_host_change_cb((std::function<void(simgrid::s4u::Actor const&, Host const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1on_1termination_1cb(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis)
{
  std::function<void(simgrid::s4u::Actor const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::Actor const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Actor const &) > const & is null");
    return;
  }
  simgrid::s4u::Actor::on_termination_cb((std::function<void(simgrid::s4u::Actor const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1on_1this_1termination_1cb(JNIEnv* jenv, jclass jcls,
                                                                                         jlong cthis, jobject jthis,
                                                                                         jlong jarg2)
{
  simgrid::s4u::Actor* arg1                             = (simgrid::s4u::Actor*)0;
  std::function<void(simgrid::s4u::Actor const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::Actor const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Actor const &) > const & is null");
    return;
  }
  (arg1)->on_this_termination_cb((std::function<void(simgrid::s4u::Actor const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1on_1destruction_1cb(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis)
{
  std::function<void(simgrid::s4u::Actor const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::Actor const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Actor const &) > const & is null");
    return;
  }
  simgrid::s4u::Actor::on_destruction_cb((std::function<void(simgrid::s4u::Actor const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1on_1this_1destruction_1cb(JNIEnv* jenv, jclass jcls,
                                                                                         jlong cthis, jobject jthis,
                                                                                         jlong jarg2)
{
  simgrid::s4u::Actor* arg1                             = (simgrid::s4u::Actor*)0;
  std::function<void(simgrid::s4u::Actor const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::Actor const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Actor const &) > const & is null");
    return;
  }
  (arg1)->on_this_destruction_cb((std::function<void(simgrid::s4u::Actor const&)> const&)*arg2);
}

std::string java_string_to_std_string(JNIEnv* jenv, jstring jstr)
{
  if (!jstr) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return std::string();
  }
  const char* pstr = (const char*)jenv->GetStringUTFChars(jstr, 0);
  if (!pstr) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return std::string();
  }
  std::string str(pstr);
  jenv->ReleaseStringUTFChars(jstr, pstr);
  return str;
}
XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1init(JNIEnv* jenv, jclass jcls, jstring cthis,
                                                                     jlong jarg2, jobject jarg2_)
{
  jlong jresult                                    = 0;
  simgrid::s4u::Host* arg2                         = (simgrid::s4u::Host*)0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg2 = 0;
  boost::intrusive_ptr<simgrid::s4u::Actor> result;

  std::string arg1 = java_string_to_std_string(jenv, cthis);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg2;
  arg2      = (simgrid::s4u::Host*)(smartarg2 ? smartarg2->get() : 0);

  result = simgrid::s4u::Actor::init(arg1, arg2);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Actor>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Actor>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1set_1stacksize(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jlong jarg2)
{
  jlong jresult             = 0;
  simgrid::s4u::Actor* arg1 = (simgrid::s4u::Actor*)0;
  unsigned int arg2;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1 = 0;
  boost::intrusive_ptr<simgrid::s4u::Actor> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = (unsigned int)jarg2;
  result = (arg1)->set_stacksize(arg2);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Actor>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Actor>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1start_1_1SWIG_10(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis, jlong jarg2)
{
  jlong jresult                                     = 0;
  simgrid::s4u::Actor* arg1                         = (simgrid::s4u::Actor*)0;
  std::function<void()>* arg2                       = 0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1 = 0;
  boost::intrusive_ptr<simgrid::s4u::Actor> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void()>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::function< void () > const & is null");
    return 0;
  }
  result = (arg1)->start((std::function<void()> const&)*arg2);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Actor>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Actor>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1start_1_1SWIG_11(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis, jlong jarg2,
                                                                                 jobjectArray jarg3)
{
  jlong jresult               = 0;
  simgrid::s4u::Actor* arg1   = (simgrid::s4u::Actor*)0;
  std::function<void()>* arg2 = 0;
  SwigValueWrapper<std::vector<std::string>> arg3;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1 = 0;
  boost::intrusive_ptr<simgrid::s4u::Actor> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void()>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::function< void () > const & is null");
    return 0;
  }
  {
    int len = (int)jenv->GetArrayLength(jarg3);
    (&arg3)->reserve(len);
    for (jsize i = 0; i < len; ++i) {
      jstring j_string     = (jstring)jenv->GetObjectArrayElement(jarg3, i);
      const char* c_string = jenv->GetStringUTFChars(j_string, 0);
      (&arg3)->push_back(c_string);
      jenv->ReleaseStringUTFChars(j_string, c_string);
      jenv->DeleteLocalRef(j_string);
    }
  }
  result = (arg1)->start((std::function<void()> const&)*arg2, arg3);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Actor>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Actor>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1daemonize(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                          jobject jthis)
{
  jlong jresult                                     = 0;
  simgrid::s4u::Actor* arg1                         = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1 = 0;
  simgrid::s4u::Actor* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Actor*)(arg1)->daemonize();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Actor>(result, SWIG_intrusive_deleter<simgrid::s4u::Actor>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Actor>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1is_1daemon(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis)
{
  jboolean jresult                                        = 0;
  simgrid::s4u::Actor* arg1                               = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor const>* smartarg1 = 0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  result  = (bool)((simgrid::s4u::Actor const*)arg1)->is_daemon();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1is_1maestro(JNIEnv* jenv, jclass jcls)
{
  jboolean jresult = 0;
  bool result;

  (void)jenv;
  (void)jcls;
  result  = (bool)simgrid::s4u::Actor::is_maestro();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1get_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jobject jthis)
{
  jstring jresult                                         = 0;
  simgrid::s4u::Actor* arg1                               = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor const>* smartarg1 = 0;
  char* result                                            = 0;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  result = (char*)((simgrid::s4u::Actor const*)arg1)->get_cname();
  if (result)
    jresult = jenv->NewStringUTF((const char*)result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1get_1host(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                          jobject jthis)
{
  jlong jresult                                           = 0;
  simgrid::s4u::Actor* arg1                               = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor const>* smartarg1 = 0;
  simgrid::s4u::Host* result                              = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Host*)((simgrid::s4u::Actor const*)arg1)->get_host();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Host>(result, SWIG_intrusive_deleter<simgrid::s4u::Host>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Host>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1get_1pid(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis)
{
  jint jresult                                            = 0;
  simgrid::s4u::Actor* arg1                               = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor const>* smartarg1 = 0;
  aid_t result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  result  = ((simgrid::s4u::Actor const*)arg1)->get_pid();
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1get_1ppid(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                         jobject jthis)
{
  jint jresult                                            = 0;
  simgrid::s4u::Actor* arg1                               = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor const>* smartarg1 = 0;
  aid_t result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  result  = ((simgrid::s4u::Actor const*)arg1)->get_ppid();
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1suspend(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                       jobject jthis)
{
  simgrid::s4u::Actor* arg1                         = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->suspend();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1resume(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                      jobject jthis)
{
  simgrid::s4u::Actor* arg1                         = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->resume();
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1is_1suspended(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis)
{
  jboolean jresult                                        = 0;
  simgrid::s4u::Actor* arg1                               = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor const>* smartarg1 = 0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  result  = (bool)((simgrid::s4u::Actor const*)arg1)->is_suspended();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1set_1auto_1restart_1_1SWIG_10(JNIEnv* jenv, jclass jcls,
                                                                                              jlong cthis,
                                                                                              jobject jthis,
                                                                                              jboolean jarg2)
{
  jlong jresult             = 0;
  simgrid::s4u::Actor* arg1 = (simgrid::s4u::Actor*)0;
  bool arg2;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1 = 0;
  simgrid::s4u::Actor* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = jarg2 ? true : false;
  result = (simgrid::s4u::Actor*)(arg1)->set_auto_restart(arg2);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Actor>(result, SWIG_intrusive_deleter<simgrid::s4u::Actor>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Actor>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1set_1auto_1restart_1_1SWIG_11(JNIEnv* jenv, jclass jcls,
                                                                                              jlong cthis,
                                                                                              jobject jthis)
{
  jlong jresult                                     = 0;
  simgrid::s4u::Actor* arg1                         = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1 = 0;
  simgrid::s4u::Actor* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Actor*)(arg1)->set_auto_restart();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Actor>(result, SWIG_intrusive_deleter<simgrid::s4u::Actor>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Actor>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1get_1restart_1count(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis)
{
  jint jresult                                            = 0;
  simgrid::s4u::Actor* arg1                               = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor const>* smartarg1 = 0;
  int result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  result  = (int)((simgrid::s4u::Actor const*)arg1)->get_restart_count();
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1on_1exit(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis, jlong jarg2)
{
  simgrid::s4u::Actor* arg1                               = (simgrid::s4u::Actor*)0;
  std::function<void(bool)>* arg2                         = 0;
  boost::shared_ptr<simgrid::s4u::Actor const>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(bool)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::function< void (bool) > const & is null");
    return;
  }
  ((simgrid::s4u::Actor const*)arg1)->on_exit((std::function<void(bool)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1set_1kill_1time(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jdouble jarg2)
{
  simgrid::s4u::Actor* arg1 = (simgrid::s4u::Actor*)0;
  double arg2;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = (double)jarg2;
  (arg1)->set_kill_time(arg2);
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1get_1kill_1time(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis)
{
  jdouble jresult                                         = 0;
  simgrid::s4u::Actor* arg1                               = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor const>* smartarg1 = 0;
  double result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  result  = (double)((simgrid::s4u::Actor const*)arg1)->get_kill_time();
  jresult = (jdouble)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1set_1host(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                         jobject jthis, jlong jarg2, jobject jarg2_)
{
  simgrid::s4u::Actor* arg1                         = (simgrid::s4u::Actor*)0;
  simgrid::s4u::Host* arg2                          = (simgrid::s4u::Host*)0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1 = 0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg2  = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg2;
  arg2      = (simgrid::s4u::Host*)(smartarg2 ? smartarg2->get() : 0);

  (arg1)->set_host(arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1kill(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                    jobject jthis)
{
  simgrid::s4u::Actor* arg1                         = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->kill();
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1by_1pid(JNIEnv* jenv, jclass jcls, jint cthis)
{
  jlong jresult = 0;
  aid_t arg1;
  boost::intrusive_ptr<simgrid::s4u::Actor> result;

  (void)jenv;
  (void)jcls;
  arg1   = (aid_t)cthis;
  result = simgrid::s4u::Actor::by_pid(std::move(arg1));

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Actor>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Actor>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1join_1_1SWIG_10(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis)
{
  simgrid::s4u::Actor* arg1                               = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor const>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  ((simgrid::s4u::Actor const*)arg1)->join();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1join_1_1SWIG_11(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jdouble jarg2)
{
  simgrid::s4u::Actor* arg1 = (simgrid::s4u::Actor*)0;
  double arg2;
  boost::shared_ptr<simgrid::s4u::Actor const>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = (double)jarg2;
  ((simgrid::s4u::Actor const*)arg1)->join(arg2);
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1restart(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis)
{
  jlong jresult                                     = 0;
  simgrid::s4u::Actor* arg1                         = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1 = 0;
  simgrid::s4u::Actor* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Actor*)(arg1)->restart();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Actor>(result, SWIG_intrusive_deleter<simgrid::s4u::Actor>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Actor>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1kill_1all(JNIEnv* jenv, jclass jcls)
{
  (void)jenv;
  (void)jcls;
  simgrid::s4u::Actor::kill_all();
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1get_1property(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis, jstring jarg2)
{
  jstring jresult                                         = 0;
  simgrid::s4u::Actor* arg1                               = (simgrid::s4u::Actor*)0;
  std::string* arg2                                       = 0;
  boost::shared_ptr<simgrid::s4u::Actor const>* smartarg1 = 0;
  char* result                                            = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = (char*)((simgrid::s4u::Actor const*)arg1)->get_property((std::string const&)*arg2);
  if (result)
    jresult = jenv->NewStringUTF((const char*)result);
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1set_1property(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis, jstring jarg2,
                                                                             jstring jarg3)
{
  simgrid::s4u::Actor* arg1                         = (simgrid::s4u::Actor*)0;
  std::string* arg2                                 = 0;
  std::string* arg3                                 = 0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  if (!jarg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return;
  }
  const char* arg3_pstr = (const char*)jenv->GetStringUTFChars(jarg3, 0);
  if (!arg3_pstr)
    return;
  std::string arg3_str(arg3_pstr);
  arg3 = &arg3_str;
  jenv->ReleaseStringUTFChars(jarg3, arg3_pstr);
  (arg1)->set_property((std::string const&)*arg2, (std::string const&)*arg3);
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Actor_1create(JNIEnv* jenv, jclass jcls, jstring cthis,
                                                                       jlong jarg2, jobject jarg2_, jlong jarg3,
                                                                       jobject jarg3_)
{
  jlong jresult                                    = 0;
  std::string* arg1                                = 0;
  simgrid::s4u::Host* arg2                         = (simgrid::s4u::Host*)0;
  ActorMain* arg3                                  = (ActorMain*)0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg2 = 0;
  boost::intrusive_ptr<simgrid::s4u::Actor> result;

  (void)jenv;
  (void)jcls;
  (void)jarg2_;
  (void)jarg3_;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg2;
  arg2      = (simgrid::s4u::Host*)(smartarg2 ? smartarg2->get() : 0);

  arg3   = *(ActorMain**)&jarg3;
  result = simgrid_s4u_Actor_create_java((std::string const&)*arg1, arg2, arg3);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Actor>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Actor>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1Actor(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  simgrid::s4u::Actor* arg1                         = (simgrid::s4u::Actor*)0;
  boost::shared_ptr<simgrid::s4u::Actor>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Actor>**)&cthis;
  arg1      = (simgrid::s4u::Actor*)(smartarg1 ? smartarg1->get() : 0);

  (void)arg1;
  delete smartarg1;
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1to_1c_1str(JNIEnv* jenv, jclass jcls, jint cthis)
{
  jstring jresult = 0;
  simgrid::s4u::Activity::State arg1;
  char* result = 0;

  (void)jenv;
  (void)jcls;
  arg1   = (simgrid::s4u::Activity::State)cthis;
  result = (char*)simgrid::s4u::Activity::to_c_str(arg1);
  if (result)
    jresult = jenv->NewStringUTF((const char*)result);
  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1is_1assigned(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis)
{
  return ((Activity*)cthis)->is_assigned();
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1dependencies_1solved(JNIEnv* jenv, jclass jcls,
                                                                                           jlong cthis, jobject jthis)
{
  return ((Activity*)cthis)->dependencies_solved();
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1has_1no_1successor(JNIEnv* jenv, jclass jcls,
                                                                                         jlong cthis, jobject jthis)
{
  return ((Activity*)cthis)->has_no_successor();
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1get_1dependencies(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis)
{
  jlong jresult                                                  = 0;
  simgrid::s4u::Activity* arg1                                   = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity const>* smartarg1     = 0;
  std::set<boost::intrusive_ptr<simgrid::s4u::Activity>>* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  result = (std::set<boost::intrusive_ptr<simgrid::s4u::Activity>>*)&((simgrid::s4u::Activity const*)arg1)
               ->get_dependencies();
  *(std::set<boost::intrusive_ptr<simgrid::s4u::Activity>>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1get_1successors(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis)
{
  jlong jresult                                                     = 0;
  simgrid::s4u::Activity* arg1                                      = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity const>* smartarg1        = 0;
  std::vector<boost::intrusive_ptr<simgrid::s4u::Activity>>* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  result = (std::vector<boost::intrusive_ptr<simgrid::s4u::Activity>>*)&((simgrid::s4u::Activity const*)arg1)
               ->get_successors();
  *(std::vector<boost::intrusive_ptr<simgrid::s4u::Activity>>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1Activity(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  intrusive_ptr_release((Activity*)cthis);
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1get_1vetoed_1activities(JNIEnv* jenv, jclass jcls)
{
  jlong jresult                             = 0;
  std::set<simgrid::s4u::Activity*>* result = 0;

  (void)jenv;
  (void)jcls;
  result = (std::set<simgrid::s4u::Activity*>*)simgrid::s4u::Activity::get_vetoed_activities();
  *(std::set<simgrid::s4u::Activity*>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1set_1vetoed_1activities(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis)
{
  std::set<simgrid::s4u::Activity*>* arg1 = (std::set<simgrid::s4u::Activity*>*)0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::set<simgrid::s4u::Activity*>**)&cthis;
  simgrid::s4u::Activity::set_vetoed_activities(arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1start(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis)
{
  ((Activity*)cthis)->start();
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1test(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  return ((Activity*)cthis)->test();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1wait_1for(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jobject jthis, jdouble jarg2)
{
  ((Activity*)cthis)->wait_for(jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1wait_1for_1or_1cancel(JNIEnv* jenv, jclass jcls,
                                                                                        jlong cthis, jobject jthis,
                                                                                        jdouble jarg2)
{
  ((Activity*)cthis)->wait_for(jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1wait_1until(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis, jdouble jarg2)
{
  ((Activity*)cthis)->wait_until(jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1cancel(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                         jobject jthis)
{
  ((Activity*)cthis)->cancel();
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1get_1state(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis)
{
  return (jint)((Activity*)cthis)->get_state();
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1get_1state_1str(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis)
{
  jstring jresult                                            = 0;
  simgrid::s4u::Activity* arg1                               = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity const>* smartarg1 = 0;
  char* result                                               = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  result = (char*)((simgrid::s4u::Activity const*)arg1)->get_state_str();
  if (result)
    jresult = jenv->NewStringUTF((const char*)result);
  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1is_1canceled(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis)
{
  return ((Activity*)cthis)->is_canceled();
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1is_1failed(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis)
{
  return ((Activity*)cthis)->is_failed();
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1is_1done(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis)
{
  return ((Activity*)cthis)->is_done();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1set_1state(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis, jint jarg2)
{
  return ((Activity*)cthis)->set_state((Activity::State)jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1set_1detached(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis, jboolean jarg2)
{
  simgrid::s4u::Activity* arg1 = (simgrid::s4u::Activity*)0;
  bool arg2;
  boost::shared_ptr<simgrid::s4u::Activity>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = jarg2 ? true : false;
  (arg1)->set_detached(arg2);
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1is_1detached(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis)
{
  jboolean jresult                                           = 0;
  simgrid::s4u::Activity* arg1                               = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity const>* smartarg1 = 0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  result  = (bool)((simgrid::s4u::Activity const*)arg1)->is_detached();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1suspend(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  jlong jresult                                        = 0;
  simgrid::s4u::Activity* arg1                         = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity>* smartarg1 = 0;
  simgrid::s4u::Activity* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Activity*)(arg1)->suspend();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Activity>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Activity>(result, SWIG_intrusive_deleter<simgrid::s4u::Activity>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Activity>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Activity>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Activity>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1resume(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                          jobject jthis)
{
  jlong jresult                                        = 0;
  simgrid::s4u::Activity* arg1                         = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity>* smartarg1 = 0;
  simgrid::s4u::Activity* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Activity*)(arg1)->resume();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Activity>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Activity>(result, SWIG_intrusive_deleter<simgrid::s4u::Activity>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Activity>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Activity>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Activity>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1is_1suspended(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis)
{
  jboolean jresult                                           = 0;
  simgrid::s4u::Activity* arg1                               = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity const>* smartarg1 = 0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  result  = (bool)((simgrid::s4u::Activity const*)arg1)->is_suspended();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1get_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis)
{
  jstring jresult                                            = 0;
  simgrid::s4u::Activity* arg1                               = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity const>* smartarg1 = 0;
  char* result                                               = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  result = (char*)((simgrid::s4u::Activity const*)arg1)->get_cname();
  if (result)
    jresult = jenv->NewStringUTF((const char*)result);
  return jresult;
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1get_1remaining(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis)
{
  jdouble jresult                                            = 0;
  simgrid::s4u::Activity* arg1                               = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity const>* smartarg1 = 0;
  double result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  result  = (double)((simgrid::s4u::Activity const*)arg1)->get_remaining();
  jresult = (jdouble)result;
  return jresult;
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1get_1start_1time(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis)
{
  jdouble jresult                                            = 0;
  simgrid::s4u::Activity* arg1                               = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity const>* smartarg1 = 0;
  double result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  result  = (double)((simgrid::s4u::Activity const*)arg1)->get_start_time();
  jresult = (jdouble)result;
  return jresult;
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1get_1finish_1time(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis)
{
  jdouble jresult                                            = 0;
  simgrid::s4u::Activity* arg1                               = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity const>* smartarg1 = 0;
  double result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  result  = (double)((simgrid::s4u::Activity const*)arg1)->get_finish_time();
  jresult = (jdouble)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1mark(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                       jobject jthis)
{
  simgrid::s4u::Activity* arg1                         = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->mark();
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1is_1marked(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis)
{
  jboolean jresult                                           = 0;
  simgrid::s4u::Activity* arg1                               = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity const>* smartarg1 = 0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  result  = (bool)((simgrid::s4u::Activity const*)arg1)->is_marked();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1add_1ref(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jobject jthis)
{
  jlong jresult                                        = 0;
  simgrid::s4u::Activity* arg1                         = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity>* smartarg1 = 0;
  simgrid::s4u::Activity* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Activity*)(arg1)->add_ref();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Activity>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Activity>(result, SWIG_intrusive_deleter<simgrid::s4u::Activity>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Activity>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Activity>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Activity>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Activity_1unref(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis)
{
  simgrid::s4u::Activity* arg1                         = (simgrid::s4u::Activity*)0;
  boost::shared_ptr<simgrid::s4u::Activity>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Activity>**)&cthis;
  arg1      = (simgrid::s4u::Activity*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->unref();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1on_1start_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::Exec const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::Exec const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Exec const &) > const & is null");
    return;
  }
  simgrid::s4u::Activity_T<simgrid::s4u::Exec>::on_start_cb(
      (std::function<void(simgrid::s4u::Exec const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1on_1this_1start_1cb(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis,
                                                                                  jlong jarg2)
{
  std::function<void(simgrid::s4u::Exec const&)>* arg2 = *(std::function<void(simgrid::s4u::Exec const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Exec const &) > const & is null");
    return;
  }
  ((Exec*)cthis)->on_this_start_cb((std::function<void(simgrid::s4u::Exec const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1on_1completion_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::Exec const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::Exec const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Exec const &) > const & is null");
    return;
  }
  simgrid::s4u::Activity_T<simgrid::s4u::Exec>::on_completion_cb(
      (std::function<void(simgrid::s4u::Exec const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1on_1this_1completion_1cb(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis,
                                                                                       jlong jarg2)
{
  std::function<void(simgrid::s4u::Exec const&)>* arg2 = *(std::function<void(simgrid::s4u::Exec const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Exec const &) > const & is null");
    return;
  }
  ((Exec*)cthis)->on_this_completion_cb((std::function<void(simgrid::s4u::Exec const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1on_1this_1suspend_1cb(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis,
                                                                                    jlong jarg2)
{
  std::function<void(simgrid::s4u::Exec const&)>* arg2 = *(std::function<void(simgrid::s4u::Exec const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Exec const &) > const & is null");
    return;
  }
  ((Exec*)cthis)->on_this_suspend_cb((std::function<void(simgrid::s4u::Exec const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1on_1this_1resume_1cb(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis,
                                                                                   jlong jarg2)
{
  std::function<void(simgrid::s4u::Exec const&)>* arg2 = *(std::function<void(simgrid::s4u::Exec const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Exec const &) > const & is null");
    return;
  }
  ((Exec*)cthis)->on_this_resume_cb((std::function<void(simgrid::s4u::Exec const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1on_1veto_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::Exec&)>* arg1 = *(std::function<void(simgrid::s4u::Exec&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Exec &) > const & is null");
    return;
  }
  simgrid::s4u::Activity_T<simgrid::s4u::Exec>::on_veto_cb((std::function<void(simgrid::s4u::Exec&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1on_1this_1veto_1cb(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis, jlong jarg2)
{
  std::function<void(simgrid::s4u::Exec&)>* arg2 = *(std::function<void(simgrid::s4u::Exec&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Exec &) > const & is null");
    return;
  }
  ((Exec*)cthis)->on_this_veto_cb((std::function<void(simgrid::s4u::Exec&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1add_1successor(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis, jlong cother,
                                                                             jobject jother)
{
  ActivityPtr other = reinterpret_cast<Activity*>(cother);
  auto self = (Exec*)cthis;
  self->add_successor(other);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1remove_1successor(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis, jlong cother,
                                                                                jobject jother)
{
  auto* self = (Exec*)cthis;
  ActivityPtr other = (Activity*)cother;
  self->remove_successor(other);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1set_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis, jstring jarg2)
{
  auto* self = (Exec*)cthis;
  self->set_name(java_string_to_std_string(jenv, jarg2));
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1get_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  auto* self         = (Exec*)cthis;
  const char* result = self->get_cname();

  if (result)
    return jenv->NewStringUTF(result);
  return 0;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1set_1tracing_1category(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis,
                                                                                     jstring jarg2)
{
  auto self = (Exec*)cthis;
  self->set_tracing_category(java_string_to_std_string(jenv, jarg2));
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1get_1tracing_1category(JNIEnv* jenv, jclass jcls,
                                                                                        jlong cthis, jobject jthis)
{
  std::string result = ((Exec*)cthis)->get_tracing_category();
  return jenv->NewStringUTF(result.c_str());
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1get_1clean_1function(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis)
{
  jlong jresult                                      = 0;
  simgrid::s4u::Activity_T<simgrid::s4u::Exec>* arg1 = (simgrid::s4u::Activity_T<simgrid::s4u::Exec>*)0;
  boost::shared_ptr<simgrid::s4u::Activity_T<simgrid::s4u::Exec> const>* smartarg1 = 0;
  std::function<void(void*)>* result                                               = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity_T<simgrid::s4u::Exec>>**)&cthis;
  arg1      = (simgrid::s4u::Activity_T<simgrid::s4u::Exec>*)(smartarg1 ? smartarg1->get() : 0);

  result =
      (std::function<void(void*)>*)&((simgrid::s4u::Activity_T<simgrid::s4u::Exec> const*)arg1)->get_clean_function();
  *(std::function<void(void*)>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1detach_1_1SWIG_10(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis)
{
  ((Exec*)cthis)->detach();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1detach_1_1SWIG_11(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis, jlong jarg2)
{
  std::function<void(void*)>* arg2 = *(std::function<void(void*)>**)&jarg2;
  if (arg2)
    ((Exec*)cthis)->detach();
  else
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::function< void (void *) > const & is null");
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1cancel(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                     jobject jthis)
{
  auto* self = (Exec*)cthis;
  self->cancel();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1wait_1for(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis, jdouble jarg2)
{
  auto* self = (Exec*)cthis;
  self->wait_for(jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1on_1start_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::Io const&)>* arg1 = *(std::function<void(simgrid::s4u::Io const&)>**)&cthis;
  if (arg1)
    simgrid::s4u::Io::on_start_cb((std::function<void(simgrid::s4u::Io const&)> const&)*arg1);
  else
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Io const &) > const & is null");
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1on_1this_1start_1cb(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis, jlong jarg2)
{
  std::function<void(simgrid::s4u::Io const&)>* arg2 = *(std::function<void(simgrid::s4u::Io const&)>**)&jarg2;
  if (arg2)
    ((Io*)cthis)->on_this_start_cb(*arg2);
  else
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Io const &) > const & is null");
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1on_1completion_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::Io const&)>* arg1 = *(std::function<void(simgrid::s4u::Io const&)>**)&cthis;
  if (arg1)
    simgrid::s4u::Io::on_completion_cb(*arg1);
  else
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Io const &) > const & is null");
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1on_1this_1completion_1cb(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis,
                                                                                     jlong jarg2)
{
  std::function<void(simgrid::s4u::Io const&)>* arg2 = *(std::function<void(simgrid::s4u::Io const&)>**)&jarg2;
  if (arg2)
    ((Io*)cthis)->on_this_completion_cb((std::function<void(simgrid::s4u::Io const&)> const&)*arg2);
  else
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Io const &) > const & is null");
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1on_1this_1suspend_1cb(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis,
                                                                                  jlong jarg2)
{
  std::function<void(simgrid::s4u::Io const&)>* arg2 = *(std::function<void(simgrid::s4u::Io const&)>**)&jarg2;
  if (arg2)
    ((Io*)cthis)->on_this_suspend_cb((std::function<void(simgrid::s4u::Io const&)> const&)*arg2);
  else
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Io const &) > const & is null");
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1on_1this_1resume_1cb(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis, jlong jarg2)
{
  std::function<void(simgrid::s4u::Io const&)>* arg2 = *(std::function<void(simgrid::s4u::Io const&)>**)&jarg2;
  if (arg2)
    ((Io*)cthis)->on_this_resume_cb((std::function<void(simgrid::s4u::Io const&)> const&)*arg2);
  else
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Io const &) > const & is null");
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1on_1veto_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::Io&)>* arg1 = *(std::function<void(simgrid::s4u::Io&)>**)&cthis;
  if (arg1)
    simgrid::s4u::Io::on_veto_cb((std::function<void(simgrid::s4u::Io&)> const&)*arg1);
  else
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Io &) > const & is null");
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1on_1this_1veto_1cb(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jlong jarg2)
{
  std::function<void(simgrid::s4u::Io&)>* arg2 = *(std::function<void(simgrid::s4u::Io&)>**)&jarg2;
  if (arg2)
    ((Io*)cthis)->on_this_veto_cb((std::function<void(simgrid::s4u::Io&)> const&)*arg2);
  else
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Io &) > const & is null");
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1add_1successor(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis, jlong jarg2, jobject jarg2_)
{
  auto* self = (Io*)cthis;
  self->add_successor((Activity*)jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1remove_1successor(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis, jlong jarg2,
                                                                              jobject jarg2_)
{
  auto* self = (Io*)cthis;
  self->remove_successor((Activity*)jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1set_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                      jobject jthis, jstring jarg2)
{
  auto* self = (Io*)cthis;
  self->set_name(java_string_to_std_string(jenv, jarg2));
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1get_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                         jobject jthis)
{
  auto* self         = (Io*)cthis;
  const char* result = self->get_cname();

  if (result)
    return jenv->NewStringUTF(result);
  return 0;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1set_1tracing_1category(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis,
                                                                                   jstring jarg2)
{
  auto* self = (Io*)cthis;
  self->set_tracing_category(java_string_to_std_string(jenv, jarg2));
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1get_1tracing_1category(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis)
{
  std::string result = ((Io*)cthis)->get_tracing_category();
  return jenv->NewStringUTF(result.c_str());
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1get_1clean_1function(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis)
{
  jlong jresult                                    = 0;
  simgrid::s4u::Activity_T<simgrid::s4u::Io>* arg1 = (simgrid::s4u::Activity_T<simgrid::s4u::Io>*)0;
  boost::shared_ptr<simgrid::s4u::Activity_T<simgrid::s4u::Io> const>* smartarg1 = 0;
  std::function<void(void*)>* result                                             = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity_T<simgrid::s4u::Io>>**)&cthis;
  arg1      = (simgrid::s4u::Activity_T<simgrid::s4u::Io>*)(smartarg1 ? smartarg1->get() : 0);

  result =
      (std::function<void(void*)>*)&((simgrid::s4u::Activity_T<simgrid::s4u::Io> const*)arg1)->get_clean_function();
  *(std::function<void(void*)>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1detach_1_1SWIG_10(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis)
{
  ((Io*)cthis)->detach();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1detach_1_1SWIG_11(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis, jlong jarg2)
{
  std::function<void(void*)>* arg2 = *(std::function<void(void*)>**)&jarg2;
  if (!arg2)
    ((Io*)cthis)->detach((std::function<void(void*)> const&)*arg2);
  else
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::function< void (void *) > const & is null");
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1cancel(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                   jobject jthis)
{
  auto* self = (Io*)cthis;
  self->cancel();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1wait_1for(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                      jobject jthis, jdouble jarg2)
{
  auto* self = (Io*)cthis;
  self->wait_for(jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1Io(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  intrusive_ptr_release((Io*)cthis);
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Barrier_1create(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  jlong jresult = 0;
  unsigned int arg1;
  boost::intrusive_ptr<simgrid::s4u::Barrier> result;

  (void)jenv;
  (void)jcls;
  arg1   = (unsigned int)cthis;
  result = simgrid::s4u::Barrier::create(arg1);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Barrier>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Barrier>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Barrier>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Barrier>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Barrier_1to_1string(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis)
{
  jstring jresult                                           = 0;
  simgrid::s4u::Barrier* arg1                               = (simgrid::s4u::Barrier*)0;
  boost::shared_ptr<simgrid::s4u::Barrier const>* smartarg1 = 0;
  std::string result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Barrier>**)&cthis;
  arg1      = (simgrid::s4u::Barrier*)(smartarg1 ? smartarg1->get() : 0);

  result  = ((simgrid::s4u::Barrier const*)arg1)->to_string();
  jresult = jenv->NewStringUTF((&result)->c_str());
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1Barrier(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  simgrid::s4u::Barrier* arg1                         = (simgrid::s4u::Barrier*)0;
  boost::shared_ptr<simgrid::s4u::Barrier>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Barrier>**)&cthis;
  arg1      = (simgrid::s4u::Barrier*)(smartarg1 ? smartarg1->get() : 0);

  (void)arg1;
  delete smartarg1;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1on_1send_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::Comm const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::Comm const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Comm const &) > const & is null");
    return;
  }
  simgrid::s4u::Comm::on_send_cb((std::function<void(simgrid::s4u::Comm const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1on_1this_1send_1cb(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis, jlong jarg2)
{
  simgrid::s4u::Comm* arg1                             = (simgrid::s4u::Comm*)0;
  std::function<void(simgrid::s4u::Comm const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Comm>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Comm>**)&cthis;
  arg1      = (simgrid::s4u::Comm*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::Comm const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Comm const &) > const & is null");
    return;
  }
  (arg1)->on_this_send_cb((std::function<void(simgrid::s4u::Comm const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1on_1recv_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::Comm const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::Comm const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Comm const &) > const & is null");
    return;
  }
  simgrid::s4u::Comm::on_recv_cb((std::function<void(simgrid::s4u::Comm const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1on_1this_1recv_1cb(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis, jlong jarg2)
{
  simgrid::s4u::Comm* arg1                             = (simgrid::s4u::Comm*)0;
  std::function<void(simgrid::s4u::Comm const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Comm>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Comm>**)&cthis;
  arg1      = (simgrid::s4u::Comm*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::Comm const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Comm const &) > const & is null");
    return;
  }
  (arg1)->on_this_recv_cb((std::function<void(simgrid::s4u::Comm const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1Comm(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  intrusive_ptr_release((Comm*)cthis);
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1sendto_1init_1_1SWIG_10(JNIEnv* jenv, jclass jcls)
{
  jlong jresult = 0;
  boost::intrusive_ptr<simgrid::s4u::Comm> result;

  (void)jenv;
  (void)jcls;
  result = simgrid::s4u::Comm::sendto_init();

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Comm>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Comm>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Comm>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Comm>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1sendto_1init_1_1SWIG_11(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis,
                                                                                       jlong jarg2, jobject jarg2_)
{
  jlong jresult                                    = 0;
  simgrid::s4u::Host* arg1                         = (simgrid::s4u::Host*)0;
  simgrid::s4u::Host* arg2                         = (simgrid::s4u::Host*)0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg1 = 0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg2 = 0;
  boost::intrusive_ptr<simgrid::s4u::Comm> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Host>**)&cthis;
  arg1      = (simgrid::s4u::Host*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg2;
  arg2      = (simgrid::s4u::Host*)(smartarg2 ? smartarg2->get() : 0);

  result = simgrid::s4u::Comm::sendto_init(arg1, arg2);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Comm>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Comm>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Comm>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Comm>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1sendto_1async(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis, jlong jarg2, jobject jarg2_,
                                                                             jlong jarg3)
{
  jlong jresult            = 0;
  simgrid::s4u::Host* arg1 = (simgrid::s4u::Host*)0;
  simgrid::s4u::Host* arg2 = (simgrid::s4u::Host*)0;
  uint64_t arg3;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg1 = 0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg2 = 0;
  boost::intrusive_ptr<simgrid::s4u::Comm> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Host>**)&cthis;
  arg1      = (simgrid::s4u::Host*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg2;
  arg2      = (simgrid::s4u::Host*)(smartarg2 ? smartarg2->get() : 0);

  arg3   = (uint64_t)jarg3;
  result = simgrid::s4u::Comm::sendto_async(arg1, arg2, std::move(arg3));

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Comm>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Comm>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Comm>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Comm>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1sendto(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                     jobject jthis, jlong jarg2, jobject jarg2_,
                                                                     jlong jarg3)
{
  simgrid::s4u::Host* arg1 = (simgrid::s4u::Host*)0;
  simgrid::s4u::Host* arg2 = (simgrid::s4u::Host*)0;
  uint64_t arg3;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg1 = 0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg2 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Host>**)&cthis;
  arg1      = (simgrid::s4u::Host*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg2;
  arg2      = (simgrid::s4u::Host*)(smartarg2 ? smartarg2->get() : 0);

  arg3 = (uint64_t)jarg3;
  simgrid::s4u::Comm::sendto(arg1, arg2, std::move(arg3));
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1set_1source(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis, jlong jarg2, jobject jarg2_)
{
  jlong jresult                                    = 0;
  simgrid::s4u::Comm* arg1                         = (simgrid::s4u::Comm*)0;
  simgrid::s4u::Host* arg2                         = (simgrid::s4u::Host*)0;
  boost::shared_ptr<simgrid::s4u::Comm>* smartarg1 = 0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg2 = 0;
  boost::intrusive_ptr<simgrid::s4u::Comm> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Comm>**)&cthis;
  arg1      = (simgrid::s4u::Comm*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg2;
  arg2      = (simgrid::s4u::Host*)(smartarg2 ? smartarg2->get() : 0);

  result = (arg1)->set_source(arg2);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Comm>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Comm>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Comm>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Comm>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1get_1source(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  jlong jresult                                          = 0;
  simgrid::s4u::Comm* arg1                               = (simgrid::s4u::Comm*)0;
  boost::shared_ptr<simgrid::s4u::Comm const>* smartarg1 = 0;
  simgrid::s4u::Host* result                             = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Comm>**)&cthis;
  arg1      = (simgrid::s4u::Comm*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Host*)((simgrid::s4u::Comm const*)arg1)->get_source();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Host>(result, SWIG_intrusive_deleter<simgrid::s4u::Host>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Host>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1set_1destination(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis, jlong jarg2,
                                                                                jobject jarg2_)
{
  jlong jresult                                    = 0;
  simgrid::s4u::Comm* arg1                         = (simgrid::s4u::Comm*)0;
  simgrid::s4u::Host* arg2                         = (simgrid::s4u::Host*)0;
  boost::shared_ptr<simgrid::s4u::Comm>* smartarg1 = 0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg2 = 0;
  boost::intrusive_ptr<simgrid::s4u::Comm> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Comm>**)&cthis;
  arg1      = (simgrid::s4u::Comm*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg2;
  arg2      = (simgrid::s4u::Host*)(smartarg2 ? smartarg2->get() : 0);

  result = (arg1)->set_destination(arg2);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Comm>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Comm>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Comm>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Comm>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1get_1destination(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis)
{
  jlong jresult                                          = 0;
  simgrid::s4u::Comm* arg1                               = (simgrid::s4u::Comm*)0;
  boost::shared_ptr<simgrid::s4u::Comm const>* smartarg1 = 0;
  simgrid::s4u::Host* result                             = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Comm>**)&cthis;
  arg1      = (simgrid::s4u::Comm*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Host*)((simgrid::s4u::Comm const*)arg1)->get_destination();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Host>(result, SWIG_intrusive_deleter<simgrid::s4u::Host>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Host>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1set_1mailbox(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jobject jthis, jlong jarg2, jobject jarg2_)
{
  jlong jresult                                       = 0;
  simgrid::s4u::Comm* arg1                            = (simgrid::s4u::Comm*)0;
  simgrid::s4u::Mailbox* arg2                         = (simgrid::s4u::Mailbox*)0;
  boost::shared_ptr<simgrid::s4u::Comm>* smartarg1    = 0;
  boost::shared_ptr<simgrid::s4u::Mailbox>* smartarg2 = 0;
  boost::intrusive_ptr<simgrid::s4u::Comm> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Comm>**)&cthis;
  arg1      = (simgrid::s4u::Comm*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<simgrid::s4u::Mailbox>**)&jarg2;
  arg2      = (simgrid::s4u::Mailbox*)(smartarg2 ? smartarg2->get() : 0);

  result = (arg1)->set_mailbox(arg2);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Comm>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Comm>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Comm>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Comm>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1get_1mailbox(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jobject jthis)
{
  jlong jresult                                          = 0;
  simgrid::s4u::Comm* arg1                               = (simgrid::s4u::Comm*)0;
  boost::shared_ptr<simgrid::s4u::Comm const>* smartarg1 = 0;
  simgrid::s4u::Mailbox* result                          = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Comm>**)&cthis;
  arg1      = (simgrid::s4u::Comm*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Mailbox*)((simgrid::s4u::Comm const*)arg1)->get_mailbox();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Mailbox>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Mailbox>(result, SWIG_intrusive_deleter<simgrid::s4u::Mailbox>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Mailbox>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Mailbox>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Mailbox>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1set_1src_1data(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis, jobject payload)
{
  auto self = (Comm*)cthis;
  auto glob = jenv->NewGlobalRef(payload);

  self->set_src_data(glob);
}

XBT_PUBLIC jobject JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1get_1payload(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis)
{
  auto self    = (Comm*)cthis;
  jobject glob = (jobject)self->get_payload();
  auto local   = jenv->NewLocalRef(glob);
  jenv->DeleteGlobalRef(glob);

  return local;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1set_1payload_1size(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis, jlong jarg2)
{
  auto self = (Comm*)cthis;
  self->set_payload_size(jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1set_1rate(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis, jdouble jarg2)
{
  auto self = (Comm*)cthis;
  self->set_rate(jarg2);
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1is_1assigned(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis)
{
  auto self = (Comm*)cthis;
  return self->is_assigned();
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1get_1sender(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  jlong jresult                                          = 0;
  simgrid::s4u::Comm* arg1                               = (simgrid::s4u::Comm*)0;
  boost::shared_ptr<simgrid::s4u::Comm const>* smartarg1 = 0;
  simgrid::s4u::Actor* result                            = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Comm>**)&cthis;
  arg1      = (simgrid::s4u::Comm*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Actor*)((simgrid::s4u::Comm const*)arg1)->get_sender();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Actor>(result, SWIG_intrusive_deleter<simgrid::s4u::Actor>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Actor>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1get_1receiver(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis)
{
  jlong jresult                                          = 0;
  simgrid::s4u::Comm* arg1                               = (simgrid::s4u::Comm*)0;
  boost::shared_ptr<simgrid::s4u::Comm const>* smartarg1 = 0;
  simgrid::s4u::Actor* result                            = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Comm>**)&cthis;
  arg1      = (simgrid::s4u::Comm*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Actor*)((simgrid::s4u::Comm const*)arg1)->get_receiver();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Actor>(result, SWIG_intrusive_deleter<simgrid::s4u::Actor>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Actor>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1wait_1for(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis, jdouble jarg2)
{
  ((Comm*)cthis)->wait_for(jarg2);
}
XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1on_1start_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::Comm const&)>* arg1 = *(std::function<void(simgrid::s4u::Comm const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Comm const &) > const & is null");
    return;
  }
  simgrid::s4u::Comm::on_start_cb((std::function<void(simgrid::s4u::Comm const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1on_1this_1start_1cb(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis,
                                                                                  jlong jarg2)
{
  std::function<void(simgrid::s4u::Comm const&)>* arg2 = *(std::function<void(simgrid::s4u::Comm const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Comm const &) > const & is null");
    return;
  }
  ((Comm*)cthis)->on_this_start_cb((std::function<void(simgrid::s4u::Comm const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1on_1completion_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::Comm const&)>* arg1 = *(std::function<void(simgrid::s4u::Comm const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Comm const &) > const & is null");
    return;
  }
  simgrid::s4u::Comm::on_completion_cb((std::function<void(simgrid::s4u::Comm const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1on_1this_1completion_1cb(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis,
                                                                                       jlong jarg2)
{
  std::function<void(simgrid::s4u::Comm const&)>* arg2 = *(std::function<void(simgrid::s4u::Comm const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Comm const &) > const & is null");
    return;
  }
  ((Comm*)cthis)->on_this_completion_cb((std::function<void(simgrid::s4u::Comm const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1on_1this_1suspend_1cb(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis,
                                                                                    jlong jarg2)
{
  std::function<void(simgrid::s4u::Comm const&)>* arg2 = *(std::function<void(simgrid::s4u::Comm const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Comm const &) > const & is null");
    return;
  }
  ((Comm*)cthis)->on_this_suspend_cb((std::function<void(simgrid::s4u::Comm const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1on_1this_1resume_1cb(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis,
                                                                                   jlong jarg2)
{
  std::function<void(simgrid::s4u::Comm const&)>* arg2 = *(std::function<void(simgrid::s4u::Comm const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Comm const &) > const & is null");
    return;
  }
  ((Comm*)cthis)->on_this_resume_cb((std::function<void(simgrid::s4u::Comm const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1on_1veto_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::Comm&)>* arg1 = *(std::function<void(simgrid::s4u::Comm&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Comm &) > const & is null");
    return;
  }
  simgrid::s4u::Comm::on_veto_cb((std::function<void(simgrid::s4u::Comm&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1on_1this_1veto_1cb(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis, jlong jarg2)
{
  simgrid::s4u::Activity_T<simgrid::s4u::Comm>* arg1 = (simgrid::s4u::Activity_T<simgrid::s4u::Comm>*)0;
  std::function<void(simgrid::s4u::Comm&)>* arg2     = 0;
  boost::shared_ptr<simgrid::s4u::Activity_T<simgrid::s4u::Comm>>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Activity_T<simgrid::s4u::Comm>>**)&cthis;
  arg1      = (simgrid::s4u::Activity_T<simgrid::s4u::Comm>*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::Comm&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Comm &) > const & is null");
    return;
  }
  (arg1)->on_this_veto_cb((std::function<void(simgrid::s4u::Comm&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1add_1successor(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis, jlong jarg2, jobject jarg2_)
{
  auto self = (Comm*)cthis;
  self->add_successor((Activity*)jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1remove_1successor(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis, jlong jarg2,
                                                                                jobject jarg2_)
{
  auto self = (Comm*)cthis;
  self->remove_successor((Activity*)jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1set_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis, jstring jarg2)
{
  auto self = (Comm*)cthis;
  self->set_name(java_string_to_std_string(jenv, jarg2));
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1get_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  jstring jresult                                    = 0;
  simgrid::s4u::Activity_T<simgrid::s4u::Comm>* arg1 = (simgrid::s4u::Activity_T<simgrid::s4u::Comm>*)0;
  boost::shared_ptr<simgrid::s4u::Activity_T<simgrid::s4u::Comm> const>* smartarg1 = 0;
  char* result                                                                     = 0;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity_T<simgrid::s4u::Comm>>**)&cthis;
  arg1      = (simgrid::s4u::Activity_T<simgrid::s4u::Comm>*)(smartarg1 ? smartarg1->get() : 0);

  result = (char*)((simgrid::s4u::Activity_T<simgrid::s4u::Comm> const*)arg1)->get_cname();
  if (result)
    jresult = jenv->NewStringUTF((const char*)result);
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1set_1tracing_1category(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis,
                                                                                     jstring jarg2)
{
  auto self = (Comm*)cthis;
  self->set_tracing_category(java_string_to_std_string(jenv, jarg2));
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1get_1tracing_1category(JNIEnv* jenv, jclass jcls,
                                                                                        jlong cthis, jobject jthis)
{
  jstring jresult                                    = 0;
  simgrid::s4u::Activity_T<simgrid::s4u::Comm>* arg1 = (simgrid::s4u::Activity_T<simgrid::s4u::Comm>*)0;
  boost::shared_ptr<simgrid::s4u::Activity_T<simgrid::s4u::Comm> const>* smartarg1 = 0;
  std::string* result                                                              = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity_T<simgrid::s4u::Comm>>**)&cthis;
  arg1      = (simgrid::s4u::Activity_T<simgrid::s4u::Comm>*)(smartarg1 ? smartarg1->get() : 0);

  result  = (std::string*)&((simgrid::s4u::Activity_T<simgrid::s4u::Comm> const*)arg1)->get_tracing_category();
  jresult = jenv->NewStringUTF(result->c_str());
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1get_1clean_1function(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis)
{
  jlong jresult                                      = 0;
  simgrid::s4u::Activity_T<simgrid::s4u::Comm>* arg1 = (simgrid::s4u::Activity_T<simgrid::s4u::Comm>*)0;
  boost::shared_ptr<simgrid::s4u::Activity_T<simgrid::s4u::Comm> const>* smartarg1 = 0;
  std::function<void(void*)>* result                                               = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Activity_T<simgrid::s4u::Comm>>**)&cthis;
  arg1      = (simgrid::s4u::Activity_T<simgrid::s4u::Comm>*)(smartarg1 ? smartarg1->get() : 0);

  result =
      (std::function<void(void*)>*)&((simgrid::s4u::Activity_T<simgrid::s4u::Comm> const*)arg1)->get_clean_function();
  *(std::function<void(void*)>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1detach_1_1SWIG_10(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis)
{
  auto* self = (Comm*)cthis;
  self->detach();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1detach_1_1SWIG_11(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis, jlong jarg2)
{
  auto* self                     = (Comm*)cthis;
  std::function<void(void*)> fun = *(std::function<void(void*)>*)jarg2;
  self->detach(fun);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Comm_1cancel(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                     jobject jthis)
{
  ((Comm*)cthis)->cancel();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1ConditionVariable(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis)
{
  simgrid::s4u::ConditionVariable* arg1                         = (simgrid::s4u::ConditionVariable*)0;
  boost::shared_ptr<simgrid::s4u::ConditionVariable>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::ConditionVariable>**)&cthis;
  arg1      = (simgrid::s4u::ConditionVariable*)(smartarg1 ? smartarg1->get() : 0);

  (void)arg1;
  delete smartarg1;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_ConditionVariable_1create(JNIEnv* jenv, jclass jcls)
{
  jlong jresult = 0;
  boost::intrusive_ptr<simgrid::s4u::ConditionVariable> result;

  (void)jenv;
  (void)jcls;
  result = simgrid::s4u::ConditionVariable::create();

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::ConditionVariable>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::ConditionVariable>(
            result.get(), SWIG_intrusive_deleter<simgrid::s4u::ConditionVariable>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::ConditionVariable>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_ConditionVariable_1wait_1until(JNIEnv* jenv, jclass jcls,
                                                                                        jlong cthis, jobject jthis,
                                                                                        jlong jarg2, jdouble jarg3)
{
  jlong jresult                               = 0;
  simgrid::s4u::ConditionVariable* arg1       = (simgrid::s4u::ConditionVariable*)0;
  std::unique_lock<simgrid::s4u::Mutex>* arg2 = 0;
  double arg3;
  boost::shared_ptr<simgrid::s4u::ConditionVariable>* smartarg1 = 0;
  std::cv_status result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::ConditionVariable>**)&cthis;
  arg1      = (simgrid::s4u::ConditionVariable*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::unique_lock<simgrid::s4u::Mutex>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::unique_lock< simgrid::s4u::Mutex > const & is null");
    return 0;
  }
  arg3                        = (double)jarg3;
  result                      = (arg1)->wait_until((std::unique_lock<simgrid::s4u::Mutex> const&)*arg2, arg3);
  *(std::cv_status**)&jresult = new std::cv_status(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_ConditionVariable_1wait_1for(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis,
                                                                                      jlong jarg2, jdouble jarg3)
{
  jlong jresult                               = 0;
  simgrid::s4u::ConditionVariable* arg1       = (simgrid::s4u::ConditionVariable*)0;
  std::unique_lock<simgrid::s4u::Mutex>* arg2 = 0;
  double arg3;
  boost::shared_ptr<simgrid::s4u::ConditionVariable>* smartarg1 = 0;
  std::cv_status result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::ConditionVariable>**)&cthis;
  arg1      = (simgrid::s4u::ConditionVariable*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::unique_lock<simgrid::s4u::Mutex>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::unique_lock< simgrid::s4u::Mutex > const & is null");
    return 0;
  }
  arg3                        = (double)jarg3;
  result                      = (arg1)->wait_for((std::unique_lock<simgrid::s4u::Mutex> const&)*arg2, arg3);
  *(std::cv_status**)&jresult = new std::cv_status(result);
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ConditionVariable_1notify_1one(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis)
{
  simgrid::s4u::ConditionVariable* arg1                         = (simgrid::s4u::ConditionVariable*)0;
  boost::shared_ptr<simgrid::s4u::ConditionVariable>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::ConditionVariable>**)&cthis;
  arg1      = (simgrid::s4u::ConditionVariable*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->notify_one();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ConditionVariable_1notify_1all(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis)
{
  simgrid::s4u::ConditionVariable* arg1                         = (simgrid::s4u::ConditionVariable*)0;
  boost::shared_ptr<simgrid::s4u::ConditionVariable>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::ConditionVariable>**)&cthis;
  arg1      = (simgrid::s4u::ConditionVariable*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->notify_all();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1Disk(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  simgrid::s4u::Disk* arg1                         = (simgrid::s4u::Disk*)0;
  boost::shared_ptr<simgrid::s4u::Disk>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  (void)arg1;
  delete smartarg1;
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1get_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  jstring jresult                                        = 0;
  simgrid::s4u::Disk* arg1                               = (simgrid::s4u::Disk*)0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg1 = 0;
  char* result                                           = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  result = (char*)((simgrid::s4u::Disk const*)arg1)->get_cname();
  if (result)
    jresult = jenv->NewStringUTF((const char*)result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1set_1read_1bandwidth(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis,
                                                                                    jdouble jarg2)
{
  jlong jresult            = 0;
  simgrid::s4u::Disk* arg1 = (simgrid::s4u::Disk*)0;
  double arg2;
  boost::shared_ptr<simgrid::s4u::Disk>* smartarg1 = 0;
  simgrid::s4u::Disk* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = (double)jarg2;
  result = (simgrid::s4u::Disk*)(arg1)->set_read_bandwidth(arg2);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Disk>(result, SWIG_intrusive_deleter<simgrid::s4u::Disk>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Disk>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1get_1read_1bandwidth(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis)
{
  jdouble jresult                                        = 0;
  simgrid::s4u::Disk* arg1                               = (simgrid::s4u::Disk*)0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg1 = 0;
  double result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  result  = (double)((simgrid::s4u::Disk const*)arg1)->get_read_bandwidth();
  jresult = (jdouble)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1set_1write_1bandwidth(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis,
                                                                                     jdouble jarg2)
{
  jlong jresult            = 0;
  simgrid::s4u::Disk* arg1 = (simgrid::s4u::Disk*)0;
  double arg2;
  boost::shared_ptr<simgrid::s4u::Disk>* smartarg1 = 0;
  simgrid::s4u::Disk* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = (double)jarg2;
  result = (simgrid::s4u::Disk*)(arg1)->set_write_bandwidth(arg2);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Disk>(result, SWIG_intrusive_deleter<simgrid::s4u::Disk>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Disk>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1get_1write_1bandwidth(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis)
{
  jdouble jresult                                        = 0;
  simgrid::s4u::Disk* arg1                               = (simgrid::s4u::Disk*)0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg1 = 0;
  double result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  result  = (double)((simgrid::s4u::Disk const*)arg1)->get_write_bandwidth();
  jresult = (jdouble)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1set_1readwrite_1bandwidth(JNIEnv* jenv, jclass jcls,
                                                                                         jlong cthis, jobject jthis,
                                                                                         jdouble jarg2)
{
  jlong jresult            = 0;
  simgrid::s4u::Disk* arg1 = (simgrid::s4u::Disk*)0;
  double arg2;
  boost::shared_ptr<simgrid::s4u::Disk>* smartarg1 = 0;
  simgrid::s4u::Disk* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = (double)jarg2;
  result = (simgrid::s4u::Disk*)(arg1)->set_readwrite_bandwidth(arg2);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Disk>(result, SWIG_intrusive_deleter<simgrid::s4u::Disk>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Disk>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1get_1property(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jstring jarg2)
{
  jstring jresult                                        = 0;
  simgrid::s4u::Disk* arg1                               = (simgrid::s4u::Disk*)0;
  std::string* arg2                                      = 0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg1 = 0;
  char* result                                           = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = (char*)((simgrid::s4u::Disk const*)arg1)->get_property((std::string const&)*arg2);
  if (result)
    jresult = jenv->NewStringUTF((const char*)result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1set_1property(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis, jstring jarg2,
                                                                             jstring jarg3)
{
  jlong jresult                                    = 0;
  simgrid::s4u::Disk* arg1                         = (simgrid::s4u::Disk*)0;
  std::string* arg2                                = 0;
  std::string* arg3                                = 0;
  boost::shared_ptr<simgrid::s4u::Disk>* smartarg1 = 0;
  simgrid::s4u::Disk* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  if (!jarg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg3_pstr = (const char*)jenv->GetStringUTFChars(jarg3, 0);
  if (!arg3_pstr)
    return 0;
  std::string arg3_str(arg3_pstr);
  arg3 = &arg3_str;
  jenv->ReleaseStringUTFChars(jarg3, arg3_pstr);
  result = (simgrid::s4u::Disk*)(arg1)->set_property((std::string const&)*arg2, (std::string const&)*arg3);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Disk>(result, SWIG_intrusive_deleter<simgrid::s4u::Disk>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Disk>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1set_1properties(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jlong jarg2)
{
  jlong jresult                                      = 0;
  simgrid::s4u::Disk* arg1                           = (simgrid::s4u::Disk*)0;
  std::unordered_map<std::string, std::string>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Disk>* smartarg1   = 0;
  simgrid::s4u::Disk* result                         = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::unordered_map<std::string, std::string>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::unordered_map< std::string,std::string > const & is null");
    return 0;
  }
  result = (simgrid::s4u::Disk*)(arg1)->set_properties((std::unordered_map<std::string, std::string> const&)*arg2);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Disk>(result, SWIG_intrusive_deleter<simgrid::s4u::Disk>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Disk>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1set_1host(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                         jobject jthis, jlong jarg2, jobject jarg2_)
{
  jlong jresult                                    = 0;
  simgrid::s4u::Disk* arg1                         = (simgrid::s4u::Disk*)0;
  simgrid::s4u::Host* arg2                         = (simgrid::s4u::Host*)0;
  boost::shared_ptr<simgrid::s4u::Disk>* smartarg1 = 0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg2 = 0;
  simgrid::s4u::Disk* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg2;
  arg2      = (simgrid::s4u::Host*)(smartarg2 ? smartarg2->get() : 0);

  result = (simgrid::s4u::Disk*)(arg1)->set_host(arg2);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Disk>(result, SWIG_intrusive_deleter<simgrid::s4u::Disk>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Disk>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1get_1host(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                         jobject jthis)
{
  jlong jresult                                          = 0;
  simgrid::s4u::Disk* arg1                               = (simgrid::s4u::Disk*)0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg1 = 0;
  simgrid::s4u::Host* result                             = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Host*)((simgrid::s4u::Disk const*)arg1)->get_host();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Host>(result, SWIG_intrusive_deleter<simgrid::s4u::Host>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Host>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1set_1concurrency_1limit(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis,
                                                                                       jint jarg2)
{
  jlong jresult            = 0;
  simgrid::s4u::Disk* arg1 = (simgrid::s4u::Disk*)0;
  int arg2;
  boost::shared_ptr<simgrid::s4u::Disk>* smartarg1 = 0;
  simgrid::s4u::Disk* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = (int)jarg2;
  result = (simgrid::s4u::Disk*)(arg1)->set_concurrency_limit(arg2);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Disk>(result, SWIG_intrusive_deleter<simgrid::s4u::Disk>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Disk>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1get_1concurrency_1limit(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis)
{
  jint jresult                                           = 0;
  simgrid::s4u::Disk* arg1                               = (simgrid::s4u::Disk*)0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg1 = 0;
  int result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  result  = (int)((simgrid::s4u::Disk const*)arg1)->get_concurrency_limit();
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1io_1init(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis, jint jarg2, jint jarg3)
{
  jlong jresult            = 0;
  simgrid::s4u::Disk* arg1 = (simgrid::s4u::Disk*)0;
  sg_size_t arg2;
  simgrid::s4u::Io::OpType arg3;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg1 = 0;
  boost::intrusive_ptr<simgrid::s4u::Io> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = (sg_size_t)jarg2;
  arg3   = (simgrid::s4u::Io::OpType)jarg3;
  result = ((simgrid::s4u::Disk const*)arg1)->io_init(arg2, arg3);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Io>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Io>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1read_1async(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis, jint jarg2)
{
  jlong jresult            = 0;
  simgrid::s4u::Disk* arg1 = (simgrid::s4u::Disk*)0;
  sg_size_t arg2;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg1 = 0;
  boost::intrusive_ptr<simgrid::s4u::Io> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = (sg_size_t)jarg2;
  result = ((simgrid::s4u::Disk const*)arg1)->read_async(arg2);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Io>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Io>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1read_1_1SWIG_10(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis, jint jarg2)
{
  jint jresult             = 0;
  simgrid::s4u::Disk* arg1 = (simgrid::s4u::Disk*)0;
  sg_size_t arg2;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg1 = 0;
  sg_size_t result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2    = (sg_size_t)jarg2;
  result  = ((simgrid::s4u::Disk const*)arg1)->read(arg2);
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1read_1_1SWIG_11(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis, jint jarg2, jdouble jarg3)
{
  jint jresult             = 0;
  simgrid::s4u::Disk* arg1 = (simgrid::s4u::Disk*)0;
  sg_size_t arg2;
  double arg3;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg1 = 0;
  sg_size_t result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2    = (sg_size_t)jarg2;
  arg3    = (double)jarg3;
  result  = ((simgrid::s4u::Disk const*)arg1)->read(arg2, arg3);
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1write_1async(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jobject jthis, jint jarg2)
{
  jlong jresult            = 0;
  simgrid::s4u::Disk* arg1 = (simgrid::s4u::Disk*)0;
  sg_size_t arg2;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg1 = 0;
  boost::intrusive_ptr<simgrid::s4u::Io> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = (sg_size_t)jarg2;
  result = ((simgrid::s4u::Disk const*)arg1)->write_async(arg2);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Io>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Io>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1write_1_1SWIG_10(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jint jarg2)
{
  jint jresult             = 0;
  simgrid::s4u::Disk* arg1 = (simgrid::s4u::Disk*)0;
  sg_size_t arg2;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg1 = 0;
  sg_size_t result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2    = (sg_size_t)jarg2;
  result  = ((simgrid::s4u::Disk const*)arg1)->write(arg2);
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1write_1_1SWIG_11(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jint jarg2, jdouble jarg3)
{
  jint jresult             = 0;
  simgrid::s4u::Disk* arg1 = (simgrid::s4u::Disk*)0;
  sg_size_t arg2;
  double arg3;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg1 = 0;
  sg_size_t result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2    = (sg_size_t)jarg2;
  arg3    = (double)jarg3;
  result  = ((simgrid::s4u::Disk const*)arg1)->write(arg2, arg3);
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1SharingPolicy_1NONLINEAR_1get(JNIEnv* jenv, jclass jcls)
{
  jint jresult = 0;
  simgrid::s4u::Disk::SharingPolicy result;

  (void)jenv;
  (void)jcls;
  result  = (simgrid::s4u::Disk::SharingPolicy)simgrid::s4u::Disk::SharingPolicy::NONLINEAR;
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1SharingPolicy_1LINEAR_1get(JNIEnv* jenv, jclass jcls)
{
  jint jresult = 0;
  simgrid::s4u::Disk::SharingPolicy result;

  (void)jenv;
  (void)jcls;
  result  = (simgrid::s4u::Disk::SharingPolicy)simgrid::s4u::Disk::SharingPolicy::LINEAR;
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1Operation_1READ_1get(JNIEnv* jenv, jclass jcls)
{
  jint jresult = 0;
  simgrid::s4u::Disk::Operation result;

  (void)jenv;
  (void)jcls;
  result  = (simgrid::s4u::Disk::Operation)simgrid::s4u::Disk::Operation::READ;
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1Operation_1WRITE_1get(JNIEnv* jenv, jclass jcls)
{
  jint jresult = 0;
  simgrid::s4u::Disk::Operation result;

  (void)jenv;
  (void)jcls;
  result  = (simgrid::s4u::Disk::Operation)simgrid::s4u::Disk::Operation::WRITE;
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1Operation_1READWRITE_1get(JNIEnv* jenv, jclass jcls)
{
  jint jresult = 0;
  simgrid::s4u::Disk::Operation result;

  (void)jenv;
  (void)jcls;
  result  = (simgrid::s4u::Disk::Operation)simgrid::s4u::Disk::Operation::READWRITE;
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1get_1sharing_1policy(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis,
                                                                                   jint jarg2)
{
  jint jresult             = 0;
  simgrid::s4u::Disk* arg1 = (simgrid::s4u::Disk*)0;
  simgrid::s4u::Disk::Operation arg2;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg1 = 0;
  simgrid::s4u::Disk::SharingPolicy result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2    = (simgrid::s4u::Disk::Operation)jarg2;
  result  = (simgrid::s4u::Disk::SharingPolicy)((simgrid::s4u::Disk const*)arg1)->get_sharing_policy(arg2);
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1set_1factor_1cb(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jlong jarg2)
{
  jlong jresult                                       = 0;
  simgrid::s4u::Disk* arg1                            = (simgrid::s4u::Disk*)0;
  std::function<simgrid::s4u::Disk::IoFactorCb>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Disk>* smartarg1    = 0;
  simgrid::s4u::Disk* result                          = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<simgrid::s4u::Disk::IoFactorCb>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< simgrid::s4u::Disk::IoFactorCb > const & is null");
    return 0;
  }
  result = (simgrid::s4u::Disk*)(arg1)->set_factor_cb((std::function<simgrid::s4u::Disk::IoFactorCb> const&)*arg2);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Disk>(result, SWIG_intrusive_deleter<simgrid::s4u::Disk>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Disk>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1seal(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                    jobject jthis)
{
  jlong jresult                                    = 0;
  simgrid::s4u::Disk* arg1                         = (simgrid::s4u::Disk*)0;
  boost::shared_ptr<simgrid::s4u::Disk>* smartarg1 = 0;
  simgrid::s4u::Disk* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Disk*)(arg1)->seal();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Disk>(result, SWIG_intrusive_deleter<simgrid::s4u::Disk>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Disk>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Disk>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1on_1onoff_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::Disk const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::Disk const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Disk const &) > const & is null");
    return;
  }
  simgrid::s4u::Disk::on_onoff_cb((std::function<void(simgrid::s4u::Disk const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1on_1this_1onoff_1cb(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis,
                                                                                  jlong jarg2)
{
  simgrid::s4u::Disk* arg1                             = (simgrid::s4u::Disk*)0;
  std::function<void(simgrid::s4u::Disk const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Disk>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::Disk const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Disk const &) > const & is null");
    return;
  }
  (arg1)->on_this_onoff_cb((std::function<void(simgrid::s4u::Disk const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1on_1read_1bandwidth_1change_1cb(JNIEnv* jenv, jclass jcls,
                                                                                              jlong cthis)
{
  std::function<void(simgrid::s4u::Disk const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::Disk const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Disk const &) > const & is null");
    return;
  }
  simgrid::s4u::Disk::on_read_bandwidth_change_cb((std::function<void(simgrid::s4u::Disk const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1on_1this_1read_1bandwidth_1change_1cb(
    JNIEnv* jenv, jclass jcls, jlong cthis, jobject jthis, jlong jarg2)
{
  simgrid::s4u::Disk* arg1                             = (simgrid::s4u::Disk*)0;
  std::function<void(simgrid::s4u::Disk const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Disk>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::Disk const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Disk const &) > const & is null");
    return;
  }
  (arg1)->on_this_read_bandwidth_change_cb((std::function<void(simgrid::s4u::Disk const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1on_1write_1bandwidth_1change_1cb(JNIEnv* jenv,
                                                                                               jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::Disk const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::Disk const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Disk const &) > const & is null");
    return;
  }
  simgrid::s4u::Disk::on_write_bandwidth_change_cb((std::function<void(simgrid::s4u::Disk const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1on_1this_1write_1bandwidth_1change_1cb(
    JNIEnv* jenv, jclass jcls, jlong cthis, jobject jthis, jlong jarg2)
{
  simgrid::s4u::Disk* arg1                             = (simgrid::s4u::Disk*)0;
  std::function<void(simgrid::s4u::Disk const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Disk>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::Disk const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Disk const &) > const & is null");
    return;
  }
  (arg1)->on_this_write_bandwidth_change_cb((std::function<void(simgrid::s4u::Disk const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1on_1destruction_1cb(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis)
{
  std::function<void(simgrid::s4u::Disk const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::Disk const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Disk const &) > const & is null");
    return;
  }
  simgrid::s4u::Disk::on_destruction_cb((std::function<void(simgrid::s4u::Disk const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Disk_1on_1this_1destruction_1cb(JNIEnv* jenv, jclass jcls,
                                                                                        jlong cthis, jobject jthis,
                                                                                        jlong jarg2)
{
  simgrid::s4u::Disk* arg1                             = (simgrid::s4u::Disk*)0;
  std::function<void(simgrid::s4u::Disk const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::Disk>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Disk>**)&cthis;
  arg1      = (simgrid::s4u::Disk*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::Disk const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Disk const &) > const & is null");
    return;
  }
  (arg1)->on_this_destruction_cb((std::function<void(simgrid::s4u::Disk const&)> const&)*arg2);
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_new_1Engine_1_1SWIG_10(JNIEnv* jenv, jclass jcls,
                                                                                jstring cthis)
{
  jlong jresult = 0;
  std::string arg1;
  simgrid::s4u::Engine* result = 0;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  (&arg1)->assign(arg1_pstr);
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  result                            = (simgrid::s4u::Engine*)new simgrid::s4u::Engine(arg1);
  *(simgrid::s4u::Engine**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_new_1Engine_1_1SWIG_11(JNIEnv* jenv, jclass jcls,
                                                                                jobjectArray cthis)
{
  jlong jresult                = 0;
  int* arg1                    = (int*)0;
  char** arg2                  = (char**)0;
  simgrid::s4u::Engine* result = 0;

  (void)jenv;
  (void)jcls;
  {
    if (cthis == (jobjectArray)0) {
      SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
      return 0;
    }
    int len = (int)jenv->GetArrayLength(cthis);
    if (len < 0) {
      SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, "array length negative");
      return 0;
    }
    arg2 = (char**)malloc((len + 1) * sizeof(char*));
    if (arg2 == NULL) {
      SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "memory allocation failed");
      return 0;
    }
    arg1 = &len;
    jsize i;
    for (i = 0; i < len; i++) {
      jstring j_string     = (jstring)jenv->GetObjectArrayElement(cthis, i);
      const char* c_string = jenv->GetStringUTFChars(j_string, 0);
      arg2[i]              = (char*)c_string;
    }
    arg2[i] = NULL;
  }
  result                            = (simgrid::s4u::Engine*)new simgrid::s4u::Engine(arg1, arg2);
  *(simgrid::s4u::Engine**)&jresult = result;
  {
    free((void*)arg2);
  }
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1run(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                    jobject jthis)
{
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  ((simgrid::s4u::Engine const*)arg1)->run();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1run_1until(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis, jdouble jarg2)
{
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  double arg2;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  arg2 = (double)jarg2;
  ((simgrid::s4u::Engine const*)arg1)->run_until(arg2);
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1clock(JNIEnv* jenv, jclass jcls)
{
  jdouble jresult = 0;
  double result;

  (void)jenv;
  (void)jcls;
  result  = (double)simgrid::s4u::Engine::get_clock();
  jresult = (jdouble)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1papi_1start(JNIEnv* jenv, jclass jcls)
{
  (void)jenv;
  (void)jcls;
  simgrid::s4u::Engine::papi_start();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1papi_1stop(JNIEnv* jenv, jclass jcls)
{
  (void)jenv;
  (void)jcls;
  simgrid::s4u::Engine::papi_stop();
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1papi_1get_1num_1counters(JNIEnv* jenv, jclass jcls)
{
  jint jresult = 0;
  int result;

  (void)jenv;
  (void)jcls;
  result  = (int)simgrid::s4u::Engine::papi_get_num_counters();
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1papi_1counters(JNIEnv* jenv, jclass jcls)
{
  jlong jresult = 0;
  SwigValueWrapper<std::vector<long long>> result;

  (void)jenv;
  (void)jcls;
  result                              = simgrid::s4u::Engine::get_papi_counters();
  *(std::vector<long long>**)&jresult = new std::vector<long long>(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1instance_1_1SWIG_10(JNIEnv* jenv, jclass jcls)
{
  jlong jresult                = 0;
  simgrid::s4u::Engine* result = 0;

  (void)jenv;
  (void)jcls;
  result                            = (simgrid::s4u::Engine*)simgrid::s4u::Engine::get_instance();
  *(simgrid::s4u::Engine**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1instance_1_1SWIG_11(JNIEnv* jenv, jclass jcls,
                                                                                          jobjectArray cthis)
{
  jlong jresult                = 0;
  int* arg1                    = (int*)0;
  char** arg2                  = (char**)0;
  simgrid::s4u::Engine* result = 0;

  (void)jenv;
  (void)jcls;
  {
    if (cthis == (jobjectArray)0) {
      SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null array");
      return 0;
    }
    int len = (int)jenv->GetArrayLength(cthis);
    if (len < 0) {
      SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, "array length negative");
      return 0;
    }
    arg2 = (char**)malloc((len + 1) * sizeof(char*));
    if (arg2 == NULL) {
      SWIG_JavaThrowException(jenv, SWIG_JavaOutOfMemoryError, "memory allocation failed");
      return 0;
    }
    arg1 = &len;
    jsize i;
    for (i = 0; i < len; i++) {
      jstring j_string     = (jstring)jenv->GetObjectArrayElement(cthis, i);
      const char* c_string = jenv->GetStringUTFChars(j_string, 0);
      arg2[i]              = (char*)c_string;
    }
    arg2[i] = NULL;
  }
  result                            = (simgrid::s4u::Engine*)simgrid::s4u::Engine::get_instance(arg1, arg2);
  *(simgrid::s4u::Engine**)&jresult = result;
  {
    free((void*)arg2);
  }
  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1has_1instance(JNIEnv* jenv, jclass jcls)
{
  jboolean jresult = 0;
  bool result;

  (void)jenv;
  (void)jcls;
  result  = (bool)simgrid::s4u::Engine::has_instance();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1cmdline(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis)
{
  jlong jresult                    = 0;
  simgrid::s4u::Engine* arg1       = (simgrid::s4u::Engine*)0;
  std::vector<std::string>* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                                  = *(simgrid::s4u::Engine**)&cthis;
  result = (std::vector<std::string>*)&((simgrid::s4u::Engine const*)arg1)->get_cmdline();
  *(std::vector<std::string>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1context_1factory_1name(JNIEnv* jenv,
                                                                                               jclass jcls, jlong cthis,
                                                                                               jobject jthis)
{
  jstring jresult            = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  char* result               = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1   = *(simgrid::s4u::Engine**)&cthis;
  result = (char*)((simgrid::s4u::Engine const*)arg1)->get_context_factory_name();
  if (result)
    jresult = jenv->NewStringUTF((const char*)result);
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1load_1platform(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jstring jarg2)
{
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  std::string* arg2          = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  ((simgrid::s4u::Engine const*)arg1)->load_platform((std::string const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1seal_1platform(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis)
{
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  ((simgrid::s4u::Engine const*)arg1)->seal_platform();
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1flatify_1platform(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis)
{
  jstring jresult            = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  std::string result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::Engine**)&cthis;
  result  = ((simgrid::s4u::Engine const*)arg1)->flatify_platform();
  jresult = jenv->NewStringUTF((&result)->c_str());
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1register_1function_1_1SWIG_10(
    JNIEnv* jenv, jclass jcls, jlong cthis, jobject jthis, jstring jarg2, jlong jarg3)
{
  simgrid::s4u::Engine* arg1             = (simgrid::s4u::Engine*)0;
  std::string* arg2                      = 0;
  std::function<void(int, char**)>* arg3 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  arg3 = *(std::function<void(int, char**)>**)&jarg3;
  if (!arg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::function< void (int,char **) > const & is null");
    return;
  }
  (arg1)->register_function((std::string const&)*arg2, (std::function<void(int, char**)> const&)*arg3);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1register_1function_1_1SWIG_11(
    JNIEnv* jenv, jclass jcls, jlong cthis, jobject jthis, jstring jarg2, jlong jarg3)
{
  simgrid::s4u::Engine* arg1                          = (simgrid::s4u::Engine*)0;
  std::string* arg2                                   = 0;
  std::function<void(std::vector<std::string>)>* arg3 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  arg3 = *(std::function<void(std::vector<std::string>)>**)&jarg3;
  if (!arg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (std::vector< std::string >) > const & is null");
    return;
  }
  (arg1)->register_function((std::string const&)*arg2, (std::function<void(std::vector<std::string>)> const&)*arg3);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1register_1default(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis,
                                                                                  jlong jarg2)
{
  simgrid::s4u::Engine* arg1             = (simgrid::s4u::Engine*)0;
  std::function<void(int, char**)>* arg2 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  arg2 = *(std::function<void(int, char**)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::function< void (int,char **) > const & is null");
    return;
  }
  (arg1)->register_default((std::function<void(int, char**)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1track_1vetoed_1activities(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jobject jthis,
                                                                                          jlong jarg2)
{
  simgrid::s4u::Engine* arg1              = (simgrid::s4u::Engine*)0;
  std::set<simgrid::s4u::Activity*>* arg2 = (std::set<simgrid::s4u::Activity*>*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  arg2 = *(std::set<simgrid::s4u::Activity*>**)&jarg2;
  ((simgrid::s4u::Engine const*)arg1)->track_vetoed_activities(arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1load_1deployment(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis, jstring jarg2)
{
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  std::string* arg2          = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  ((simgrid::s4u::Engine const*)arg1)->load_deployment((std::string const&)*arg2);
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1host_1count(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis)
{
  jlong jresult              = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  size_t result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::Engine**)&cthis;
  result  = ((simgrid::s4u::Engine const*)arg1)->get_host_count();
  jresult = (jlong)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1all_1hosts(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis)
{
  jlong jresult              = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  SwigValueWrapper<std::vector<simgrid::s4u::Host*>> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                                          = *(simgrid::s4u::Engine**)&cthis;
  result                                        = ((simgrid::s4u::Engine const*)arg1)->get_all_hosts();
  *(std::vector<simgrid::s4u::Host*>**)&jresult = new std::vector<simgrid::s4u::Host*>(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1filtered_1hosts(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis,
                                                                                      jlong jarg2)
{
  jlong jresult                                  = 0;
  simgrid::s4u::Engine* arg1                     = (simgrid::s4u::Engine*)0;
  std::function<bool(simgrid::s4u::Host*)>* arg2 = 0;
  SwigValueWrapper<std::vector<simgrid::s4u::Host*>> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  arg2 = *(std::function<bool(simgrid::s4u::Host*)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< bool (simgrid::s4u::Host *) > const & is null");
    return 0;
  }
  result =
      ((simgrid::s4u::Engine const*)arg1)->get_filtered_hosts((std::function<bool(simgrid::s4u::Host*)> const&)*arg2);
  *(std::vector<simgrid::s4u::Host*>**)&jresult = new std::vector<simgrid::s4u::Host*>(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1hosts_1from_1MPI_1hostfile(
    JNIEnv* jenv, jclass jcls, jlong cthis, jobject jthis, jstring jarg2)
{
  jlong jresult              = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  std::string* arg2          = 0;
  SwigValueWrapper<std::vector<simgrid::s4u::Host*>> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = ((simgrid::s4u::Engine const*)arg1)->get_hosts_from_MPI_hostfile((std::string const&)*arg2);
  *(std::vector<simgrid::s4u::Host*>**)&jresult = new std::vector<simgrid::s4u::Host*>(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1host_1by_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis, jstring jarg2)
{
  jlong jresult              = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  std::string* arg2          = 0;
  simgrid::s4u::Host* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = (simgrid::s4u::Host*)((simgrid::s4u::Engine const*)arg1)->host_by_name((std::string const&)*arg2);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Host>(result, SWIG_intrusive_deleter<simgrid::s4u::Host>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Host>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1host_1by_1name_1or_1null(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jobject jthis,
                                                                                          jstring jarg2)
{
  jlong jresult              = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  std::string* arg2          = 0;
  simgrid::s4u::Host* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = (simgrid::s4u::Host*)((simgrid::s4u::Engine const*)arg1)->host_by_name_or_null((std::string const&)*arg2);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Host>(result, SWIG_intrusive_deleter<simgrid::s4u::Host>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Host>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1link_1count(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis)
{
  jlong jresult              = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  size_t result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::Engine**)&cthis;
  result  = ((simgrid::s4u::Engine const*)arg1)->get_link_count();
  jresult = (jlong)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1all_1links(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis)
{
  jlong jresult              = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  SwigValueWrapper<std::vector<simgrid::s4u::Link*>> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                                          = *(simgrid::s4u::Engine**)&cthis;
  result                                        = ((simgrid::s4u::Engine const*)arg1)->get_all_links();
  *(std::vector<simgrid::s4u::Link*>**)&jresult = new std::vector<simgrid::s4u::Link*>(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1filtered_1links(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis,
                                                                                      jlong jarg2)
{
  jlong jresult                                  = 0;
  simgrid::s4u::Engine* arg1                     = (simgrid::s4u::Engine*)0;
  std::function<bool(simgrid::s4u::Link*)>* arg2 = 0;
  SwigValueWrapper<std::vector<simgrid::s4u::Link*>> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  arg2 = *(std::function<bool(simgrid::s4u::Link*)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< bool (simgrid::s4u::Link *) > const & is null");
    return 0;
  }
  result =
      ((simgrid::s4u::Engine const*)arg1)->get_filtered_links((std::function<bool(simgrid::s4u::Link*)> const&)*arg2);
  *(std::vector<simgrid::s4u::Link*>**)&jresult = new std::vector<simgrid::s4u::Link*>(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1link_1by_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis, jstring jarg2)
{
  jlong jresult              = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  std::string* arg2          = 0;
  simgrid::s4u::Link* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = (simgrid::s4u::Link*)((simgrid::s4u::Engine const*)arg1)->link_by_name((std::string const&)*arg2);
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1link_1by_1name_1or_1null(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jobject jthis,
                                                                                          jstring jarg2)
{
  jlong jresult              = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  std::string* arg2          = 0;
  simgrid::s4u::Link* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = (simgrid::s4u::Link*)((simgrid::s4u::Engine const*)arg1)->link_by_name_or_null((std::string const&)*arg2);
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1mailbox_1by_1name_1or_1create(JNIEnv* jenv,
                                                                                               jclass jcls, jlong cthis,
                                                                                               jobject jthis,
                                                                                               jstring jarg2)
{
  jlong jresult                 = 0;
  simgrid::s4u::Engine* arg1    = (simgrid::s4u::Engine*)0;
  std::string* arg2             = 0;
  simgrid::s4u::Mailbox* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result =
      (simgrid::s4u::Mailbox*)((simgrid::s4u::Engine const*)arg1)->mailbox_by_name_or_create((std::string const&)*arg2);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Mailbox>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Mailbox>(result, SWIG_intrusive_deleter<simgrid::s4u::Mailbox>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Mailbox>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Mailbox>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Mailbox>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1message_1queue_1by_1name_1or_1create(
    JNIEnv* jenv, jclass jcls, jlong cthis, jobject jthis, jstring jarg2)
{
  jlong jresult                      = 0;
  simgrid::s4u::Engine* arg1         = (simgrid::s4u::Engine*)0;
  std::string* arg2                  = 0;
  simgrid::s4u::MessageQueue* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = (simgrid::s4u::MessageQueue*)((simgrid::s4u::Engine const*)arg1)
               ->message_queue_by_name_or_create((std::string const&)*arg2);
  *(simgrid::s4u::MessageQueue**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1actor_1count(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis)
{
  jlong jresult              = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  size_t result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::Engine**)&cthis;
  result  = ((simgrid::s4u::Engine const*)arg1)->get_actor_count();
  jresult = (jlong)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1actor_1max_1pid(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis)
{
  jint jresult               = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  aid_t result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::Engine**)&cthis;
  result  = ((simgrid::s4u::Engine const*)arg1)->get_actor_max_pid();
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1all_1actors(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis)
{
  jlong jresult              = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  SwigValueWrapper<std::vector<boost::intrusive_ptr<simgrid::s4u::Actor>>> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1   = *(simgrid::s4u::Engine**)&cthis;
  result = ((simgrid::s4u::Engine const*)arg1)->get_all_actors();
  *(std::vector<boost::intrusive_ptr<simgrid::s4u::Actor>>**)&jresult =
      new std::vector<boost::intrusive_ptr<simgrid::s4u::Actor>>(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1filtered_1actors(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis,
                                                                                       jlong jarg2)
{
  jlong jresult                                                        = 0;
  simgrid::s4u::Engine* arg1                                           = (simgrid::s4u::Engine*)0;
  std::function<bool(boost::intrusive_ptr<simgrid::s4u::Actor>)>* arg2 = 0;
  SwigValueWrapper<std::vector<boost::intrusive_ptr<simgrid::s4u::Actor>>> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  arg2 = *(std::function<bool(boost::intrusive_ptr<simgrid::s4u::Actor>)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< bool (boost::intrusive_ptr< simgrid::s4u::Actor >) > const & is null");
    return 0;
  }
  result = ((simgrid::s4u::Engine const*)arg1)
               ->get_filtered_actors((std::function<bool(boost::intrusive_ptr<simgrid::s4u::Actor>)> const&)*arg2);
  *(std::vector<boost::intrusive_ptr<simgrid::s4u::Actor>>**)&jresult =
      new std::vector<boost::intrusive_ptr<simgrid::s4u::Actor>>(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1netzone_1root(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis)
{
  jlong jresult                 = 0;
  simgrid::s4u::Engine* arg1    = (simgrid::s4u::Engine*)0;
  simgrid::s4u::NetZone* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                               = *(simgrid::s4u::Engine**)&cthis;
  result                             = (simgrid::s4u::NetZone*)((simgrid::s4u::Engine const*)arg1)->get_netzone_root();
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1get_1all_1netzones(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis)
{
  jlong jresult              = 0;
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;
  SwigValueWrapper<std::vector<simgrid::s4u::NetZone*>> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                                             = *(simgrid::s4u::Engine**)&cthis;
  result                                           = ((simgrid::s4u::Engine const*)arg1)->get_all_netzones();
  *(std::vector<simgrid::s4u::NetZone*>**)&jresult = new std::vector<simgrid::s4u::NetZone*>(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1netzone_1by_1name_1or_1null(JNIEnv* jenv, jclass jcls,
                                                                                             jlong cthis, jobject jthis,
                                                                                             jstring jarg2)
{
  jlong jresult                 = 0;
  simgrid::s4u::Engine* arg1    = (simgrid::s4u::Engine*)0;
  std::string* arg2             = 0;
  simgrid::s4u::NetZone* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result =
      (simgrid::s4u::NetZone*)((simgrid::s4u::Engine const*)arg1)->netzone_by_name_or_null((std::string const&)*arg2);
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1is_1initialized(JNIEnv* jenv, jclass jcls)
{
  jboolean jresult = 0;
  bool result;

  (void)jenv;
  (void)jcls;
  result  = (bool)simgrid::s4u::Engine::is_initialized();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1set_1config_1_1SWIG_10(JNIEnv* jenv, jclass jcls,
                                                                                       jstring cthis)
{
  std::string* arg1 = 0;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  simgrid::s4u::Engine::set_config((std::string const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1set_1config_1_1SWIG_11(JNIEnv* jenv, jclass jcls,
                                                                                       jstring cthis, jint jarg2)
{
  std::string* arg1 = 0;
  int arg2;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  arg2 = (int)jarg2;
  simgrid::s4u::Engine::set_config((std::string const&)*arg1, arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1set_1config_1_1SWIG_12(JNIEnv* jenv, jclass jcls,
                                                                                       jstring cthis, jboolean jarg2)
{
  std::string* arg1 = 0;
  bool arg2;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  arg2 = jarg2 ? true : false;
  simgrid::s4u::Engine::set_config((std::string const&)*arg1, arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1set_1config_1_1SWIG_13(JNIEnv* jenv, jclass jcls,
                                                                                       jstring cthis, jdouble jarg2)
{
  std::string* arg1 = 0;
  double arg2;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  arg2 = (double)jarg2;
  simgrid::s4u::Engine::set_config((std::string const&)*arg1, arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1set_1config_1_1SWIG_14(JNIEnv* jenv, jclass jcls,
                                                                                       jstring cthis, jstring jarg2)
{
  std::string* arg1 = 0;
  std::string* arg2 = 0;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  simgrid::s4u::Engine::set_config((std::string const&)*arg1, (std::string const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1on_1platform_1created_1cb(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis)
{
  std::function<void()>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void()>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::function< void () > const & is null");
    return;
  }
  simgrid::s4u::Engine::on_platform_created_cb((std::function<void()> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1on_1platform_1creation_1cb(JNIEnv* jenv, jclass jcls,
                                                                                           jlong cthis)
{
  std::function<void()>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void()>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::function< void () > const & is null");
    return;
  }
  simgrid::s4u::Engine::on_platform_creation_cb((std::function<void()> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1on_1simulation_1start_1cb(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis)
{
  std::function<void()>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void()>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::function< void () > const & is null");
    return;
  }
  simgrid::s4u::Engine::on_simulation_start_cb((std::function<void()> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1on_1simulation_1end_1cb(JNIEnv* jenv, jclass jcls,
                                                                                        jlong cthis)
{
  std::function<void()>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void()>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::function< void () > const & is null");
    return;
  }
  simgrid::s4u::Engine::on_simulation_end_cb((std::function<void()> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1on_1time_1advance_1cb(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis)
{
  std::function<void(double)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(double)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::function< void (double) > const & is null");
    return;
  }
  simgrid::s4u::Engine::on_time_advance_cb((std::function<void(double)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1on_1deadlock_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(void)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(void)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::function< void (void) > const & is null");
    return;
  }
  simgrid::s4u::Engine::on_deadlock_cb((std::function<void(void)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1die(JNIEnv* jenv, jclass jcls, jstring cthis)
{
  char* arg1 = (char*)0;

  (void)jenv;
  (void)jcls;
  arg1 = 0;
  if (cthis) {
    arg1 = (char*)jenv->GetStringUTFChars(cthis, 0);
    if (!arg1)
      return;
  }
  simgrid_s4u_Engine_die((char const*)arg1);
  if (arg1)
    jenv->ReleaseStringUTFChars(cthis, (const char*)arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1critical(JNIEnv* jenv, jclass jcls, jstring cthis)
{
  char* arg1 = (char*)0;

  (void)jenv;
  (void)jcls;
  arg1 = 0;
  if (cthis) {
    arg1 = (char*)jenv->GetStringUTFChars(cthis, 0);
    if (!arg1)
      return;
  }
  simgrid_s4u_Engine_critical((char const*)arg1);
  if (arg1)
    jenv->ReleaseStringUTFChars(cthis, (const char*)arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1error(JNIEnv* jenv, jclass jcls, jstring cthis)
{
  char* arg1 = (char*)0;

  (void)jenv;
  (void)jcls;
  arg1 = 0;
  if (cthis) {
    arg1 = (char*)jenv->GetStringUTFChars(cthis, 0);
    if (!arg1)
      return;
  }
  simgrid_s4u_Engine_error((char const*)arg1);
  if (arg1)
    jenv->ReleaseStringUTFChars(cthis, (const char*)arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1warn(JNIEnv* jenv, jclass jcls, jstring cthis)
{
  char* arg1 = (char*)0;

  (void)jenv;
  (void)jcls;
  arg1 = 0;
  if (cthis) {
    arg1 = (char*)jenv->GetStringUTFChars(cthis, 0);
    if (!arg1)
      return;
  }
  simgrid_s4u_Engine_warn((char const*)arg1);
  if (arg1)
    jenv->ReleaseStringUTFChars(cthis, (const char*)arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1info(JNIEnv* jenv, jclass jcls, jstring cthis)
{
  char* arg1 = (char*)0;

  (void)jenv;
  (void)jcls;
  arg1 = 0;
  if (cthis) {
    arg1 = (char*)jenv->GetStringUTFChars(cthis, 0);
    if (!arg1)
      return;
  }
  simgrid_s4u_Engine_info((char const*)arg1);
  if (arg1)
    jenv->ReleaseStringUTFChars(cthis, (const char*)arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1verbose(JNIEnv* jenv, jclass jcls, jstring cthis)
{
  char* arg1 = (char*)0;

  (void)jenv;
  (void)jcls;
  arg1 = 0;
  if (cthis) {
    arg1 = (char*)jenv->GetStringUTFChars(cthis, 0);
    if (!arg1)
      return;
  }
  simgrid_s4u_Engine_verbose((char const*)arg1);
  if (arg1)
    jenv->ReleaseStringUTFChars(cthis, (const char*)arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Engine_1debug(JNIEnv* jenv, jclass jcls, jstring cthis)
{
  char* arg1 = (char*)0;

  (void)jenv;
  (void)jcls;
  arg1 = 0;
  if (cthis) {
    arg1 = (char*)jenv->GetStringUTFChars(cthis, 0);
    if (!arg1)
      return;
  }
  simgrid_s4u_Engine_debug((char const*)arg1);
  if (arg1)
    jenv->ReleaseStringUTFChars(cthis, (const char*)arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1Engine(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  simgrid::s4u::Engine* arg1 = (simgrid::s4u::Engine*)0;

  (void)jenv;
  (void)jcls;
  arg1 = *(simgrid::s4u::Engine**)&cthis;
  delete arg1;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_create_1DAG_1from_1dot(JNIEnv* jenv, jclass jcls,
                                                                                jstring cthis)
{
  jlong jresult     = 0;
  std::string* arg1 = 0;
  SwigValueWrapper<std::vector<boost::intrusive_ptr<simgrid::s4u::Activity>>> result;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  result = simgrid::s4u::create_DAG_from_dot((std::string const&)*arg1);
  *(std::vector<boost::intrusive_ptr<simgrid::s4u::Activity>>**)&jresult =
      new std::vector<boost::intrusive_ptr<simgrid::s4u::Activity>>(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_create_1DAG_1from_1DAX(JNIEnv* jenv, jclass jcls,
                                                                                jstring cthis)
{
  jlong jresult     = 0;
  std::string* arg1 = 0;
  SwigValueWrapper<std::vector<boost::intrusive_ptr<simgrid::s4u::Activity>>> result;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  result = simgrid::s4u::create_DAG_from_DAX((std::string const&)*arg1);
  *(std::vector<boost::intrusive_ptr<simgrid::s4u::Activity>>**)&jresult =
      new std::vector<boost::intrusive_ptr<simgrid::s4u::Activity>>(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_create_1DAG_1from_1json(JNIEnv* jenv, jclass jcls,
                                                                                 jstring cthis)
{
  jlong jresult     = 0;
  std::string* arg1 = 0;
  SwigValueWrapper<std::vector<boost::intrusive_ptr<simgrid::s4u::Activity>>> result;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  result = simgrid::s4u::create_DAG_from_json((std::string const&)*arg1);
  *(std::vector<boost::intrusive_ptr<simgrid::s4u::Activity>>**)&jresult =
      new std::vector<boost::intrusive_ptr<simgrid::s4u::Activity>>(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1init(JNIEnv* jenv, jclass jcls)
{
  jlong jresult = 0;
  boost::intrusive_ptr<simgrid::s4u::Exec> result;

  (void)jenv;
  (void)jcls;
  result = simgrid::s4u::Exec::init();

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Exec>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Exec>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Exec>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Exec>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1get_1remaining(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis)
{
  jdouble jresult                                        = 0;
  simgrid::s4u::Exec* arg1                               = (simgrid::s4u::Exec*)0;
  boost::shared_ptr<simgrid::s4u::Exec const>* smartarg1 = 0;
  double result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Exec>**)&cthis;
  arg1      = (simgrid::s4u::Exec*)(smartarg1 ? smartarg1->get() : 0);

  result  = (double)((simgrid::s4u::Exec const*)arg1)->get_remaining();
  jresult = (jdouble)result;
  return jresult;
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1get_1remaining_1ratio(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis)
{
  jdouble jresult                                        = 0;
  simgrid::s4u::Exec* arg1                               = (simgrid::s4u::Exec*)0;
  boost::shared_ptr<simgrid::s4u::Exec const>* smartarg1 = 0;
  double result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Exec>**)&cthis;
  arg1      = (simgrid::s4u::Exec*)(smartarg1 ? smartarg1->get() : 0);

  result  = (double)((simgrid::s4u::Exec const*)arg1)->get_remaining_ratio();
  jresult = (jdouble)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1set_1host(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis, jlong chost, jobject jhost)
{
  ((Exec*)cthis)->set_host((Host*)chost);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1set_1hosts(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                         jobject jthis, jlong jarg2)
{
  auto* self                             = (Exec*)cthis;
  std::vector<simgrid::s4u::Host*>* arg2 = 0;
  boost::intrusive_ptr<simgrid::s4u::Exec> result;

  arg2 = *(std::vector<simgrid::s4u::Host*>**)&jarg2;
  if (!arg2)
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::vector< simgrid::s4u::Host * > const & is null");
  else
    self->set_hosts((std::vector<simgrid::s4u::Host*> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1unset_1host(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                          jobject jthis)
{
  ((Exec*)cthis)->unset_host();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1unset_1hosts(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  ((Exec*)cthis)->unset_hosts();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1set_1flops_1amount(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis, jdouble jarg2)
{
  ((Exec*)cthis)->set_flops_amount(jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1set_1flops_1amounts(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis,
                                                                                  jlong jarg2)
{
  auto* self                = (Exec*)cthis;
  std::vector<double>* arg2 = *(std::vector<double>**)&jarg2;
  if (!arg2)
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::vector< double > const & is null");
  else
    self->set_flops_amounts((std::vector<double> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1set_1bytes_1amounts(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis,
                                                                                  jlong jarg2)
{
  auto* self                = (Exec*)cthis;
  std::vector<double>* arg2 = *(std::vector<double>**)&jarg2;
  if (!arg2)
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::vector< double > const & is null");
  else
    self->set_bytes_amounts((std::vector<double> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1set_1thread_1count(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis, jint jarg2)
{
  ((Exec*)cthis)->set_thread_count(jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1set_1bound(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                         jobject jthis, jdouble jarg2)
{
  ((Exec*)cthis)->set_bound(jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1set_1priority(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jobject jthis, jdouble jarg2)
{
  ((Exec*)cthis)->set_priority(jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1update_1priority(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jdouble jarg2)
{
  ((Exec*)cthis)->update_priority(jarg2);
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1get_1host(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                         jobject jthis)
{
  return (jlong)((Exec*)cthis)->get_host();
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1get_1host_1number(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis)
{
  jlong jresult                                          = 0;
  simgrid::s4u::Exec* arg1                               = (simgrid::s4u::Exec*)0;
  boost::shared_ptr<simgrid::s4u::Exec const>* smartarg1 = 0;
  unsigned int result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Exec>**)&cthis;
  arg1      = (simgrid::s4u::Exec*)(smartarg1 ? smartarg1->get() : 0);

  result  = (unsigned int)((simgrid::s4u::Exec const*)arg1)->get_host_number();
  jresult = (jlong)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1get_1thread_1count(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis)
{
  jint jresult                                           = 0;
  simgrid::s4u::Exec* arg1                               = (simgrid::s4u::Exec*)0;
  boost::shared_ptr<simgrid::s4u::Exec const>* smartarg1 = 0;
  int result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Exec>**)&cthis;
  arg1      = (simgrid::s4u::Exec*)(smartarg1 ? smartarg1->get() : 0);

  result  = (int)((simgrid::s4u::Exec const*)arg1)->get_thread_count();
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1get_1cost(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  jdouble jresult                                        = 0;
  simgrid::s4u::Exec* arg1                               = (simgrid::s4u::Exec*)0;
  boost::shared_ptr<simgrid::s4u::Exec const>* smartarg1 = 0;
  double result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Exec>**)&cthis;
  arg1      = (simgrid::s4u::Exec*)(smartarg1 ? smartarg1->get() : 0);

  result  = (double)((simgrid::s4u::Exec const*)arg1)->get_cost();
  jresult = (jdouble)result;
  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1is_1parallel(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis)
{
  jboolean jresult                                       = 0;
  simgrid::s4u::Exec* arg1                               = (simgrid::s4u::Exec*)0;
  boost::shared_ptr<simgrid::s4u::Exec const>* smartarg1 = 0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Exec>**)&cthis;
  arg1      = (simgrid::s4u::Exec*)(smartarg1 ? smartarg1->get() : 0);

  result  = (bool)((simgrid::s4u::Exec const*)arg1)->is_parallel();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Exec_1is_1assigned(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis)
{
  jboolean jresult                                       = 0;
  simgrid::s4u::Exec* arg1                               = (simgrid::s4u::Exec*)0;
  boost::shared_ptr<simgrid::s4u::Exec const>* smartarg1 = 0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Exec>**)&cthis;
  arg1      = (simgrid::s4u::Exec*)(smartarg1 ? smartarg1->get() : 0);

  result  = (bool)((simgrid::s4u::Exec const*)arg1)->is_assigned();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1Exec(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  intrusive_ptr_release((Exec*)cthis);
}
XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Host_1get_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  const char* result = ((simgrid::s4u::Host*)cthis)->get_cname();
  if (result)
    return jenv->NewStringUTF(result);
  return nullptr;
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Host_1get_1speed(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jobject jthis)
{
  return (jdouble)((simgrid::s4u::Host*)cthis)->get_speed();
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1init(JNIEnv* jenv, jclass jcls)
{
  jlong jresult = 0;
  boost::intrusive_ptr<simgrid::s4u::Io> result;

  (void)jenv;
  (void)jcls;
  result = simgrid::s4u::Io::init();

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Io>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Io>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1get_1remaining(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis)
{
  jdouble jresult                                      = 0;
  simgrid::s4u::Io* arg1                               = (simgrid::s4u::Io*)0;
  boost::shared_ptr<simgrid::s4u::Io const>* smartarg1 = 0;
  double result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Io>**)&cthis;
  arg1      = (simgrid::s4u::Io*)(smartarg1 ? smartarg1->get() : 0);

  result  = (double)((simgrid::s4u::Io const*)arg1)->get_remaining();
  jresult = (jdouble)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1get_1performed_1ioops(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis)
{
  jint jresult                                         = 0;
  simgrid::s4u::Io* arg1                               = (simgrid::s4u::Io*)0;
  boost::shared_ptr<simgrid::s4u::Io const>* smartarg1 = 0;
  sg_size_t result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Io>**)&cthis;
  arg1      = (simgrid::s4u::Io*)(smartarg1 ? smartarg1->get() : 0);

  result  = ((simgrid::s4u::Io const*)arg1)->get_performed_ioops();
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1set_1disk(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                       jobject jthis, jlong jarg2, jobject jarg2_)
{
  jlong jresult                                          = 0;
  simgrid::s4u::Io* arg1                                 = (simgrid::s4u::Io*)0;
  simgrid::s4u::Disk* arg2                               = (simgrid::s4u::Disk*)0;
  boost::shared_ptr<simgrid::s4u::Io>* smartarg1         = 0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg2 = 0;
  boost::intrusive_ptr<simgrid::s4u::Io> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Io>**)&cthis;
  arg1      = (simgrid::s4u::Io*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&jarg2;
  arg2      = (simgrid::s4u::Disk*)(smartarg2 ? smartarg2->get() : 0);

  result = (arg1)->set_disk((simgrid::s4u::Disk const*)arg2);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Io>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Io>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1set_1priority(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis, jdouble jarg2)
{
  jlong jresult          = 0;
  simgrid::s4u::Io* arg1 = (simgrid::s4u::Io*)0;
  double arg2;
  boost::shared_ptr<simgrid::s4u::Io>* smartarg1 = 0;
  boost::intrusive_ptr<simgrid::s4u::Io> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Io>**)&cthis;
  arg1      = (simgrid::s4u::Io*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = (double)jarg2;
  result = (arg1)->set_priority(arg2);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Io>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Io>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1set_1size(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                       jobject jthis, jint jarg2)
{
  jlong jresult          = 0;
  simgrid::s4u::Io* arg1 = (simgrid::s4u::Io*)0;
  sg_size_t arg2;
  boost::shared_ptr<simgrid::s4u::Io>* smartarg1 = 0;
  boost::intrusive_ptr<simgrid::s4u::Io> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Io>**)&cthis;
  arg1      = (simgrid::s4u::Io*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = (sg_size_t)jarg2;
  result = (arg1)->set_size(arg2);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Io>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Io>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1set_1op_1type(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis, jint jarg2)
{
  jlong jresult          = 0;
  simgrid::s4u::Io* arg1 = (simgrid::s4u::Io*)0;
  simgrid::s4u::Io::OpType arg2;
  boost::shared_ptr<simgrid::s4u::Io>* smartarg1 = 0;
  boost::intrusive_ptr<simgrid::s4u::Io> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Io>**)&cthis;
  arg1      = (simgrid::s4u::Io*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = (simgrid::s4u::Io::OpType)jarg2;
  result = (arg1)->set_op_type(arg2);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Io>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Io>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1streamto_1init(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jobject jthis, jlong jarg2, jobject jarg2_,
                                                                            jlong jarg3, jobject jarg3_, jlong jarg4,
                                                                            jobject jarg4_)
{
  jlong jresult                                          = 0;
  simgrid::s4u::Host* arg1                               = (simgrid::s4u::Host*)0;
  simgrid::s4u::Disk* arg2                               = (simgrid::s4u::Disk*)0;
  simgrid::s4u::Host* arg3                               = (simgrid::s4u::Host*)0;
  simgrid::s4u::Disk* arg4                               = (simgrid::s4u::Disk*)0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg1       = 0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg2 = 0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg3       = 0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg4 = 0;
  boost::intrusive_ptr<simgrid::s4u::Io> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;
  (void)jarg3_;
  (void)jarg4_;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Host>**)&cthis;
  arg1      = (simgrid::s4u::Host*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&jarg2;
  arg2      = (simgrid::s4u::Disk*)(smartarg2 ? smartarg2->get() : 0);

  // plain pointer
  smartarg3 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg3;
  arg3      = (simgrid::s4u::Host*)(smartarg3 ? smartarg3->get() : 0);

  // plain pointer
  smartarg4 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&jarg4;
  arg4      = (simgrid::s4u::Disk*)(smartarg4 ? smartarg4->get() : 0);

  result =
      simgrid::s4u::Io::streamto_init(arg1, (simgrid::s4u::Disk const*)arg2, arg3, (simgrid::s4u::Disk const*)arg4);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Io>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Io>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1streamto_1async(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis, jlong jarg2, jobject jarg2_,
                                                                             jlong jarg3, jobject jarg3_, jlong jarg4,
                                                                             jobject jarg4_, jlong jarg5)
{
  jlong jresult            = 0;
  simgrid::s4u::Host* arg1 = (simgrid::s4u::Host*)0;
  simgrid::s4u::Disk* arg2 = (simgrid::s4u::Disk*)0;
  simgrid::s4u::Host* arg3 = (simgrid::s4u::Host*)0;
  simgrid::s4u::Disk* arg4 = (simgrid::s4u::Disk*)0;
  uint64_t arg5;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg1       = 0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg2 = 0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg3       = 0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg4 = 0;
  boost::intrusive_ptr<simgrid::s4u::Io> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;
  (void)jarg3_;
  (void)jarg4_;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Host>**)&cthis;
  arg1      = (simgrid::s4u::Host*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&jarg2;
  arg2      = (simgrid::s4u::Disk*)(smartarg2 ? smartarg2->get() : 0);

  // plain pointer
  smartarg3 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg3;
  arg3      = (simgrid::s4u::Host*)(smartarg3 ? smartarg3->get() : 0);

  // plain pointer
  smartarg4 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&jarg4;
  arg4      = (simgrid::s4u::Disk*)(smartarg4 ? smartarg4->get() : 0);

  arg5   = (uint64_t)jarg5;
  result = simgrid::s4u::Io::streamto_async(arg1, (simgrid::s4u::Disk const*)arg2, arg3,
                                            (simgrid::s4u::Disk const*)arg4, std::move(arg5));

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Io>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Io>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1streamto(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                     jobject jthis, jlong jarg2, jobject jarg2_,
                                                                     jlong jarg3, jobject jarg3_, jlong jarg4,
                                                                     jobject jarg4_, jlong jarg5)
{
  simgrid::s4u::Host* arg1 = (simgrid::s4u::Host*)0;
  simgrid::s4u::Disk* arg2 = (simgrid::s4u::Disk*)0;
  simgrid::s4u::Host* arg3 = (simgrid::s4u::Host*)0;
  simgrid::s4u::Disk* arg4 = (simgrid::s4u::Disk*)0;
  uint64_t arg5;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg1       = 0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg2 = 0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg3       = 0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg4 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;
  (void)jarg3_;
  (void)jarg4_;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Host>**)&cthis;
  arg1      = (simgrid::s4u::Host*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&jarg2;
  arg2      = (simgrid::s4u::Disk*)(smartarg2 ? smartarg2->get() : 0);

  // plain pointer
  smartarg3 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg3;
  arg3      = (simgrid::s4u::Host*)(smartarg3 ? smartarg3->get() : 0);

  // plain pointer
  smartarg4 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&jarg4;
  arg4      = (simgrid::s4u::Disk*)(smartarg4 ? smartarg4->get() : 0);

  arg5 = (uint64_t)jarg5;
  simgrid::s4u::Io::streamto(arg1, (simgrid::s4u::Disk const*)arg2, arg3, (simgrid::s4u::Disk const*)arg4,
                             std::move(arg5));
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1set_1source(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                         jobject jthis, jlong jarg2, jobject jarg2_,
                                                                         jlong jarg3, jobject jarg3_)
{
  jlong jresult                                          = 0;
  simgrid::s4u::Io* arg1                                 = (simgrid::s4u::Io*)0;
  simgrid::s4u::Host* arg2                               = (simgrid::s4u::Host*)0;
  simgrid::s4u::Disk* arg3                               = (simgrid::s4u::Disk*)0;
  boost::shared_ptr<simgrid::s4u::Io>* smartarg1         = 0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg2       = 0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg3 = 0;
  boost::intrusive_ptr<simgrid::s4u::Io> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;
  (void)jarg3_;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Io>**)&cthis;
  arg1      = (simgrid::s4u::Io*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg2;
  arg2      = (simgrid::s4u::Host*)(smartarg2 ? smartarg2->get() : 0);

  // plain pointer
  smartarg3 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&jarg3;
  arg3      = (simgrid::s4u::Disk*)(smartarg3 ? smartarg3->get() : 0);

  result = (arg1)->set_source(arg2, (simgrid::s4u::Disk const*)arg3);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Io>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Io>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1set_1destination(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis, jlong jarg2,
                                                                              jobject jarg2_, jlong jarg3,
                                                                              jobject jarg3_)
{
  jlong jresult                                          = 0;
  simgrid::s4u::Io* arg1                                 = (simgrid::s4u::Io*)0;
  simgrid::s4u::Host* arg2                               = (simgrid::s4u::Host*)0;
  simgrid::s4u::Disk* arg3                               = (simgrid::s4u::Disk*)0;
  boost::shared_ptr<simgrid::s4u::Io>* smartarg1         = 0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg2       = 0;
  boost::shared_ptr<simgrid::s4u::Disk const>* smartarg3 = 0;
  boost::intrusive_ptr<simgrid::s4u::Io> result;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Io>**)&cthis;
  arg1      = (simgrid::s4u::Io*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg2;
  arg2      = (simgrid::s4u::Host*)(smartarg2 ? smartarg2->get() : 0);

  // plain pointer
  smartarg3 = *(boost::shared_ptr<const simgrid::s4u::Disk>**)&jarg3;
  arg3      = (simgrid::s4u::Disk*)(smartarg3 ? smartarg3->get() : 0);

  result = (arg1)->set_destination(arg2, (simgrid::s4u::Disk const*)arg3);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Io>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Io>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1update_1priority(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis, jdouble jarg2)
{
  jlong jresult          = 0;
  simgrid::s4u::Io* arg1 = (simgrid::s4u::Io*)0;
  double arg2;
  boost::shared_ptr<simgrid::s4u::Io>* smartarg1 = 0;
  boost::intrusive_ptr<simgrid::s4u::Io> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Io>**)&cthis;
  arg1      = (simgrid::s4u::Io*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = (double)jarg2;
  result = (arg1)->update_priority(arg2);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Io>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Io>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Io>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Io_1is_1assigned(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis)
{
  jboolean jresult                                     = 0;
  simgrid::s4u::Io* arg1                               = (simgrid::s4u::Io*)0;
  boost::shared_ptr<simgrid::s4u::Io const>* smartarg1 = 0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Io>**)&cthis;
  arg1      = (simgrid::s4u::Io*)(smartarg1 ? smartarg1->get() : 0);

  result  = (bool)((simgrid::s4u::Io const*)arg1)->is_assigned();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1Host(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  simgrid::s4u::Host* arg1                         = (simgrid::s4u::Host*)0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Host>**)&cthis;
  arg1      = (simgrid::s4u::Host*)(smartarg1 ? smartarg1->get() : 0);

  (void)arg1;
  delete smartarg1;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1SharingPolicy_1NONLINEAR_1get(JNIEnv* jenv, jclass jcls)
{
  jint jresult = 0;
  simgrid::s4u::Link::SharingPolicy result;

  (void)jenv;
  (void)jcls;
  result  = (simgrid::s4u::Link::SharingPolicy)simgrid::s4u::Link::SharingPolicy::NONLINEAR;
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1SharingPolicy_1WIFI_1get(JNIEnv* jenv, jclass jcls)
{
  jint jresult = 0;
  simgrid::s4u::Link::SharingPolicy result;

  (void)jenv;
  (void)jcls;
  result  = (simgrid::s4u::Link::SharingPolicy)simgrid::s4u::Link::SharingPolicy::WIFI;
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1SharingPolicy_1SPLITDUPLEX_1get(JNIEnv* jenv, jclass jcls)
{
  jint jresult = 0;
  simgrid::s4u::Link::SharingPolicy result;

  (void)jenv;
  (void)jcls;
  result  = (simgrid::s4u::Link::SharingPolicy)simgrid::s4u::Link::SharingPolicy::SPLITDUPLEX;
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1SharingPolicy_1SHARED_1get(JNIEnv* jenv, jclass jcls)
{
  jint jresult = 0;
  simgrid::s4u::Link::SharingPolicy result;

  (void)jenv;
  (void)jcls;
  result  = (simgrid::s4u::Link::SharingPolicy)simgrid::s4u::Link::SharingPolicy::SHARED;
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1SharingPolicy_1FATPIPE_1get(JNIEnv* jenv, jclass jcls)
{
  jint jresult = 0;
  simgrid::s4u::Link::SharingPolicy result;

  (void)jenv;
  (void)jcls;
  result  = (simgrid::s4u::Link::SharingPolicy)simgrid::s4u::Link::SharingPolicy::FATPIPE;
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1by_1name(JNIEnv* jenv, jclass jcls, jstring cthis)
{
  jlong jresult              = 0;
  std::string* arg1          = 0;
  simgrid::s4u::Link* result = 0;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  result                          = (simgrid::s4u::Link*)simgrid::s4u::Link::by_name((std::string const&)*arg1);
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1by_1name_1or_1null(JNIEnv* jenv, jclass jcls,
                                                                                  jstring cthis)
{
  jlong jresult              = 0;
  std::string* arg1          = 0;
  simgrid::s4u::Link* result = 0;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  result                          = (simgrid::s4u::Link*)simgrid::s4u::Link::by_name_or_null((std::string const&)*arg1);
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1get_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)cthis;
  const char* result       = (arg1)->get_cname();
  if (result)
    return jenv->NewStringUTF(result);
  return nullptr;
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1get_1bandwidth(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis)
{
  return (jdouble)((simgrid::s4u::Link*)cthis)->get_bandwidth();
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1set_1bandwidth(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis, jdouble jarg2)
{
  jlong jresult            = 0;
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;
  double arg2;
  simgrid::s4u::Link* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                            = *(simgrid::s4u::Link**)&cthis;
  arg2                            = (double)jarg2;
  result                          = (simgrid::s4u::Link*)(arg1)->set_bandwidth(arg2);
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1get_1latency(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis)
{
  jdouble jresult          = 0;
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;
  double result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::Link**)&cthis;
  result  = (double)((simgrid::s4u::Link const*)arg1)->get_latency();
  jresult = (jdouble)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1set_1latency_1_1SWIG_10(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis,
                                                                                       jdouble jarg2)
{
  jlong jresult            = 0;
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;
  double arg2;
  simgrid::s4u::Link* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                            = *(simgrid::s4u::Link**)&cthis;
  arg2                            = (double)jarg2;
  result                          = (simgrid::s4u::Link*)(arg1)->set_latency(arg2);
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1set_1latency_1_1SWIG_11(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis,
                                                                                       jstring jarg2)
{
  jlong jresult              = 0;
  simgrid::s4u::Link* arg1   = (simgrid::s4u::Link*)0;
  std::string* arg2          = 0;
  simgrid::s4u::Link* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Link**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result                          = (simgrid::s4u::Link*)(arg1)->set_latency((std::string const&)*arg2);
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1get_1sharing_1policy(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis)
{
  jint jresult             = 0;
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;
  simgrid::s4u::Link::SharingPolicy result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::Link**)&cthis;
  result  = (simgrid::s4u::Link::SharingPolicy)((simgrid::s4u::Link const*)arg1)->get_sharing_policy();
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1get_1property(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jstring jarg2)
{
  jstring jresult          = 0;
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;
  std::string* arg2        = 0;
  char* result             = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Link**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = (char*)((simgrid::s4u::Link const*)arg1)->get_property((std::string const&)*arg2);
  if (result)
    jresult = jenv->NewStringUTF((const char*)result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1set_1properties(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jlong jarg2)
{
  jlong jresult                                      = 0;
  simgrid::s4u::Link* arg1                           = (simgrid::s4u::Link*)0;
  std::unordered_map<std::string, std::string>* arg2 = 0;
  simgrid::s4u::Link* result                         = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Link**)&cthis;
  arg2 = *(std::unordered_map<std::string, std::string>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::unordered_map< std::string,std::string > const & is null");
    return 0;
  }
  result = (simgrid::s4u::Link*)(arg1)->set_properties((std::unordered_map<std::string, std::string> const&)*arg2);
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1set_1property(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis, jstring jarg2,
                                                                             jstring jarg3)
{
  jlong jresult              = 0;
  simgrid::s4u::Link* arg1   = (simgrid::s4u::Link*)0;
  std::string* arg2          = 0;
  std::string* arg3          = 0;
  simgrid::s4u::Link* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Link**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  if (!jarg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg3_pstr = (const char*)jenv->GetStringUTFChars(jarg3, 0);
  if (!arg3_pstr)
    return 0;
  std::string arg3_str(arg3_pstr);
  arg3 = &arg3_str;
  jenv->ReleaseStringUTFChars(jarg3, arg3_pstr);
  result = (simgrid::s4u::Link*)(arg1)->set_property((std::string const&)*arg2, (std::string const&)*arg3);
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1set_1concurrency_1limit(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis,
                                                                                       jint jarg2)
{
  jlong jresult            = 0;
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;
  int arg2;
  simgrid::s4u::Link* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                            = *(simgrid::s4u::Link**)&cthis;
  arg2                            = (int)jarg2;
  result                          = (simgrid::s4u::Link*)(arg1)->set_concurrency_limit(arg2);
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1get_1concurrency_1limit(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis)
{
  jint jresult             = 0;
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;
  int result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::Link**)&cthis;
  result  = (int)((simgrid::s4u::Link const*)arg1)->get_concurrency_limit();
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1set_1host_1wifi_1rate(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis,
                                                                                    jlong jarg2, jobject jarg2_,
                                                                                    jint jarg3)
{
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;
  simgrid::s4u::Host* arg2 = (simgrid::s4u::Host*)0;
  int arg3;
  boost::shared_ptr<simgrid::s4u::Host const>* smartarg2 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;
  arg1 = *(simgrid::s4u::Link**)&cthis;

  // plain pointer
  smartarg2 = *(boost::shared_ptr<const simgrid::s4u::Host>**)&jarg2;
  arg2      = (simgrid::s4u::Host*)(smartarg2 ? smartarg2->get() : 0);

  arg3 = (int)jarg3;
  ((simgrid::s4u::Link const*)arg1)->set_host_wifi_rate((simgrid::s4u::Host const*)arg2, arg3);
}

XBT_PUBLIC jdouble JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1get_1load(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  jdouble jresult          = 0;
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;
  double result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::Link**)&cthis;
  result  = (double)((simgrid::s4u::Link const*)arg1)->get_load();
  jresult = (jdouble)result;
  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1is_1used(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  jboolean jresult         = 0;
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::Link**)&cthis;
  result  = (bool)((simgrid::s4u::Link const*)arg1)->is_used();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1is_1shared(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis)
{
  jboolean jresult         = 0;
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::Link**)&cthis;
  result  = (bool)((simgrid::s4u::Link const*)arg1)->is_shared();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1turn_1on(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                       jobject jthis)
{
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Link**)&cthis;
  (arg1)->turn_on();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1turn_1off(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis)
{
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Link**)&cthis;
  (arg1)->turn_off();
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1is_1on(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                         jobject jthis)
{
  jboolean jresult         = 0;
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::Link**)&cthis;
  result  = (bool)((simgrid::s4u::Link const*)arg1)->is_on();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1seal(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                    jobject jthis)
{
  jlong jresult              = 0;
  simgrid::s4u::Link* arg1   = (simgrid::s4u::Link*)0;
  simgrid::s4u::Link* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                            = *(simgrid::s4u::Link**)&cthis;
  result                          = (simgrid::s4u::Link*)(arg1)->seal();
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1on_1onoff_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::Link const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::Link const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Link const &) > const & is null");
    return;
  }
  simgrid::s4u::Link::on_onoff_cb((std::function<void(simgrid::s4u::Link const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1on_1this_1onoff_1cb(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis,
                                                                                  jlong jarg2)
{
  simgrid::s4u::Link* arg1                             = (simgrid::s4u::Link*)0;
  std::function<void(simgrid::s4u::Link const&)>* arg2 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Link**)&cthis;
  arg2 = *(std::function<void(simgrid::s4u::Link const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Link const &) > const & is null");
    return;
  }
  (arg1)->on_this_onoff_cb((std::function<void(simgrid::s4u::Link const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1on_1bandwidth_1change_1cb(JNIEnv* jenv, jclass jcls,
                                                                                        jlong cthis)
{
  std::function<void(simgrid::s4u::Link const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::Link const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Link const &) > const & is null");
    return;
  }
  simgrid::s4u::Link::on_bandwidth_change_cb((std::function<void(simgrid::s4u::Link const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1on_1this_1bandwidth_1change_1cb(JNIEnv* jenv, jclass jcls,
                                                                                              jlong cthis,
                                                                                              jobject jthis,
                                                                                              jlong jarg2)
{
  simgrid::s4u::Link* arg1                             = (simgrid::s4u::Link*)0;
  std::function<void(simgrid::s4u::Link const&)>* arg2 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Link**)&cthis;
  arg2 = *(std::function<void(simgrid::s4u::Link const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Link const &) > const & is null");
    return;
  }
  (arg1)->on_this_bandwidth_change_cb((std::function<void(simgrid::s4u::Link const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1on_1destruction_1cb(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis)
{
  std::function<void(simgrid::s4u::Link const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::Link const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Link const &) > const & is null");
    return;
  }
  simgrid::s4u::Link::on_destruction_cb((std::function<void(simgrid::s4u::Link const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Link_1on_1this_1destruction_1cb(JNIEnv* jenv, jclass jcls,
                                                                                        jlong cthis, jobject jthis,
                                                                                        jlong jarg2)
{
  simgrid::s4u::Link* arg1                             = (simgrid::s4u::Link*)0;
  std::function<void(simgrid::s4u::Link const&)>* arg2 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::Link**)&cthis;
  arg2 = *(std::function<void(simgrid::s4u::Link const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::Link const &) > const & is null");
    return;
  }
  (arg1)->on_this_destruction_cb((std::function<void(simgrid::s4u::Link const&)> const&)*arg2);
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_LinkInRoute_1Direction_1UP_1get(JNIEnv* jenv, jclass jcls)
{
  jint jresult = 0;
  simgrid::s4u::LinkInRoute::Direction result;

  (void)jenv;
  (void)jcls;
  result  = (simgrid::s4u::LinkInRoute::Direction)simgrid::s4u::LinkInRoute::Direction::UP;
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_LinkInRoute_1Direction_1DOWN_1get(JNIEnv* jenv, jclass jcls)
{
  jint jresult = 0;
  simgrid::s4u::LinkInRoute::Direction result;

  (void)jenv;
  (void)jcls;
  result  = (simgrid::s4u::LinkInRoute::Direction)simgrid::s4u::LinkInRoute::Direction::DOWN;
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_LinkInRoute_1Direction_1NONE_1get(JNIEnv* jenv, jclass jcls)
{
  jint jresult = 0;
  simgrid::s4u::LinkInRoute::Direction result;

  (void)jenv;
  (void)jcls;
  result  = (simgrid::s4u::LinkInRoute::Direction)simgrid::s4u::LinkInRoute::Direction::NONE;
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_new_1LinkInRoute_1_1SWIG_10(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis)
{
  jlong jresult                     = 0;
  simgrid::s4u::Link* arg1          = (simgrid::s4u::Link*)0;
  simgrid::s4u::LinkInRoute* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1   = *(simgrid::s4u::Link**)&cthis;
  result = (simgrid::s4u::LinkInRoute*)new simgrid::s4u::LinkInRoute((simgrid::s4u::Link const*)arg1);
  *(simgrid::s4u::LinkInRoute**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_new_1LinkInRoute_1_1SWIG_11(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis,
                                                                                     jint jarg2)
{
  jlong jresult            = 0;
  simgrid::s4u::Link* arg1 = (simgrid::s4u::Link*)0;
  simgrid::s4u::LinkInRoute::Direction arg2;
  simgrid::s4u::LinkInRoute* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1   = *(simgrid::s4u::Link**)&cthis;
  arg2   = (simgrid::s4u::LinkInRoute::Direction)jarg2;
  result = (simgrid::s4u::LinkInRoute*)new simgrid::s4u::LinkInRoute((simgrid::s4u::Link const*)arg1, arg2);
  *(simgrid::s4u::LinkInRoute**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_LinkInRoute_1get_1direction(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis)
{
  jint jresult                    = 0;
  simgrid::s4u::LinkInRoute* arg1 = (simgrid::s4u::LinkInRoute*)0;
  simgrid::s4u::LinkInRoute::Direction result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::LinkInRoute**)&cthis;
  result  = (simgrid::s4u::LinkInRoute::Direction)((simgrid::s4u::LinkInRoute const*)arg1)->get_direction();
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_LinkInRoute_1get_1link(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis)
{
  jlong jresult                   = 0;
  simgrid::s4u::LinkInRoute* arg1 = (simgrid::s4u::LinkInRoute*)0;
  simgrid::s4u::Link* result      = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                            = *(simgrid::s4u::LinkInRoute**)&cthis;
  result                          = (simgrid::s4u::Link*)((simgrid::s4u::LinkInRoute const*)arg1)->get_link();
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1LinkInRoute(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  simgrid::s4u::LinkInRoute* arg1 = (simgrid::s4u::LinkInRoute*)0;

  (void)jenv;
  (void)jcls;
  arg1 = *(simgrid::s4u::LinkInRoute**)&cthis;
  delete arg1;
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1get_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis)
{
  jstring jresult             = 0;
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;
  char* result                = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1   = *(simgrid::s4u::NetZone**)&cthis;
  result = (char*)((simgrid::s4u::NetZone const*)arg1)->get_cname();
  if (result)
    jresult = jenv->NewStringUTF((const char*)result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1get_1parent(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis)
{
  jlong jresult                 = 0;
  simgrid::s4u::NetZone* arg1   = (simgrid::s4u::NetZone*)0;
  simgrid::s4u::NetZone* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                               = *(simgrid::s4u::NetZone**)&cthis;
  result                             = (simgrid::s4u::NetZone*)((simgrid::s4u::NetZone const*)arg1)->get_parent();
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1set_1parent(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis, jlong jarg2,
                                                                              jobject jarg2_)
{
  jlong jresult                 = 0;
  simgrid::s4u::NetZone* arg1   = (simgrid::s4u::NetZone*)0;
  simgrid::s4u::NetZone* arg2   = (simgrid::s4u::NetZone*)0;
  simgrid::s4u::NetZone* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;
  arg1                               = *(simgrid::s4u::NetZone**)&cthis;
  arg2                               = *(simgrid::s4u::NetZone**)&jarg2;
  result                             = (simgrid::s4u::NetZone*)(arg1)->set_parent((simgrid::s4u::NetZone const*)arg2);
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1get_1children(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis)
{
  jlong jresult               = 0;
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;
  SwigValueWrapper<std::vector<simgrid::s4u::NetZone*>> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                                             = *(simgrid::s4u::NetZone**)&cthis;
  result                                           = ((simgrid::s4u::NetZone const*)arg1)->get_children();
  *(std::vector<simgrid::s4u::NetZone*>**)&jresult = new std::vector<simgrid::s4u::NetZone*>(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1get_1all_1hosts(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis)
{
  jlong jresult               = 0;
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;
  SwigValueWrapper<std::vector<simgrid::s4u::Host*>> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                                          = *(simgrid::s4u::NetZone**)&cthis;
  result                                        = ((simgrid::s4u::NetZone const*)arg1)->get_all_hosts();
  *(std::vector<simgrid::s4u::Host*>**)&jresult = new std::vector<simgrid::s4u::Host*>(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1get_1host_1count(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis)
{
  jlong jresult               = 0;
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;
  size_t result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::NetZone**)&cthis;
  result  = ((simgrid::s4u::NetZone const*)arg1)->get_host_count();
  jresult = (jlong)result;
  return jresult;
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1get_1property(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis,
                                                                                  jstring jarg2)
{
  jstring jresult             = 0;
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;
  std::string* arg2           = 0;
  char* result                = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::NetZone**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  result = (char*)((simgrid::s4u::NetZone const*)arg1)->get_property((std::string const&)*arg2);
  if (result)
    jresult = jenv->NewStringUTF((const char*)result);
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1set_1property(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jstring jarg2,
                                                                               jstring jarg3)
{
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;
  std::string* arg2           = 0;
  std::string* arg3           = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::NetZone**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  if (!jarg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return;
  }
  const char* arg3_pstr = (const char*)jenv->GetStringUTFChars(jarg3, 0);
  if (!arg3_pstr)
    return;
  std::string arg3_str(arg3_pstr);
  arg3 = &arg3_str;
  jenv->ReleaseStringUTFChars(jarg3, arg3_pstr);
  (arg1)->set_property((std::string const&)*arg2, (std::string const&)*arg3);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1on_1seal_1cb(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::NetZone const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::NetZone const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::NetZone const &) > const & is null");
    return;
  }
  simgrid::s4u::NetZone::on_seal_cb((std::function<void(simgrid::s4u::NetZone const&)> const&)*arg1);
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1create_1host_1_1SWIG_10(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jobject jthis,
                                                                                          jstring jarg2, jlong jarg3)
{
  jlong jresult               = 0;
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;
  std::string* arg2           = 0;
  std::vector<double>* arg3   = 0;
  simgrid::s4u::Host* result  = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::NetZone**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  arg3 = *(std::vector<double>**)&jarg3;
  if (!arg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::vector< double > const & is null");
    return 0;
  }
  result = (simgrid::s4u::Host*)(arg1)->create_host((std::string const&)*arg2, (std::vector<double> const&)*arg3);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Host>(result, SWIG_intrusive_deleter<simgrid::s4u::Host>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Host>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1create_1host_1_1SWIG_11(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jobject jthis,
                                                                                          jstring jarg2, jdouble jarg3)
{
  jlong jresult               = 0;
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;
  std::string* arg2           = 0;
  double arg3;
  simgrid::s4u::Host* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::NetZone**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  arg3   = (double)jarg3;
  result = (simgrid::s4u::Host*)(arg1)->create_host((std::string const&)*arg2, arg3);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Host>(result, SWIG_intrusive_deleter<simgrid::s4u::Host>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Host>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1create_1host_1_1SWIG_12(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jobject jthis,
                                                                                          jstring jarg2, jlong jarg3)
{
  jlong jresult                  = 0;
  simgrid::s4u::NetZone* arg1    = (simgrid::s4u::NetZone*)0;
  std::string* arg2              = 0;
  std::vector<std::string>* arg3 = 0;
  simgrid::s4u::Host* result     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::NetZone**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  arg3 = *(std::vector<std::string>**)&jarg3;
  if (!arg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::vector< std::string > const & is null");
    return 0;
  }
  result = (simgrid::s4u::Host*)(arg1)->create_host((std::string const&)*arg2, (std::vector<std::string> const&)*arg3);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Host>(result, SWIG_intrusive_deleter<simgrid::s4u::Host>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Host>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1create_1host_1_1SWIG_13(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jobject jthis,
                                                                                          jstring jarg2, jstring jarg3)
{
  jlong jresult               = 0;
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;
  std::string* arg2           = 0;
  std::string* arg3           = 0;
  simgrid::s4u::Host* result  = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::NetZone**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  if (!jarg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg3_pstr = (const char*)jenv->GetStringUTFChars(jarg3, 0);
  if (!arg3_pstr)
    return 0;
  std::string arg3_str(arg3_pstr);
  arg3 = &arg3_str;
  jenv->ReleaseStringUTFChars(jarg3, arg3_pstr);
  result = (simgrid::s4u::Host*)(arg1)->create_host((std::string const&)*arg2, (std::string const&)*arg3);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Host>(result, SWIG_intrusive_deleter<simgrid::s4u::Host>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Host>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1create_1link_1_1SWIG_10(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jobject jthis,
                                                                                          jstring jarg2, jlong jarg3)
{
  jlong jresult               = 0;
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;
  std::string* arg2           = 0;
  std::vector<double>* arg3   = 0;
  simgrid::s4u::Link* result  = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::NetZone**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  arg3 = *(std::vector<double>**)&jarg3;
  if (!arg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::vector< double > const & is null");
    return 0;
  }
  result = (simgrid::s4u::Link*)(arg1)->create_link((std::string const&)*arg2, (std::vector<double> const&)*arg3);
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1create_1link_1_1SWIG_11(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jobject jthis,
                                                                                          jstring jarg2, jdouble jarg3)
{
  jlong jresult               = 0;
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;
  std::string* arg2           = 0;
  double arg3;
  simgrid::s4u::Link* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::NetZone**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  arg3                            = (double)jarg3;
  result                          = (simgrid::s4u::Link*)(arg1)->create_link((std::string const&)*arg2, arg3);
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1create_1link_1_1SWIG_12(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jobject jthis,
                                                                                          jstring jarg2, jlong jarg3)
{
  jlong jresult                  = 0;
  simgrid::s4u::NetZone* arg1    = (simgrid::s4u::NetZone*)0;
  std::string* arg2              = 0;
  std::vector<std::string>* arg3 = 0;
  simgrid::s4u::Link* result     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::NetZone**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  arg3 = *(std::vector<std::string>**)&jarg3;
  if (!arg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::vector< std::string > const & is null");
    return 0;
  }
  result = (simgrid::s4u::Link*)(arg1)->create_link((std::string const&)*arg2, (std::vector<std::string> const&)*arg3);
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1create_1link_1_1SWIG_13(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jobject jthis,
                                                                                          jstring jarg2, jstring jarg3)
{
  jlong jresult               = 0;
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;
  std::string* arg2           = 0;
  std::string* arg3           = 0;
  simgrid::s4u::Link* result  = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::NetZone**)&cthis;
  if (!jarg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg2_pstr = (const char*)jenv->GetStringUTFChars(jarg2, 0);
  if (!arg2_pstr)
    return 0;
  std::string arg2_str(arg2_pstr);
  arg2 = &arg2_str;
  jenv->ReleaseStringUTFChars(jarg2, arg2_pstr);
  if (!jarg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg3_pstr = (const char*)jenv->GetStringUTFChars(jarg3, 0);
  if (!arg3_pstr)
    return 0;
  std::string arg3_str(arg3_pstr);
  arg3 = &arg3_str;
  jenv->ReleaseStringUTFChars(jarg3, arg3_pstr);
  result = (simgrid::s4u::Link*)(arg1)->create_link((std::string const&)*arg2, (std::string const&)*arg3);
  *(simgrid::s4u::Link**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1seal(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                       jobject jthis)
{
  jlong jresult                 = 0;
  simgrid::s4u::NetZone* arg1   = (simgrid::s4u::NetZone*)0;
  simgrid::s4u::NetZone* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                               = *(simgrid::s4u::NetZone**)&cthis;
  result                             = (simgrid::s4u::NetZone*)(arg1)->seal();
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1set_1latency_1factor_1cb(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jobject jthis,
                                                                                          jlong jarg2)
{
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;
  std::function<double(double, simgrid::s4u::Host const*, simgrid::s4u::Host const*,
                       std::vector<simgrid::s4u::Link*> const&, std::unordered_set<simgrid::s4u::NetZone*> const&)>*
      arg2 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::NetZone**)&cthis;
  arg2 = *(std::function<double(double, simgrid::s4u::Host const*, simgrid::s4u::Host const*,
                                std::vector<simgrid::s4u::Link*> const&,
                                std::unordered_set<simgrid::s4u::NetZone*> const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(
        jenv, SWIG_JavaNullPointerException,
        "std::function< double (double,simgrid::s4u::Host const *,simgrid::s4u::Host const *,std::vector< "
        "simgrid::s4u::Link * > const &,std::unordered_set< simgrid::s4u::NetZone * > const &) > const & is null");
    return;
  }
  ((simgrid::s4u::NetZone const*)arg1)
      ->set_latency_factor_cb((std::function<double(double, simgrid::s4u::Host const*, simgrid::s4u::Host const*,
                                                    std::vector<simgrid::s4u::Link*> const&,
                                                    std::unordered_set<simgrid::s4u::NetZone*> const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_NetZone_1set_1bandwidth_1factor_1cb(JNIEnv* jenv, jclass jcls,
                                                                                            jlong cthis, jobject jthis,
                                                                                            jlong jarg2)
{
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;
  std::function<double(double, simgrid::s4u::Host const*, simgrid::s4u::Host const*,
                       std::vector<simgrid::s4u::Link*> const&, std::unordered_set<simgrid::s4u::NetZone*> const&)>*
      arg2 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::NetZone**)&cthis;
  arg2 = *(std::function<double(double, simgrid::s4u::Host const*, simgrid::s4u::Host const*,
                                std::vector<simgrid::s4u::Link*> const&,
                                std::unordered_set<simgrid::s4u::NetZone*> const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(
        jenv, SWIG_JavaNullPointerException,
        "std::function< double (double,simgrid::s4u::Host const *,simgrid::s4u::Host const *,std::vector< "
        "simgrid::s4u::Link * > const &,std::unordered_set< simgrid::s4u::NetZone * > const &) > const & is null");
    return;
  }
  ((simgrid::s4u::NetZone const*)arg1)
      ->set_bandwidth_factor_cb((std::function<double(double, simgrid::s4u::Host const*, simgrid::s4u::Host const*,
                                                      std::vector<simgrid::s4u::Link*> const&,
                                                      std::unordered_set<simgrid::s4u::NetZone*> const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1NetZone(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  simgrid::s4u::NetZone* arg1 = (simgrid::s4u::NetZone*)0;

  (void)jenv;
  (void)jcls;
  arg1 = *(simgrid::s4u::NetZone**)&cthis;
  delete arg1;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_create_1full_1zone(JNIEnv* jenv, jclass jcls, jstring cthis)
{
  jlong jresult                 = 0;
  std::string* arg1             = 0;
  simgrid::s4u::NetZone* result = 0;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  result = (simgrid::s4u::NetZone*)simgrid::s4u::create_full_zone((std::string const&)*arg1);
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_create_1star_1zone(JNIEnv* jenv, jclass jcls, jstring cthis)
{
  jlong jresult                 = 0;
  std::string* arg1             = 0;
  simgrid::s4u::NetZone* result = 0;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  result = (simgrid::s4u::NetZone*)simgrid::s4u::create_star_zone((std::string const&)*arg1);
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_create_1dijkstra_1zone(JNIEnv* jenv, jclass jcls,
                                                                                jstring cthis, jboolean jarg2)
{
  jlong jresult     = 0;
  std::string* arg1 = 0;
  bool arg2;
  simgrid::s4u::NetZone* result = 0;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  arg2   = jarg2 ? true : false;
  result = (simgrid::s4u::NetZone*)simgrid::s4u::create_dijkstra_zone((std::string const&)*arg1, arg2);
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_create_1empty_1zone(JNIEnv* jenv, jclass jcls, jstring cthis)
{
  jlong jresult                 = 0;
  std::string* arg1             = 0;
  simgrid::s4u::NetZone* result = 0;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  result = (simgrid::s4u::NetZone*)simgrid::s4u::create_empty_zone((std::string const&)*arg1);
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_create_1floyd_1zone(JNIEnv* jenv, jclass jcls, jstring cthis)
{
  jlong jresult                 = 0;
  std::string* arg1             = 0;
  simgrid::s4u::NetZone* result = 0;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  result = (simgrid::s4u::NetZone*)simgrid::s4u::create_floyd_zone((std::string const&)*arg1);
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_create_1vivaldi_1zone(JNIEnv* jenv, jclass jcls, jstring cthis)
{
  jlong jresult                 = 0;
  std::string* arg1             = 0;
  simgrid::s4u::NetZone* result = 0;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  result = (simgrid::s4u::NetZone*)simgrid::s4u::create_vivaldi_zone((std::string const&)*arg1);
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_create_1wifi_1zone(JNIEnv* jenv, jclass jcls, jstring cthis)
{
  jlong jresult                 = 0;
  std::string* arg1             = 0;
  simgrid::s4u::NetZone* result = 0;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  result = (simgrid::s4u::NetZone*)simgrid::s4u::create_wifi_zone((std::string const&)*arg1);
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1by_1netzone_1_1set(JNIEnv* jenv, jclass jcls,
                                                                                             jlong cthis, jobject jthis,
                                                                                             jboolean jarg2)
{
  simgrid::s4u::ClusterCallbacks* arg1 = (simgrid::s4u::ClusterCallbacks*)0;
  bool arg2;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  arg2 = jarg2 ? true : false;
  if (arg1)
    (arg1)->by_netzone_ = arg2;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1by_1netzone_1_1get(JNIEnv* jenv,
                                                                                                 jclass jcls,
                                                                                                 jlong cthis,
                                                                                                 jobject jthis)
{
  jboolean jresult                     = 0;
  simgrid::s4u::ClusterCallbacks* arg1 = (simgrid::s4u::ClusterCallbacks*)0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  result  = (bool)((arg1)->by_netzone_);
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1is_1by_1netzone(JNIEnv* jenv, jclass jcls,
                                                                                              jlong cthis,
                                                                                              jobject jthis)
{
  jboolean jresult                     = 0;
  simgrid::s4u::ClusterCallbacks* arg1 = (simgrid::s4u::ClusterCallbacks*)0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  result  = (bool)((simgrid::s4u::ClusterCallbacks const*)arg1)->is_by_netzone();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1by_1netpoint_1_1set(JNIEnv* jenv, jclass jcls,
                                                                                              jlong cthis,
                                                                                              jobject jthis,
                                                                                              jboolean jarg2)
{
  simgrid::s4u::ClusterCallbacks* arg1 = (simgrid::s4u::ClusterCallbacks*)0;
  bool arg2;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  arg2 = jarg2 ? true : false;
  if (arg1)
    (arg1)->by_netpoint_ = arg2;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1by_1netpoint_1_1get(JNIEnv* jenv,
                                                                                                  jclass jcls,
                                                                                                  jlong cthis,
                                                                                                  jobject jthis)
{
  jboolean jresult                     = 0;
  simgrid::s4u::ClusterCallbacks* arg1 = (simgrid::s4u::ClusterCallbacks*)0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  result  = (bool)((arg1)->by_netpoint_);
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1is_1by_1netpoint(JNIEnv* jenv,
                                                                                               jclass jcls, jlong cthis,
                                                                                               jobject jthis)
{
  jboolean jresult                     = 0;
  simgrid::s4u::ClusterCallbacks* arg1 = (simgrid::s4u::ClusterCallbacks*)0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  result  = (bool)((simgrid::s4u::ClusterCallbacks const*)arg1)->is_by_netpoint();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1netpoint_1set(JNIEnv* jenv, jclass jcls,
                                                                                        jlong cthis, jobject jthis,
                                                                                        jlong jarg2)
{
  simgrid::s4u::ClusterCallbacks* arg1 = (simgrid::s4u::ClusterCallbacks*)0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterNetPointCb> arg2;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterNetPointCb>* argp2;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1  = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  argp2 = *(std::function<simgrid::s4u::ClusterCallbacks::ClusterNetPointCb>**)&jarg2;
  if (!argp2) {
    SWIG_JavaThrowException(
        jenv, SWIG_JavaNullPointerException,
        "Attempt to dereference null std::function< simgrid::s4u::ClusterCallbacks::ClusterNetPointCb >");
    return;
  }
  arg2 = *argp2;
  if (arg1)
    (arg1)->netpoint = arg2;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1netpoint_1get(JNIEnv* jenv, jclass jcls,
                                                                                         jlong cthis, jobject jthis)
{
  jlong jresult                        = 0;
  simgrid::s4u::ClusterCallbacks* arg1 = (simgrid::s4u::ClusterCallbacks*)0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterNetPointCb> result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1   = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  result = ((arg1)->netpoint);
  *(std::function<simgrid::s4u::ClusterCallbacks::ClusterNetPointCb>**)&jresult =
      new std::function<simgrid::s4u::ClusterCallbacks::ClusterNetPointCb>(result);
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1host_1set(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis,
                                                                                    jlong jarg2)
{
  simgrid::s4u::ClusterCallbacks* arg1 = (simgrid::s4u::ClusterCallbacks*)0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterHostCb>* arg2 =
      (std::function<simgrid::s4u::ClusterCallbacks::ClusterHostCb>*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  arg2 = *(std::function<simgrid::s4u::ClusterCallbacks::ClusterHostCb>**)&jarg2;
  if (arg1)
    (arg1)->host = *arg2;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1host_1get(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis)
{
  jlong jresult                                                        = 0;
  simgrid::s4u::ClusterCallbacks* arg1                                 = (simgrid::s4u::ClusterCallbacks*)0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterHostCb>* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1   = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  result = (std::function<simgrid::s4u::ClusterCallbacks::ClusterHostCb>*)&((arg1)->host);
  *(std::function<simgrid::s4u::ClusterCallbacks::ClusterHostCb>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1netzone_1set(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis,
                                                                                       jlong jarg2)
{
  simgrid::s4u::ClusterCallbacks* arg1 = (simgrid::s4u::ClusterCallbacks*)0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb>* arg2 =
      (std::function<simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb>*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  arg2 = *(std::function<simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb>**)&jarg2;
  if (arg1)
    (arg1)->netzone = *arg2;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1netzone_1get(JNIEnv* jenv, jclass jcls,
                                                                                        jlong cthis, jobject jthis)
{
  jlong jresult                                                           = 0;
  simgrid::s4u::ClusterCallbacks* arg1                                    = (simgrid::s4u::ClusterCallbacks*)0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb>* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1   = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  result = (std::function<simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb>*)&((arg1)->netzone);
  *(std::function<simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1loopback_1set(JNIEnv* jenv, jclass jcls,
                                                                                        jlong cthis, jobject jthis,
                                                                                        jlong jarg2)
{
  simgrid::s4u::ClusterCallbacks* arg1 = (simgrid::s4u::ClusterCallbacks*)0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>* arg2 =
      (std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  arg2 = *(std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>**)&jarg2;
  if (arg1)
    (arg1)->loopback = *arg2;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1loopback_1get(JNIEnv* jenv, jclass jcls,
                                                                                         jlong cthis, jobject jthis)
{
  jlong jresult                                                        = 0;
  simgrid::s4u::ClusterCallbacks* arg1                                 = (simgrid::s4u::ClusterCallbacks*)0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1   = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  result = (std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>*)&((arg1)->loopback);
  *(std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1limiter_1set(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis,
                                                                                       jlong jarg2)
{
  simgrid::s4u::ClusterCallbacks* arg1 = (simgrid::s4u::ClusterCallbacks*)0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>* arg2 =
      (std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  arg2 = *(std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>**)&jarg2;
  if (arg1)
    (arg1)->limiter = *arg2;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_ClusterCallbacks_1limiter_1get(JNIEnv* jenv, jclass jcls,
                                                                                        jlong cthis, jobject jthis)
{
  jlong jresult                                                        = 0;
  simgrid::s4u::ClusterCallbacks* arg1                                 = (simgrid::s4u::ClusterCallbacks*)0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1   = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  result = (std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>*)&((arg1)->limiter);
  *(std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_new_1ClusterCallbacks_1_1SWIG_10(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis)
{
  jlong jresult                                                         = 0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb>* arg1 = 0;
  simgrid::s4u::ClusterCallbacks* result                                = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb > const & is null");
    return 0;
  }
  result = (simgrid::s4u::ClusterCallbacks*)new simgrid::s4u::ClusterCallbacks(
      (std::function<simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb> const&)*arg1);
  *(simgrid::s4u::ClusterCallbacks**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_new_1ClusterCallbacks_1_1SWIG_11(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jlong jarg2,
                                                                                          jlong jarg3)
{
  jlong jresult                                                         = 0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb>* arg1 = 0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>* arg2    = 0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>* arg3    = 0;
  simgrid::s4u::ClusterCallbacks* result                                = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb > const & is null");
    return 0;
  }
  arg2 = *(std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< simgrid::s4u::ClusterCallbacks::ClusterLinkCb > const & is null");
    return 0;
  }
  arg3 = *(std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>**)&jarg3;
  if (!arg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< simgrid::s4u::ClusterCallbacks::ClusterLinkCb > const & is null");
    return 0;
  }
  result = (simgrid::s4u::ClusterCallbacks*)new simgrid::s4u::ClusterCallbacks(
      (std::function<simgrid::s4u::ClusterCallbacks::ClusterNetZoneCb> const&)*arg1,
      (std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb> const&)*arg2,
      (std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb> const&)*arg3);
  *(simgrid::s4u::ClusterCallbacks**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_new_1ClusterCallbacks_1_1SWIG_12(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis)
{
  jlong jresult                                                      = 0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterHostCb>* arg1 = 0;
  simgrid::s4u::ClusterCallbacks* result                             = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<simgrid::s4u::ClusterCallbacks::ClusterHostCb>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< simgrid::s4u::ClusterCallbacks::ClusterHostCb > const & is null");
    return 0;
  }
  result = (simgrid::s4u::ClusterCallbacks*)new simgrid::s4u::ClusterCallbacks(
      (std::function<simgrid::s4u::ClusterCallbacks::ClusterHostCb> const&)*arg1);
  *(simgrid::s4u::ClusterCallbacks**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_new_1ClusterCallbacks_1_1SWIG_13(JNIEnv* jenv, jclass jcls,
                                                                                          jlong cthis, jlong jarg2,
                                                                                          jlong jarg3)
{
  jlong jresult                                                      = 0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterHostCb>* arg1 = 0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>* arg2 = 0;
  std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>* arg3 = 0;
  simgrid::s4u::ClusterCallbacks* result                             = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<simgrid::s4u::ClusterCallbacks::ClusterHostCb>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< simgrid::s4u::ClusterCallbacks::ClusterHostCb > const & is null");
    return 0;
  }
  arg2 = *(std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< simgrid::s4u::ClusterCallbacks::ClusterLinkCb > const & is null");
    return 0;
  }
  arg3 = *(std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb>**)&jarg3;
  if (!arg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< simgrid::s4u::ClusterCallbacks::ClusterLinkCb > const & is null");
    return 0;
  }
  result = (simgrid::s4u::ClusterCallbacks*)new simgrid::s4u::ClusterCallbacks(
      (std::function<simgrid::s4u::ClusterCallbacks::ClusterHostCb> const&)*arg1,
      (std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb> const&)*arg2,
      (std::function<simgrid::s4u::ClusterCallbacks::ClusterLinkCb> const&)*arg3);
  *(simgrid::s4u::ClusterCallbacks**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1ClusterCallbacks(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  simgrid::s4u::ClusterCallbacks* arg1 = (simgrid::s4u::ClusterCallbacks*)0;

  (void)jenv;
  (void)jcls;
  arg1 = *(simgrid::s4u::ClusterCallbacks**)&cthis;
  delete arg1;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_create_1torus_1zone(JNIEnv* jenv, jclass jcls, jstring cthis,
                                                                             jlong jarg2, jobject jarg2_, jlong jarg3,
                                                                             jlong jarg4, jobject jarg4_, jdouble jarg5,
                                                                             jdouble jarg6, jint jarg7)
{
  jlong jresult                        = 0;
  std::string* arg1                    = 0;
  simgrid::s4u::NetZone* arg2          = (simgrid::s4u::NetZone*)0;
  std::vector<unsigned long>* arg3     = 0;
  simgrid::s4u::ClusterCallbacks* arg4 = 0;
  double arg5;
  double arg6;
  simgrid::s4u::Link::SharingPolicy arg7;
  simgrid::s4u::NetZone* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jarg2_;
  (void)jarg4_;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  arg2 = *(simgrid::s4u::NetZone**)&jarg2;
  arg3 = *(std::vector<unsigned long>**)&jarg3;
  if (!arg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::vector< unsigned long > const & is null");
    return 0;
  }
  arg4 = *(simgrid::s4u::ClusterCallbacks**)&jarg4;
  if (!arg4) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "simgrid::s4u::ClusterCallbacks const & is null");
    return 0;
  }
  arg5   = (double)jarg5;
  arg6   = (double)jarg6;
  arg7   = (simgrid::s4u::Link::SharingPolicy)jarg7;
  result = (simgrid::s4u::NetZone*)simgrid::s4u::create_torus_zone(
      (std::string const&)*arg1, (simgrid::s4u::NetZone const*)arg2, (std::vector<unsigned long> const&)*arg3,
      (simgrid::s4u::ClusterCallbacks const&)*arg4, arg5, arg6, arg7);
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_FatTreeParams_1levels_1set(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis,
                                                                                   jlong jarg2)
{
  simgrid::s4u::FatTreeParams* arg1 = (simgrid::s4u::FatTreeParams*)0;
  unsigned int arg2;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::FatTreeParams**)&cthis;
  arg2 = (unsigned int)jarg2;
  if (arg1)
    (arg1)->levels = arg2;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_FatTreeParams_1levels_1get(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis)
{
  jlong jresult                     = 0;
  simgrid::s4u::FatTreeParams* arg1 = (simgrid::s4u::FatTreeParams*)0;
  unsigned int result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::FatTreeParams**)&cthis;
  result  = (unsigned int)((arg1)->levels);
  jresult = (jlong)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_FatTreeParams_1down_1set(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis, jlong jarg2)
{
  simgrid::s4u::FatTreeParams* arg1 = (simgrid::s4u::FatTreeParams*)0;
  std::vector<unsigned int>* arg2   = (std::vector<unsigned int>*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::FatTreeParams**)&cthis;
  arg2 = *(std::vector<unsigned int>**)&jarg2;
  if (arg1)
    (arg1)->down = *arg2;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_FatTreeParams_1down_1get(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis)
{
  jlong jresult                     = 0;
  simgrid::s4u::FatTreeParams* arg1 = (simgrid::s4u::FatTreeParams*)0;
  std::vector<unsigned int>* result = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                                   = *(simgrid::s4u::FatTreeParams**)&cthis;
  result                                 = (std::vector<unsigned int>*)&((arg1)->down);
  *(std::vector<unsigned int>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_FatTreeParams_1up_1set(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jlong jarg2)
{
  simgrid::s4u::FatTreeParams* arg1 = (simgrid::s4u::FatTreeParams*)0;
  std::vector<unsigned int>* arg2   = (std::vector<unsigned int>*)0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::FatTreeParams**)&cthis;
  arg2 = *(std::vector<unsigned int>**)&jarg2;
  if (arg1)
    (arg1)->up = *arg2;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_FatTreeParams_1up_1get(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis)
{
  jlong jresult                     = 0;
  simgrid::s4u::FatTreeParams* arg1      = *(simgrid::s4u::FatTreeParams**)&cthis;
  std::vector<unsigned int>* result      = (std::vector<unsigned int>*)&((arg1)->up);
  *(std::vector<unsigned int>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_FatTreeParams_1number_1set(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis,
                                                                                   jlong jarg2)
{
  simgrid::s4u::FatTreeParams* arg1 = *(simgrid::s4u::FatTreeParams**)&cthis;
  std::vector<unsigned int>* arg2   = *(std::vector<unsigned int>**)&jarg2;
  if (arg1)
    (arg1)->number = *arg2;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_FatTreeParams_1number_1get(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis)
{
  jlong jresult                     = 0;
  simgrid::s4u::FatTreeParams* arg1      = *(simgrid::s4u::FatTreeParams**)&cthis;
  std::vector<unsigned int>* result      = (std::vector<unsigned int>*)&((arg1)->number);
  *(std::vector<unsigned int>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_new_1FatTreeParams(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jlong jarg2, jlong jarg3, jlong jarg4)
{
  jlong jresult = 0;
  unsigned int arg1;
  std::vector<unsigned int>* arg2     = 0;
  std::vector<unsigned int>* arg3     = 0;
  std::vector<unsigned int>* arg4     = 0;
  simgrid::s4u::FatTreeParams* result = 0;

  arg1 = (unsigned int)cthis;
  arg2 = *(std::vector<unsigned int>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::vector< unsigned int > const & is null");
    return 0;
  }
  arg3 = *(std::vector<unsigned int>**)&jarg3;
  if (!arg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::vector< unsigned int > const & is null");
    return 0;
  }
  arg4 = *(std::vector<unsigned int>**)&jarg4;
  if (!arg4) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "std::vector< unsigned int > const & is null");
    return 0;
  }
  result = (simgrid::s4u::FatTreeParams*)new simgrid::s4u::FatTreeParams(arg1, (std::vector<unsigned int> const&)*arg2,
                                                                         (std::vector<unsigned int> const&)*arg3,
                                                                         (std::vector<unsigned int> const&)*arg4);
  *(simgrid::s4u::FatTreeParams**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1FatTreeParams(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  auto arg1 = *(simgrid::s4u::FatTreeParams**)&cthis;
  delete arg1;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_create_1fatTree_1zone(JNIEnv* jenv, jclass jcls, jstring cthis,
                                                                               jlong jarg2, jobject jarg2_, jlong jarg3,
                                                                               jobject jarg3_, jlong jarg4,
                                                                               jobject jarg4_, jdouble jarg5,
                                                                               jdouble jarg6, jint jarg7)
{
  jlong jresult                        = 0;
  std::string* arg1                    = 0;
  simgrid::s4u::NetZone* arg2          = (simgrid::s4u::NetZone*)0;
  simgrid::s4u::FatTreeParams* arg3    = 0;
  simgrid::s4u::ClusterCallbacks* arg4 = 0;
  double arg5;
  double arg6;
  simgrid::s4u::Link::SharingPolicy arg7;
  simgrid::s4u::NetZone* result = 0;

  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  arg2 = *(simgrid::s4u::NetZone**)&jarg2;
  arg3 = *(simgrid::s4u::FatTreeParams**)&jarg3;
  if (!arg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "simgrid::s4u::FatTreeParams const & is null");
    return 0;
  }
  arg4 = *(simgrid::s4u::ClusterCallbacks**)&jarg4;
  if (!arg4) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "simgrid::s4u::ClusterCallbacks const & is null");
    return 0;
  }
  arg5   = (double)jarg5;
  arg6   = (double)jarg6;
  arg7   = (simgrid::s4u::Link::SharingPolicy)jarg7;
  result = (simgrid::s4u::NetZone*)simgrid::s4u::create_fatTree_zone(
      (std::string const&)*arg1, (simgrid::s4u::NetZone const*)arg2, (simgrid::s4u::FatTreeParams const&)*arg3,
      (simgrid::s4u::ClusterCallbacks const&)*arg4, arg5, arg6, arg7);
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_DragonflyParams_1groups_1set(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis,
                                                                                     jlong jarg2)
{
  simgrid::s4u::DragonflyParams* arg1         = (simgrid::s4u::DragonflyParams*)0;
  std::pair<unsigned int, unsigned int>* arg2 = (std::pair<unsigned int, unsigned int>*)0;

  arg1 = *(simgrid::s4u::DragonflyParams**)&cthis;
  arg2 = *(std::pair<unsigned int, unsigned int>**)&jarg2;
  if (arg1)
    (arg1)->groups = *arg2;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_DragonflyParams_1groups_1get(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis)
{
  jlong jresult                                 = 0;
  simgrid::s4u::DragonflyParams* arg1           = (simgrid::s4u::DragonflyParams*)0;
  std::pair<unsigned int, unsigned int>* result = 0;

  arg1                                               = *(simgrid::s4u::DragonflyParams**)&cthis;
  result                                             = (std::pair<unsigned int, unsigned int>*)&((arg1)->groups);
  *(std::pair<unsigned int, unsigned int>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_DragonflyParams_1chassis_1set(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis,
                                                                                      jlong jarg2)
{
  simgrid::s4u::DragonflyParams* arg1         = (simgrid::s4u::DragonflyParams*)0;
  std::pair<unsigned int, unsigned int>* arg2 = (std::pair<unsigned int, unsigned int>*)0;

  arg1 = *(simgrid::s4u::DragonflyParams**)&cthis;
  arg2 = *(std::pair<unsigned int, unsigned int>**)&jarg2;
  if (arg1)
    (arg1)->chassis = *arg2;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_DragonflyParams_1chassis_1get(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis)
{
  jlong jresult                                 = 0;
  simgrid::s4u::DragonflyParams* arg1           = (simgrid::s4u::DragonflyParams*)0;
  std::pair<unsigned int, unsigned int>* result = 0;

  arg1                                               = *(simgrid::s4u::DragonflyParams**)&cthis;
  result                                             = (std::pair<unsigned int, unsigned int>*)&((arg1)->chassis);
  *(std::pair<unsigned int, unsigned int>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_DragonflyParams_1routers_1set(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis,
                                                                                      jlong jarg2)
{
  simgrid::s4u::DragonflyParams* arg1         = (simgrid::s4u::DragonflyParams*)0;
  std::pair<unsigned int, unsigned int>* arg2 = (std::pair<unsigned int, unsigned int>*)0;

  arg1 = *(simgrid::s4u::DragonflyParams**)&cthis;
  arg2 = *(std::pair<unsigned int, unsigned int>**)&jarg2;
  if (arg1)
    (arg1)->routers = *arg2;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_DragonflyParams_1routers_1get(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis)
{
  jlong jresult                                 = 0;
  simgrid::s4u::DragonflyParams* arg1           = (simgrid::s4u::DragonflyParams*)0;
  std::pair<unsigned int, unsigned int>* result = 0;

  arg1                                               = *(simgrid::s4u::DragonflyParams**)&cthis;
  result                                             = (std::pair<unsigned int, unsigned int>*)&((arg1)->routers);
  *(std::pair<unsigned int, unsigned int>**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_DragonflyParams_1nodes_1set(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis,
                                                                                    jlong jarg2)
{
  simgrid::s4u::DragonflyParams* arg1 = (simgrid::s4u::DragonflyParams*)0;
  unsigned int arg2;

  arg1 = *(simgrid::s4u::DragonflyParams**)&cthis;
  arg2 = (unsigned int)jarg2;
  if (arg1)
    (arg1)->nodes = arg2;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_DragonflyParams_1nodes_1get(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis)
{
  jlong jresult                       = 0;
  simgrid::s4u::DragonflyParams* arg1 = (simgrid::s4u::DragonflyParams*)0;
  unsigned int result;

  arg1    = *(simgrid::s4u::DragonflyParams**)&cthis;
  result  = (unsigned int)((arg1)->nodes);
  jresult = (jlong)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_new_1DragonflyParams(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jlong jarg2, jlong jarg3, jlong jarg4)
{
  jlong jresult                               = 0;
  std::pair<unsigned int, unsigned int>* arg1 = 0;
  std::pair<unsigned int, unsigned int>* arg2 = 0;
  std::pair<unsigned int, unsigned int>* arg3 = 0;
  unsigned int arg4;
  simgrid::s4u::DragonflyParams* result = 0;
  arg1                                  = *(std::pair<unsigned int, unsigned int>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::pair< unsigned int,unsigned int > const & is null");
    return 0;
  }
  arg2 = *(std::pair<unsigned int, unsigned int>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::pair< unsigned int,unsigned int > const & is null");
    return 0;
  }
  arg3 = *(std::pair<unsigned int, unsigned int>**)&jarg3;
  if (!arg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::pair< unsigned int,unsigned int > const & is null");
    return 0;
  }
  arg4   = (unsigned int)jarg4;
  result = (simgrid::s4u::DragonflyParams*)new simgrid::s4u::DragonflyParams(
      (std::pair<unsigned int, unsigned int> const&)*arg1, (std::pair<unsigned int, unsigned int> const&)*arg2,
      (std::pair<unsigned int, unsigned int> const&)*arg3, arg4);
  *(simgrid::s4u::DragonflyParams**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1DragonflyParams(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  simgrid::s4u::DragonflyParams* arg1 = (simgrid::s4u::DragonflyParams*)0;

  arg1 = *(simgrid::s4u::DragonflyParams**)&cthis;
  delete arg1;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_create_1dragonfly_1zone(
    JNIEnv* jenv, jclass jcls, jstring cthis, jlong jarg2, jobject jarg2_, jlong jarg3, jobject jarg3_, jlong jarg4,
    jobject jarg4_, jdouble jarg5, jdouble jarg6, jint jarg7)
{
  jlong jresult                        = 0;
  std::string* arg1                    = 0;
  simgrid::s4u::NetZone* arg2          = (simgrid::s4u::NetZone*)0;
  simgrid::s4u::DragonflyParams* arg3  = 0;
  simgrid::s4u::ClusterCallbacks* arg4 = 0;
  double arg5;
  double arg6;
  simgrid::s4u::Link::SharingPolicy arg7;
  simgrid::s4u::NetZone* result = 0;

  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  arg2 = *(simgrid::s4u::NetZone**)&jarg2;
  arg3 = *(simgrid::s4u::DragonflyParams**)&jarg3;
  if (!arg3) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "simgrid::s4u::DragonflyParams const & is null");
    return 0;
  }
  arg4 = *(simgrid::s4u::ClusterCallbacks**)&jarg4;
  if (!arg4) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "simgrid::s4u::ClusterCallbacks const & is null");
    return 0;
  }
  arg5   = (double)jarg5;
  arg6   = (double)jarg6;
  arg7   = (simgrid::s4u::Link::SharingPolicy)jarg7;
  result = (simgrid::s4u::NetZone*)simgrid::s4u::create_dragonfly_zone(
      (std::string const&)*arg1, (simgrid::s4u::NetZone const*)arg2, (simgrid::s4u::DragonflyParams const&)*arg3,
      (simgrid::s4u::ClusterCallbacks const&)*arg4, arg5, arg6, arg7);
  *(simgrid::s4u::NetZone**)&jresult = result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1Mailbox(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  boost::shared_ptr<simgrid::s4u::Mailbox>* smartarg1 = *(boost::shared_ptr<simgrid::s4u::Mailbox>**)&cthis;
  delete smartarg1;
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1get_1name(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis)
{
  auto self          = (Mailbox*)cthis;
  const char* result = self->get_cname();
  if (result)
    return jenv->NewStringUTF((const char*)result);
  return 0;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1by_1name(JNIEnv* jenv, jclass jcls, jstring jname)
{
  if (!jname) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* cname = (const char*)jenv->GetStringUTFChars(jname, 0);
  Mailbox* result   = simgrid::s4u::Mailbox::by_name(cname);
  jenv->ReleaseStringUTFChars(jname, cname);

  return (jlong)result;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1empty(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  return ((Mailbox*)cthis)->empty();
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1size(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                       jobject jthis)
{
  return ((Mailbox*)cthis)->empty();
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1listen(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jobject jthis)
{
  return ((Mailbox*)cthis)->listen();
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1listen_1from(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis)
{
  return ((Mailbox*)cthis)->listen_from();
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1ready(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  return ((Mailbox*)cthis)->ready();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1set_1receiver(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis, jlong cactor,
                                                                               jobject jactor)
{
  return ((Mailbox*)cthis)->set_receiver((Actor*)cactor);
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1get_1receiver(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis)
{
  auto res = ((Mailbox*)cthis)->get_receiver();
  intrusive_ptr_add_ref(res.get());

  return (jlong)res.get();
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1put_1init_1_1SWIG_10(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis)
{
  auto self = (Mailbox*)cthis;

  CommPtr result = self->put_init();
  intrusive_ptr_add_ref(result.get());

  return (jlong)result.get();
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1put_1init_1_1SWIG_11(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis,
                                                                                       jobject payload,
                                                                                       jlong simulated_size_in_bytes)
{
  auto self = (Mailbox*)cthis;
  auto glob = jenv->NewGlobalRef(payload);

  CommPtr result = self->put_init(glob, simulated_size_in_bytes);
  intrusive_ptr_add_ref(result.get());

  return (jlong)result.get();
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1put_1async(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis, jobject payload,
                                                                             jlong simulated_size_in_bytes)
{
  auto self = (Mailbox*)cthis;

  CommPtr result = self->put_async(jenv->NewGlobalRef(payload), simulated_size_in_bytes);
  intrusive_ptr_add_ref(result.get());
  return (jlong)result.get();
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1to_1c_1str(JNIEnv* jenv, jclass jcls, jint cthis)
{
  auto arg1                              = (simgrid::s4u::Mailbox::IprobeKind)cthis;
  char* result                           = (char*)simgrid::s4u::Mailbox::to_c_str(arg1);
  if (result)
    return jenv->NewStringUTF((const char*)result);
  return 0;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1get_1init(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jobject jthis)
{
  auto self = (Mailbox*)cthis;

  CommPtr result = self->get_init();
  intrusive_ptr_add_ref(result.get());
  return (jlong)result.get();
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1get_1async(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis)
{
  auto self = (Mailbox*)cthis;

  CommPtr result = self->get_async();
  intrusive_ptr_add_ref(result.get());
  return (jlong)result.get();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1clear(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                       jobject jthis)
{
  ((Mailbox*)cthis)->clear();
}
XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1put_1_1SWIG_10(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis, jobject payload,
                                                                                jlong simulated_size_in_bytes)
{
  auto self = (Mailbox*)cthis;
  self->put(jenv->NewGlobalRef(payload), (uint64_t)simulated_size_in_bytes);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1put_1_1SWIG_11(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis, jobject payload,
                                                                                jlong simulated_size_in_bytes,
                                                                                jdouble timeout)
{
  auto self = (Mailbox*)cthis;
  self->put(jenv->NewGlobalRef(payload), simulated_size_in_bytes, timeout);
}

XBT_PUBLIC jobject JNICALL Java_org_simgrid_s4u_simgridJNI_Mailbox_1get(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                        jobject jthis)
{
  auto self    = (Mailbox*)cthis;
  jobject glob = self->get<_jobject>();
  auto local   = jenv->NewLocalRef(glob);
  jenv->DeleteGlobalRef(glob);

  return local;
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_MessageQueue_1get_1name(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis)
{
  jstring jresult                  = 0;
  simgrid::s4u::MessageQueue* arg1 = (simgrid::s4u::MessageQueue*)0;
  char* result                     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1   = *(simgrid::s4u::MessageQueue**)&cthis;
  result = (char*)((simgrid::s4u::MessageQueue const*)arg1)->get_cname();
  if (result)
    jresult = jenv->NewStringUTF((const char*)result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_MessageQueue_1by_1name(JNIEnv* jenv, jclass jcls,
                                                                                jstring cthis)
{
  jlong jresult                      = 0;
  std::string* arg1                  = 0;
  simgrid::s4u::MessageQueue* result = 0;

  (void)jenv;
  (void)jcls;
  if (!cthis) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
    return 0;
  }
  const char* arg1_pstr = (const char*)jenv->GetStringUTFChars(cthis, 0);
  if (!arg1_pstr)
    return 0;
  std::string arg1_str(arg1_pstr);
  arg1 = &arg1_str;
  jenv->ReleaseStringUTFChars(cthis, arg1_pstr);
  result = (simgrid::s4u::MessageQueue*)simgrid::s4u::MessageQueue::by_name((std::string const&)*arg1);
  *(simgrid::s4u::MessageQueue**)&jresult = result;
  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_MessageQueue_1empty(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis)
{
  jboolean jresult                 = 0;
  simgrid::s4u::MessageQueue* arg1 = (simgrid::s4u::MessageQueue*)0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::MessageQueue**)&cthis;
  result  = (bool)((simgrid::s4u::MessageQueue const*)arg1)->empty();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_MessageQueue_1size(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                            jobject jthis)
{
  jlong jresult                    = 0;
  simgrid::s4u::MessageQueue* arg1 = (simgrid::s4u::MessageQueue*)0;
  size_t result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1    = *(simgrid::s4u::MessageQueue**)&cthis;
  result  = ((simgrid::s4u::MessageQueue const*)arg1)->size();
  jresult = (jlong)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_MessageQueue_1put_1init_1_1SWIG_10(JNIEnv* jenv, jclass jcls,
                                                                                            jlong cthis, jobject jthis)
{
  jlong jresult                    = 0;
  simgrid::s4u::MessageQueue* arg1 = (simgrid::s4u::MessageQueue*)0;
  MessPtr result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                 = *(simgrid::s4u::MessageQueue**)&cthis;
  result               = (arg1)->put_init();
  *(MessPtr**)&jresult = new MessPtr(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_MessageQueue_1put_1init_1_1SWIG_11(JNIEnv* jenv, jclass jcls,
                                                                                            jlong cthis, jobject jthis,
                                                                                            jlong jarg2)
{
  jlong jresult                    = 0;
  simgrid::s4u::MessageQueue* arg1 = (simgrid::s4u::MessageQueue*)0;
  void* arg2                       = (void*)0;
  MessPtr result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::MessageQueue**)&cthis;
  {
    // From JNI to C++
    arg2 = (void*)jarg2;
  }
  result               = (arg1)->put_init(arg2);
  *(MessPtr**)&jresult = new MessPtr(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_MessageQueue_1put_1async(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis,
                                                                                  jlong jarg2)
{
  jlong jresult                    = 0;
  simgrid::s4u::MessageQueue* arg1 = (simgrid::s4u::MessageQueue*)0;
  void* arg2                       = (void*)0;
  MessPtr result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1 = *(simgrid::s4u::MessageQueue**)&cthis;
  {
    // From JNI to C++
    arg2 = (void*)jarg2;
  }
  result               = (arg1)->put_async(arg2);
  *(MessPtr**)&jresult = new MessPtr(result);
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_MessageQueue_1put_1_1SWIG_10(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis,
                                                                                     jlong jarg2)
{
  ((simgrid::s4u::MessageQueue*)cthis)->put((void*)jarg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_MessageQueue_1put_1_1SWIG_11(JNIEnv* jenv, jclass jcls,
                                                                                     jlong cthis, jobject jthis,
                                                                                     jlong jarg2, jdouble jarg3)
{
  ((simgrid::s4u::MessageQueue*)cthis)->put((void*)jarg2, jarg3);
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_MessageQueue_1get_1init(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis)
{
  jlong jresult                    = 0;
  simgrid::s4u::MessageQueue* arg1 = (simgrid::s4u::MessageQueue*)0;
  MessPtr result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                 = *(simgrid::s4u::MessageQueue**)&cthis;
  result               = (arg1)->get_init();
  *(MessPtr**)&jresult = new MessPtr(result);
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_MessageQueue_1get_1async(JNIEnv* jenv, jclass jcls,
                                                                                  jlong cthis, jobject jthis)
{
  jlong jresult                    = 0;
  simgrid::s4u::MessageQueue* arg1 = (simgrid::s4u::MessageQueue*)0;
  MessPtr result;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  arg1                 = *(simgrid::s4u::MessageQueue**)&cthis;
  result               = (arg1)->get_async();
  *(MessPtr**)&jresult = new MessPtr(result);
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1Mutex(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  simgrid::s4u::Mutex* arg1                         = (simgrid::s4u::Mutex*)0;
  boost::shared_ptr<simgrid::s4u::Mutex>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Mutex>**)&cthis;
  arg1      = (simgrid::s4u::Mutex*)(smartarg1 ? smartarg1->get() : 0);

  (void)arg1;
  delete smartarg1;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Mutex_1create_1_1SWIG_10(JNIEnv* jenv, jclass jcls,
                                                                                  jboolean cthis)
{
  jlong jresult = 0;
  bool arg1;
  boost::intrusive_ptr<simgrid::s4u::Mutex> result;

  (void)jenv;
  (void)jcls;
  arg1   = cthis ? true : false;
  result = simgrid::s4u::Mutex::create(arg1);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Mutex>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Mutex>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Mutex>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Mutex>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Mutex_1create_1_1SWIG_11(JNIEnv* jenv, jclass jcls)
{
  jlong jresult = 0;
  boost::intrusive_ptr<simgrid::s4u::Mutex> result;

  (void)jenv;
  (void)jcls;
  result = simgrid::s4u::Mutex::create();

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Mutex>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Mutex>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Mutex>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Mutex>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Mutex_1lock(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                    jobject jthis)
{
  simgrid::s4u::Mutex* arg1                         = (simgrid::s4u::Mutex*)0;
  boost::shared_ptr<simgrid::s4u::Mutex>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Mutex>**)&cthis;
  arg1      = (simgrid::s4u::Mutex*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->lock();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Mutex_1unlock(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                      jobject jthis)
{
  simgrid::s4u::Mutex* arg1                         = (simgrid::s4u::Mutex*)0;
  boost::shared_ptr<simgrid::s4u::Mutex>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Mutex>**)&cthis;
  arg1      = (simgrid::s4u::Mutex*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->unlock();
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Mutex_1try_1lock(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                             jobject jthis)
{
  jboolean jresult                                  = 0;
  simgrid::s4u::Mutex* arg1                         = (simgrid::s4u::Mutex*)0;
  boost::shared_ptr<simgrid::s4u::Mutex>* smartarg1 = 0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Mutex>**)&cthis;
  arg1      = (simgrid::s4u::Mutex*)(smartarg1 ? smartarg1->get() : 0);

  result  = (bool)(arg1)->try_lock();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Mutex_1get_1owner(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  jlong jresult                                     = 0;
  simgrid::s4u::Mutex* arg1                         = (simgrid::s4u::Mutex*)0;
  boost::shared_ptr<simgrid::s4u::Mutex>* smartarg1 = 0;
  simgrid::s4u::Actor* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Mutex>**)&cthis;
  arg1      = (simgrid::s4u::Mutex*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Actor*)(arg1)->get_owner();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Actor>(result, SWIG_intrusive_deleter<simgrid::s4u::Actor>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Actor>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Actor>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1Semaphore(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  simgrid::s4u::Semaphore* arg1                         = (simgrid::s4u::Semaphore*)0;
  boost::shared_ptr<simgrid::s4u::Semaphore>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Semaphore>**)&cthis;
  arg1      = (simgrid::s4u::Semaphore*)(smartarg1 ? smartarg1->get() : 0);

  (void)arg1;
  delete smartarg1;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_Semaphore_1create(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  jlong jresult = 0;
  unsigned int arg1;
  boost::intrusive_ptr<simgrid::s4u::Semaphore> result;

  (void)jenv;
  (void)jcls;
  arg1   = (unsigned int)cthis;
  result = simgrid::s4u::Semaphore::create(arg1);

  if (result) {
    intrusive_ptr_add_ref(result.get());
    *(boost::shared_ptr<simgrid::s4u::Semaphore>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Semaphore>(result.get(), SWIG_intrusive_deleter<simgrid::s4u::Semaphore>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Semaphore>**)&jresult = 0;
  }

  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Semaphore_1acquire(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  simgrid::s4u::Semaphore* arg1                         = (simgrid::s4u::Semaphore*)0;
  boost::shared_ptr<simgrid::s4u::Semaphore>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Semaphore>**)&cthis;
  arg1      = (simgrid::s4u::Semaphore*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->acquire();
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Semaphore_1acquire_1timeout(JNIEnv* jenv, jclass jcls,
                                                                                        jlong cthis, jobject jthis,
                                                                                        jdouble jarg2)
{
  jboolean jresult              = 0;
  simgrid::s4u::Semaphore* arg1 = (simgrid::s4u::Semaphore*)0;
  double arg2;
  boost::shared_ptr<simgrid::s4u::Semaphore>* smartarg1 = 0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Semaphore>**)&cthis;
  arg1      = (simgrid::s4u::Semaphore*)(smartarg1 ? smartarg1->get() : 0);

  arg2    = (double)jarg2;
  result  = (bool)(arg1)->acquire_timeout(arg2);
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_Semaphore_1release(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                           jobject jthis)
{
  simgrid::s4u::Semaphore* arg1                         = (simgrid::s4u::Semaphore*)0;
  boost::shared_ptr<simgrid::s4u::Semaphore>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::Semaphore>**)&cthis;
  arg1      = (simgrid::s4u::Semaphore*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->release();
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_Semaphore_1get_1capacity(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis)
{
  jint jresult                                                = 0;
  simgrid::s4u::Semaphore* arg1                               = (simgrid::s4u::Semaphore*)0;
  boost::shared_ptr<simgrid::s4u::Semaphore const>* smartarg1 = 0;
  int result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Semaphore>**)&cthis;
  arg1      = (simgrid::s4u::Semaphore*)(smartarg1 ? smartarg1->get() : 0);

  result  = (int)((simgrid::s4u::Semaphore const*)arg1)->get_capacity();
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC jboolean JNICALL Java_org_simgrid_s4u_simgridJNI_Semaphore_1would_1block(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis)
{
  jboolean jresult                                            = 0;
  simgrid::s4u::Semaphore* arg1                               = (simgrid::s4u::Semaphore*)0;
  boost::shared_ptr<simgrid::s4u::Semaphore const>* smartarg1 = 0;
  bool result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::Semaphore>**)&cthis;
  arg1      = (simgrid::s4u::Semaphore*)(smartarg1 ? smartarg1->get() : 0);

  result  = (bool)((simgrid::s4u::Semaphore const*)arg1)->would_block();
  jresult = (jboolean)result;
  return jresult;
}

XBT_PUBLIC jstring JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1to_1c_1str(JNIEnv* jenv, jclass jcls,
                                                                                      jint cthis)
{
  jstring jresult = 0;
  simgrid::s4u::VirtualMachine::State arg1;
  char* result = 0;

  (void)jenv;
  (void)jcls;
  arg1   = (simgrid::s4u::VirtualMachine::State)cthis;
  result = (char*)simgrid::s4u::VirtualMachine::to_c_str(arg1);
  if (result)
    jresult = jenv->NewStringUTF((const char*)result);
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1start(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                              jobject jthis)
{
  simgrid::s4u::VirtualMachine* arg1                         = (simgrid::s4u::VirtualMachine*)0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->start();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1suspend(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis)
{
  simgrid::s4u::VirtualMachine* arg1                         = (simgrid::s4u::VirtualMachine*)0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->suspend();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1resume(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                               jobject jthis)
{
  simgrid::s4u::VirtualMachine* arg1                         = (simgrid::s4u::VirtualMachine*)0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->resume();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1shutdown(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis)
{
  simgrid::s4u::VirtualMachine* arg1                         = (simgrid::s4u::VirtualMachine*)0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->shutdown();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1destroy(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                jobject jthis)
{
  simgrid::s4u::VirtualMachine* arg1                         = (simgrid::s4u::VirtualMachine*)0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  (arg1)->destroy();
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1get_1pm(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis)
{
  jlong jresult                                                    = 0;
  simgrid::s4u::VirtualMachine* arg1                               = (simgrid::s4u::VirtualMachine*)0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine const>* smartarg1 = 0;
  simgrid::s4u::Host* result                                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  result = (simgrid::s4u::Host*)((simgrid::s4u::VirtualMachine const*)arg1)->get_pm();

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
        new boost::shared_ptr<simgrid::s4u::Host>(result, SWIG_intrusive_deleter<simgrid::s4u::Host>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::Host>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::Host>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1set_1pm(JNIEnv* jenv, jclass jcls, jlong cthis,
                                                                                 jobject jthis, jlong jarg2,
                                                                                 jobject jarg2_)
{
  jlong jresult                                              = 0;
  simgrid::s4u::VirtualMachine* arg1                         = (simgrid::s4u::VirtualMachine*)0;
  simgrid::s4u::Host* arg2                                   = (simgrid::s4u::Host*)0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1 = 0;
  boost::shared_ptr<simgrid::s4u::Host>* smartarg2           = 0;
  simgrid::s4u::VirtualMachine* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;
  (void)jarg2_;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  // plain pointer
  smartarg2 = *(boost::shared_ptr<simgrid::s4u::Host>**)&jarg2;
  arg2      = (simgrid::s4u::Host*)(smartarg2 ? smartarg2->get() : 0);

  result = (simgrid::s4u::VirtualMachine*)(arg1)->set_pm(arg2);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&jresult = new boost::shared_ptr<simgrid::s4u::VirtualMachine>(
        result, SWIG_intrusive_deleter<simgrid::s4u::VirtualMachine>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::VirtualMachine>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1get_1ramsize(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis)
{
  jlong jresult                                                    = 0;
  simgrid::s4u::VirtualMachine* arg1                               = (simgrid::s4u::VirtualMachine*)0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine const>* smartarg1 = 0;
  size_t result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  result  = ((simgrid::s4u::VirtualMachine const*)arg1)->get_ramsize();
  jresult = (jlong)result;
  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1set_1ramsize(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis, jobject jthis,
                                                                                      jlong jarg2)
{
  jlong jresult                      = 0;
  simgrid::s4u::VirtualMachine* arg1 = (simgrid::s4u::VirtualMachine*)0;
  size_t arg2;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1 = 0;
  simgrid::s4u::VirtualMachine* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = (size_t)jarg2;
  result = (simgrid::s4u::VirtualMachine*)(arg1)->set_ramsize(arg2);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&jresult = new boost::shared_ptr<simgrid::s4u::VirtualMachine>(
        result, SWIG_intrusive_deleter<simgrid::s4u::VirtualMachine>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::VirtualMachine>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC jlong JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1set_1bound(JNIEnv* jenv, jclass jcls,
                                                                                    jlong cthis, jobject jthis,
                                                                                    jdouble jarg2)
{
  jlong jresult                      = 0;
  simgrid::s4u::VirtualMachine* arg1 = (simgrid::s4u::VirtualMachine*)0;
  double arg2;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1 = 0;
  simgrid::s4u::VirtualMachine* result                       = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  arg2   = (double)jarg2;
  result = (simgrid::s4u::VirtualMachine*)(arg1)->set_bound(arg2);

  // plain pointer(out)
#if (0)
  if (result) {
    intrusive_ptr_add_ref(result);
    *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&jresult = new boost::shared_ptr<simgrid::s4u::VirtualMachine>(
        result, SWIG_intrusive_deleter<simgrid::s4u::VirtualMachine>());
  } else {
    *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&jresult = 0;
  }
#else
  *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&jresult =
      result ? new boost::shared_ptr<simgrid::s4u::VirtualMachine>(result SWIG_NO_NULL_DELETER_0) : 0;
#endif

  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1start_1migration(JNIEnv* jenv, jclass jcls,
                                                                                         jlong cthis, jobject jthis)
{
  simgrid::s4u::VirtualMachine* arg1                               = (simgrid::s4u::VirtualMachine*)0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine const>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  ((simgrid::s4u::VirtualMachine const*)arg1)->start_migration();
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1end_1migration(JNIEnv* jenv, jclass jcls,
                                                                                       jlong cthis, jobject jthis)
{
  simgrid::s4u::VirtualMachine* arg1                               = (simgrid::s4u::VirtualMachine*)0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine const>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  ((simgrid::s4u::VirtualMachine const*)arg1)->end_migration();
}

XBT_PUBLIC jint JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1get_1state(JNIEnv* jenv, jclass jcls,
                                                                                   jlong cthis, jobject jthis)
{
  jint jresult                                                     = 0;
  simgrid::s4u::VirtualMachine* arg1                               = (simgrid::s4u::VirtualMachine*)0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine const>* smartarg1 = 0;
  simgrid::s4u::VirtualMachine::State result;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<const simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  result  = (simgrid::s4u::VirtualMachine::State)((simgrid::s4u::VirtualMachine const*)arg1)->get_state();
  jresult = (jint)result;
  return jresult;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1on_1start_1cb(JNIEnv* jenv, jclass jcls,
                                                                                      jlong cthis)
{
  std::function<void(simgrid::s4u::VirtualMachine const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::VirtualMachine const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::VirtualMachine const &) > const & is null");
    return;
  }
  simgrid::s4u::VirtualMachine::on_start_cb((std::function<void(simgrid::s4u::VirtualMachine const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1on_1this_1start_1cb(JNIEnv* jenv, jclass jcls,
                                                                                            jlong cthis, jobject jthis,
                                                                                            jlong jarg2)
{
  simgrid::s4u::VirtualMachine* arg1                             = (simgrid::s4u::VirtualMachine*)0;
  std::function<void(simgrid::s4u::VirtualMachine const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::VirtualMachine const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::VirtualMachine const &) > const & is null");
    return;
  }
  (arg1)->on_this_start_cb((std::function<void(simgrid::s4u::VirtualMachine const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1on_1started_1cb(JNIEnv* jenv, jclass jcls,
                                                                                        jlong cthis)
{
  std::function<void(simgrid::s4u::VirtualMachine const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::VirtualMachine const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::VirtualMachine const &) > const & is null");
    return;
  }
  simgrid::s4u::VirtualMachine::on_started_cb((std::function<void(simgrid::s4u::VirtualMachine const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1on_1this_1started_1cb(JNIEnv* jenv, jclass jcls,
                                                                                              jlong cthis,
                                                                                              jobject jthis,
                                                                                              jlong jarg2)
{
  simgrid::s4u::VirtualMachine* arg1                             = (simgrid::s4u::VirtualMachine*)0;
  std::function<void(simgrid::s4u::VirtualMachine const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::VirtualMachine const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::VirtualMachine const &) > const & is null");
    return;
  }
  (arg1)->on_this_started_cb((std::function<void(simgrid::s4u::VirtualMachine const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1on_1shutdown_1cb(JNIEnv* jenv, jclass jcls,
                                                                                         jlong cthis)
{
  std::function<void(simgrid::s4u::VirtualMachine const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::VirtualMachine const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::VirtualMachine const &) > const & is null");
    return;
  }
  simgrid::s4u::VirtualMachine::on_shutdown_cb((std::function<void(simgrid::s4u::VirtualMachine const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1on_1this_1shutdown_1cb(JNIEnv* jenv,
                                                                                               jclass jcls, jlong cthis,
                                                                                               jobject jthis,
                                                                                               jlong jarg2)
{
  simgrid::s4u::VirtualMachine* arg1                             = (simgrid::s4u::VirtualMachine*)0;
  std::function<void(simgrid::s4u::VirtualMachine const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::VirtualMachine const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::VirtualMachine const &) > const & is null");
    return;
  }
  (arg1)->on_this_shutdown_cb((std::function<void(simgrid::s4u::VirtualMachine const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1on_1this_1suspend_1cb(JNIEnv* jenv, jclass jcls,
                                                                                              jlong cthis,
                                                                                              jobject jthis,
                                                                                              jlong jarg2)
{
  simgrid::s4u::VirtualMachine* arg1                             = (simgrid::s4u::VirtualMachine*)0;
  std::function<void(simgrid::s4u::VirtualMachine const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::VirtualMachine const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::VirtualMachine const &) > const & is null");
    return;
  }
  (arg1)->on_this_suspend_cb((std::function<void(simgrid::s4u::VirtualMachine const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1on_1this_1resume_1cb(JNIEnv* jenv, jclass jcls,
                                                                                             jlong cthis, jobject jthis,
                                                                                             jlong jarg2)
{
  simgrid::s4u::VirtualMachine* arg1                             = (simgrid::s4u::VirtualMachine*)0;
  std::function<void(simgrid::s4u::VirtualMachine const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::VirtualMachine const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::VirtualMachine const &) > const & is null");
    return;
  }
  (arg1)->on_this_resume_cb((std::function<void(simgrid::s4u::VirtualMachine const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1on_1destruction_1cb(JNIEnv* jenv, jclass jcls,
                                                                                            jlong cthis)
{
  std::function<void(simgrid::s4u::VirtualMachine const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::VirtualMachine const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::VirtualMachine const &) > const & is null");
    return;
  }
  simgrid::s4u::VirtualMachine::on_destruction_cb(
      (std::function<void(simgrid::s4u::VirtualMachine const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1on_1this_1destruction_1cb(
    JNIEnv* jenv, jclass jcls, jlong cthis, jobject jthis, jlong jarg2)
{
  simgrid::s4u::VirtualMachine* arg1                             = (simgrid::s4u::VirtualMachine*)0;
  std::function<void(simgrid::s4u::VirtualMachine const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::VirtualMachine const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::VirtualMachine const &) > const & is null");
    return;
  }
  (arg1)->on_this_destruction_cb((std::function<void(simgrid::s4u::VirtualMachine const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1on_1migration_1start_1cb(JNIEnv* jenv,
                                                                                                 jclass jcls,
                                                                                                 jlong cthis)
{
  std::function<void(simgrid::s4u::VirtualMachine const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::VirtualMachine const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::VirtualMachine const &) > const & is null");
    return;
  }
  simgrid::s4u::VirtualMachine::on_migration_start_cb(
      (std::function<void(simgrid::s4u::VirtualMachine const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1on_1this_1migration_1start_1cb(
    JNIEnv* jenv, jclass jcls, jlong cthis, jobject jthis, jlong jarg2)
{
  simgrid::s4u::VirtualMachine* arg1                             = (simgrid::s4u::VirtualMachine*)0;
  std::function<void(simgrid::s4u::VirtualMachine const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::VirtualMachine const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::VirtualMachine const &) > const & is null");
    return;
  }
  (arg1)->on_this_migration_start_cb((std::function<void(simgrid::s4u::VirtualMachine const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1on_1migration_1end_1cb(JNIEnv* jenv,
                                                                                               jclass jcls, jlong cthis)
{
  std::function<void(simgrid::s4u::VirtualMachine const&)>* arg1 = 0;

  (void)jenv;
  (void)jcls;
  arg1 = *(std::function<void(simgrid::s4u::VirtualMachine const&)>**)&cthis;
  if (!arg1) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::VirtualMachine const &) > const & is null");
    return;
  }
  simgrid::s4u::VirtualMachine::on_migration_end_cb(
      (std::function<void(simgrid::s4u::VirtualMachine const&)> const&)*arg1);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_VirtualMachine_1on_1this_1migration_1end_1cb(
    JNIEnv* jenv, jclass jcls, jlong cthis, jobject jthis, jlong jarg2)
{
  simgrid::s4u::VirtualMachine* arg1                             = (simgrid::s4u::VirtualMachine*)0;
  std::function<void(simgrid::s4u::VirtualMachine const&)>* arg2 = 0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1     = 0;

  (void)jenv;
  (void)jcls;
  (void)jthis;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  arg2 = *(std::function<void(simgrid::s4u::VirtualMachine const&)>**)&jarg2;
  if (!arg2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException,
                            "std::function< void (simgrid::s4u::VirtualMachine const &) > const & is null");
    return;
  }
  (arg1)->on_this_migration_end_cb((std::function<void(simgrid::s4u::VirtualMachine const&)> const&)*arg2);
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_delete_1VirtualMachine(JNIEnv* jenv, jclass jcls, jlong cthis)
{
  simgrid::s4u::VirtualMachine* arg1                         = (simgrid::s4u::VirtualMachine*)0;
  boost::shared_ptr<simgrid::s4u::VirtualMachine>* smartarg1 = 0;

  (void)jenv;
  (void)jcls;

  // plain pointer
  smartarg1 = *(boost::shared_ptr<simgrid::s4u::VirtualMachine>**)&cthis;
  arg1      = (simgrid::s4u::VirtualMachine*)(smartarg1 ? smartarg1->get() : 0);

  (void)arg1;
  delete smartarg1;
}

XBT_PUBLIC void JNICALL Java_org_simgrid_s4u_simgridJNI_swig_1module_1init(JNIEnv* jenv, jclass jcls)
{
  int i;

  static struct {
    const char* method;
    const char* signature;
  } methods[3]            = {{"SwigDirector_BooleanCallback_run", "(Lorg/simgrid/s4u/BooleanCallback;Z)V"},
                             {"SwigDirector_ActorCallback_run", "(Lorg/simgrid/s4u/ActorCallback;J)V"},
                             {"SwigDirector_ActorMain_run", "(Lorg/simgrid/s4u/ActorMain;)V"}};
  Swig::jclass_simgridJNI = (jclass)jenv->NewGlobalRef(jcls);
  if (!Swig::jclass_simgridJNI)
    return;
  for (i = 0; i < (int)(sizeof(methods) / sizeof(methods[0])); ++i) {
    Swig::director_method_ids[i] = jenv->GetStaticMethodID(jcls, methods[i].method, methods[i].signature);
    if (!Swig::director_method_ids[i])
      return;
  }
}

#ifdef __cplusplus
}
#endif

JavaVM* simgrid_cached_jvm = NULL;
extern bool do_install_signal_handlers;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* jvm, void* reserved)
{
  simgrid_cached_jvm = jvm;
  setlocale(LC_NUMERIC, "C");
  do_install_signal_handlers = false;
  return JNI_VERSION_1_2;
}

#include "src/kernel/context/ContextJava.hpp"
static struct SimGridJavaInit {
  SimGridJavaInit()
  {
    simgrid::kernel::context::ContextFactory::initializer = []() {
      XBT_INFO("Using regular java threads.");
      return new simgrid::kernel::context::JavaContextFactory();
    };
  }
} sgJavaInit;

/* Restore our compilation flags, requesting declarations */
#pragma GCC diagnostic pop
