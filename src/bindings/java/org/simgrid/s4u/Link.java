/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class Link {
  private transient long swigCPtr;
  protected transient boolean swigCMemOwn;

  protected Link(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(Link obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected static long swigRelease(Link obj) {
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

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        throw new UnsupportedOperationException("C++ destructor does not have public access");
      }
      swigCPtr = 0;
    }
  }

  public String get_name() {
    return simgridJNI.Link_get_name(swigCPtr, this);
  }

  public double get_bandwidth() {
    return simgridJNI.Link_get_bandwidth(swigCPtr, this);
  }

  public Link set_bandwidth(double value) {
    simgridJNI.Link_set_bandwidth(swigCPtr, this, value);
    return this;
  }

  public double get_latency() {
    return simgridJNI.Link_get_latency(swigCPtr, this);
  }

  public Link set_latency(double value) {
    simgridJNI.Link_set_latency__SWIG_0(swigCPtr, this, value);
    return this;
  }

  public Link set_latency(String value) {
    simgridJNI.Link_set_latency__SWIG_1(swigCPtr, this, value);
    return this;
  }

  public Link.SharingPolicy get_sharing_policy() {
    return Link.SharingPolicy.swigToEnum(simgridJNI.Link_get_sharing_policy(swigCPtr, this));
  }

  public String get_property(String key) {
    return simgridJNI.Link_get_property(swigCPtr, this, key);
  }

  public Link set_property(String key, String value) {
    simgridJNI.Link_set_property(swigCPtr, this, key, value);
    return this;
  }

  public Link set_concurrency_limit(int limit) {
    simgridJNI.Link_set_concurrency_limit(swigCPtr, this, limit);
    return this;
  }

  public int get_concurrency_limit() {
    return simgridJNI.Link_get_concurrency_limit(swigCPtr, this);
  }

  public void set_host_wifi_rate(Host host, int level) {
    simgridJNI.Link_set_host_wifi_rate(swigCPtr, this, Host.getCPtr(host), host, level);
  }

  public double get_load() {
    return simgridJNI.Link_get_load(swigCPtr, this);
  }

  public boolean is_used() {
    return simgridJNI.Link_is_used(swigCPtr, this);
  }

  public boolean is_shared() {
    return simgridJNI.Link_is_shared(swigCPtr, this);
  }

  public void turn_on() {
    simgridJNI.Link_turn_on(swigCPtr, this);
  }

  public void turn_off() {
    simgridJNI.Link_turn_off(swigCPtr, this);
  }

  public boolean is_on() {
    return simgridJNI.Link_is_on(swigCPtr, this);
  }

  public Link seal() {
    simgridJNI.Link_seal(swigCPtr, this);
    return this;
  }

  public static void on_onoff_cb(CallbackLink cb) { simgridJNI.Link_on_onoff_cb(cb); }

  public void on_this_onoff_cb(CallbackLink cb) { simgridJNI.Link_on_this_onoff_cb(swigCPtr, this, cb); }

  public static void on_bandwidth_change_cb(CallbackLink cb) { simgridJNI.Link_on_bandwidth_change_cb(cb); }

  public void on_this_bandwidth_change_cb(CallbackLink cb)
  {
    simgridJNI.Link_on_this_bandwidth_change_cb(swigCPtr, this, cb);
  }

  public static void on_destruction_cb(CallbackLink cb) { simgridJNI.Link_on_destruction_cb(cb); }

  public void on_this_destruction_cb(CallbackLink cb) { simgridJNI.Link_on_this_destruction_cb(swigCPtr, this, cb); }

  public final static class SharingPolicy {
    public final static Link.SharingPolicy NONLINEAR = new Link.SharingPolicy("NONLINEAR", simgridJNI.Link_SharingPolicy_NONLINEAR_get());
    public final static Link.SharingPolicy WIFI = new Link.SharingPolicy("WIFI", simgridJNI.Link_SharingPolicy_WIFI_get());
    public final static Link.SharingPolicy SPLITDUPLEX = new Link.SharingPolicy("SPLITDUPLEX", simgridJNI.Link_SharingPolicy_SPLITDUPLEX_get());
    public final static Link.SharingPolicy SHARED = new Link.SharingPolicy("SHARED", simgridJNI.Link_SharingPolicy_SHARED_get());
    public final static Link.SharingPolicy FATPIPE = new Link.SharingPolicy("FATPIPE", simgridJNI.Link_SharingPolicy_FATPIPE_get());

    public final int swigValue() {
      return swigValue;
    }

    public String toString() {
      return swigName;
    }

    public static SharingPolicy swigToEnum(int swigValue) {
      if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
        return swigValues[swigValue];
      for (int i = 0; i < swigValues.length; i++)
        if (swigValues[i].swigValue == swigValue)
          return swigValues[i];
      throw new IllegalArgumentException("No enum " + SharingPolicy.class + " with value " + swigValue);
    }

    private SharingPolicy(String swigName) {
      this.swigName = swigName;
      this.swigValue = swigNext++;
    }

    private SharingPolicy(String swigName, int swigValue) {
      this.swigName = swigName;
      this.swigValue = swigValue;
      swigNext = swigValue+1;
    }

    private SharingPolicy(String swigName, SharingPolicy swigEnum) {
      this.swigName = swigName;
      this.swigValue = swigEnum.swigValue;
      swigNext = this.swigValue+1;
    }

    private static SharingPolicy[] swigValues = { NONLINEAR, WIFI, SPLITDUPLEX, SHARED, FATPIPE };
    private static int swigNext = 0;
    private final int swigValue;
    private final String swigName;
  }

  static boolean LoadPluginInited = false;
  public class LoadPlugin {

    public void track()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_host_load_init() before using this plugin");
      simgridJNI.Link_load_track(swigCPtr);
    }
    public void untrack()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_host_load_init() before using this plugin");
      simgridJNI.Link_load_untrack(swigCPtr);
    }
    public void reset()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_host_load_init() before using this plugin");
      simgridJNI.Link_load_reset(swigCPtr);
    }
    public double get_cumulative()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_host_load_init() before using this plugin");
      return simgridJNI.Link_get_cum_load(swigCPtr);
    }
    public double get_average()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_host_load_init() before using this plugin");
      return simgridJNI.Link_get_avg_load(swigCPtr);
    }
    public double get_min_instantaneous()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_host_load_init() before using this plugin");
      return simgridJNI.Link_get_min_instantaneous_load(swigCPtr);
    }
    public double get_max_instantaneous()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_host_load_init() before using this plugin");
      return simgridJNI.Link_get_max_instantaneous_load(swigCPtr);
    }
  }
  public LoadPlugin load = new LoadPlugin();
}
