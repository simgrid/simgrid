/* simgrid-java.cpp - Native code of the Java bindings                      */

/* Copyright (c) 2024-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_JAVA_HPP_
#define SIMGRID_JAVA_HPP_

struct SwigDirector_BooleanCallback : public BooleanCallback, public Swig::Director {

public:
  void swig_connect_director(JNIEnv* jenv, jobject jself, jclass jcls, bool swig_mem_own, bool weak_global);
  SwigDirector_BooleanCallback(JNIEnv* jenv);
  virtual void run(bool b);
  virtual ~SwigDirector_BooleanCallback();

public:
  bool swig_overrides(int n) { return (n < 1 ? swig_override[n] : false); }

protected:
  Swig::BoolArray<1> swig_override;
};

struct SwigDirector_ActorCallback : public ActorCallback, public Swig::Director {

public:
  void swig_connect_director(JNIEnv* jenv, jobject jself, jclass jcls, bool swig_mem_own, bool weak_global);
  SwigDirector_ActorCallback(JNIEnv* jenv);
  virtual void run(simgrid::s4u::Actor* a);
  virtual ~SwigDirector_ActorCallback();

public:
  bool swig_overrides(int n) { return (n < 1 ? swig_override[n] : false); }

protected:
  Swig::BoolArray<1> swig_override;
};

struct SwigDirector_ActorMain : public ActorMain, public Swig::Director {

public:
  void swig_connect_director(JNIEnv* jenv, jobject jself, jclass jcls, bool swig_mem_own, bool weak_global);
  SwigDirector_ActorMain(JNIEnv* jenv);
  virtual void run();
  virtual ~SwigDirector_ActorMain();

public:
  bool swig_overrides(int n) { return (n < 1 ? swig_override[n] : false); }

protected:
  Swig::BoolArray<1> swig_override;
};

#endif
