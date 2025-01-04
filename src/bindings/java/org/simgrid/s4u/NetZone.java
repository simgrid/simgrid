/* Copyright (c) 2024-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class NetZone {
  private transient long swigCPtr;
  protected transient boolean swigCMemOwn;

  protected NetZone(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(NetZone obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected static long swigRelease(NetZone obj) {
    long ptr = 0;
    if (obj != null) {
      if (!obj.swigCMemOwn)
        throw new RuntimeException("Cannot release ownership as memory is not owned");
      ptr = obj.swigCPtr;
      obj.swigCMemOwn = false;
      obj.delete();
    }
    return ptr;
  }

  @SuppressWarnings({"deprecation", "removal"})
  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        simgridJNI.delete_NetZone(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public String get_name() {
    return simgridJNI.NetZone_get_name(swigCPtr, this);
  }

  public NetZone get_parent() {
    long cPtr = simgridJNI.NetZone_get_parent(swigCPtr, this);
    return (cPtr == 0) ? null : new NetZone(cPtr, false);
  }

  public NetZone set_parent(NetZone parent) {
    long cPtr = simgridJNI.NetZone_set_parent(swigCPtr, this, NetZone.getCPtr(parent), parent);
    return (cPtr == 0) ? null : new NetZone(cPtr, false);
  }

  public SWIGTYPE_p_std__vectorT_simgrid__s4u__NetZone_p_t get_children() {
    return new SWIGTYPE_p_std__vectorT_simgrid__s4u__NetZone_p_t(simgridJNI.NetZone_get_children(swigCPtr, this), true);
  }

  public Host[] get_all_hosts() { return simgridJNI.NetZone_get_all_hosts(swigCPtr, this); }

  public long get_host_count() {
    return simgridJNI.NetZone_get_host_count(swigCPtr, this);
  }

  public String get_property(String key) {
    return simgridJNI.NetZone_get_property(swigCPtr, this, key);
  }

  public void set_property(String key, String value) {
    simgridJNI.NetZone_set_property(swigCPtr, this, key, value);
  }

  public static void on_seal_cb(SWIGTYPE_p_std__functionT_void_fsimgrid__s4u__NetZone_const_RF_t cb) {
    simgridJNI.NetZone_on_seal_cb(SWIGTYPE_p_std__functionT_void_fsimgrid__s4u__NetZone_const_RF_t.getCPtr(cb));
  }

  public Host create_host(String name, SWIGTYPE_p_std__vectorT_double_t speed_per_pstate) {
    long cPtr = simgridJNI.NetZone_create_host__SWIG_0(swigCPtr, this, name, SWIGTYPE_p_std__vectorT_double_t.getCPtr(speed_per_pstate));
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public Host create_host(String name, double speed) {
    long cPtr = simgridJNI.NetZone_create_host__SWIG_1(swigCPtr, this, name, speed);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public Host create_host(String name, SWIGTYPE_p_std__vectorT_std__string_t speed_per_pstate) {
    long cPtr = simgridJNI.NetZone_create_host__SWIG_2(swigCPtr, this, name, SWIGTYPE_p_std__vectorT_std__string_t.getCPtr(speed_per_pstate));
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public Host create_host(String name, String speed) {
    long cPtr = simgridJNI.NetZone_create_host__SWIG_3(swigCPtr, this, name, speed);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public Link create_link(String name, SWIGTYPE_p_std__vectorT_double_t bandwidths) {
    long cPtr = simgridJNI.NetZone_create_link__SWIG_0(swigCPtr, this, name, SWIGTYPE_p_std__vectorT_double_t.getCPtr(bandwidths));
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  public Link create_link(String name, double bandwidth) {
    long cPtr = simgridJNI.NetZone_create_link__SWIG_1(swigCPtr, this, name, bandwidth);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  public Link create_link(String name, SWIGTYPE_p_std__vectorT_std__string_t bandwidths) {
    long cPtr = simgridJNI.NetZone_create_link__SWIG_2(swigCPtr, this, name, SWIGTYPE_p_std__vectorT_std__string_t.getCPtr(bandwidths));
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  public Link create_link(String name, String bandwidth) {
    long cPtr = simgridJNI.NetZone_create_link__SWIG_3(swigCPtr, this, name, bandwidth);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  public NetZone seal() {
    long cPtr = simgridJNI.NetZone_seal(swigCPtr, this);
    return (cPtr == 0) ? null : new NetZone(cPtr, false);
  }

  public void set_latency_factor_cb(SWIGTYPE_p_std__functionT_double_fdouble_simgrid__s4u__Host_const_p_simgrid__s4u__Host_const_p_std__vectorT_simgrid__s4u__Link_p_t_const_R_std__unordered_setT_simgrid__s4u__NetZone_p_t_const_RF_t cb) {
    simgridJNI.NetZone_set_latency_factor_cb(swigCPtr, this, SWIGTYPE_p_std__functionT_double_fdouble_simgrid__s4u__Host_const_p_simgrid__s4u__Host_const_p_std__vectorT_simgrid__s4u__Link_p_t_const_R_std__unordered_setT_simgrid__s4u__NetZone_p_t_const_RF_t.getCPtr(cb));
  }

  public void set_bandwidth_factor_cb(SWIGTYPE_p_std__functionT_double_fdouble_simgrid__s4u__Host_const_p_simgrid__s4u__Host_const_p_std__vectorT_simgrid__s4u__Link_p_t_const_R_std__unordered_setT_simgrid__s4u__NetZone_p_t_const_RF_t cb) {
    simgridJNI.NetZone_set_bandwidth_factor_cb(swigCPtr, this, SWIGTYPE_p_std__functionT_double_fdouble_simgrid__s4u__Host_const_p_simgrid__s4u__Host_const_p_std__vectorT_simgrid__s4u__Link_p_t_const_R_std__unordered_setT_simgrid__s4u__NetZone_p_t_const_RF_t.getCPtr(cb));
  }

}
