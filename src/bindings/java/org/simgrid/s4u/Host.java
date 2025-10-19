/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
package org.simgrid.s4u;

public class Host {
  private transient long swigCPtr;

  protected Host(long cPtr) { swigCPtr = cPtr; }

  protected static long getCPtr(Host obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  public String get_name() {
    return simgridJNI.Host_get_name(swigCPtr, this);
  }

  public double get_speed() {
    return simgridJNI.Host_get_speed(swigCPtr, this);
  }
  public int get_pstate() { return simgridJNI.Host_get_pstate(swigCPtr); }
  public void set_pstate(int p) { simgridJNI.Host_set_pstate(swigCPtr, p); }
  public int get_pstate_count() { return simgridJNI.Host_get_pstate_count(swigCPtr, this); }
  public double get_pstate_speed(int pstate) { return simgridJNI.Host_get_pstate_speed(swigCPtr, this, pstate); }
  public double get_load() { return simgridJNI.Host_get_load(swigCPtr); }

  public int get_core_count() { return simgridJNI.Host_get_core_count(swigCPtr); }

  public boolean is_on() { return simgridJNI.Host_is_on(swigCPtr, this); }
  public void turn_off() { simgridJNI.Host_turn_off(swigCPtr); }
  public void turn_on() { simgridJNI.Host_turn_on(swigCPtr); }

  public Exec exec_init(double flops_amounts)
  {
    long cPtr = simgridJNI.Host_exec_init(swigCPtr, flops_amounts);
    return (cPtr == 0) ? null : new Exec(cPtr, true);
  }
  public Exec exec_async(double flops_amounts)
  {
    long cPtr = simgridJNI.Host_exec_async(swigCPtr, flops_amounts);
    return (cPtr == 0) ? null : new Exec(cPtr, true);
  }
  public static Host current()
  {
    long cPtr = simgridJNI.Host_current();
    return (cPtr == 0) ? null : new Host(cPtr);
  }
  public Disk add_disk(String name, double read_bandwidth, double write_bandwidth)
  {
    long cPtr = simgridJNI.Host_add_disk(swigCPtr, this, name, read_bandwidth, write_bandwidth);
    return (cPtr == 0) ? null : new Disk(cPtr);
  }

  public Actor add_actor(String name, Actor actor)
  {
    if (Actor.getCPtr(actor) != 0)
      Engine.die("The cPtr of actor %s is not 0 as expected", name);
    var cPtr = simgridJNI.Actor_create(name, Host.getCPtr(this), this, actor);
    Actor.fire_creation_signal(actor, cPtr);
    return actor;
  }
  public String[] get_properties_names() { return simgridJNI.Host_get_properties_names(swigCPtr, this); }
  public String get_property(String name) { return simgridJNI.Host_get_property(swigCPtr, this, name); }

  public Disk[] get_disks() { return simgridJNI.Host_get_disks(swigCPtr, this); }

  public VirtualMachine create_vm(String name, int core_amount)
  {
    long cPtr = simgridJNI.Host_create_vm(swigCPtr, this, name, core_amount);
    return (cPtr == 0) ? null : new VirtualMachine(cPtr);
  }

  public Link[] route_links_to(Host host) { return simgridJNI.Host_route_links_to(swigCPtr, Host.getCPtr(host)); }
  public double route_latency_to(Host host) { return simgridJNI.Host_route_latency_to(swigCPtr, Host.getCPtr(host)); }
  public Object get_data() { return simgridJNI.Host_get_data(swigCPtr); }
  public void set_data(Object o) { simgridJNI.Host_set_data(swigCPtr, o); }
  public void set_concurrency_limit(int i) { simgridJNI.Host_set_concurrency_limit(swigCPtr, i); }
  /** If the action runs on more than one Host, only the first one is returned */
  public void set_cpu_factor_cb(CallbackDHostDouble cb) { simgridJNI.Host_set_cpu_factor_cb(swigCPtr, cb); }

  static boolean LoadPluginInited = false;
  public class LoadPlugin {
    public double get_current()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_link_load_init() before using this plugin");
      return simgridJNI.Host_get_current_load(swigCPtr);
    }
    public double get_average()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_link_load_init() before using this plugin");
      return simgridJNI.Host_get_avg_load(swigCPtr);
    }
    public double get_idle_time()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_link_load_init() before using this plugin");
      return simgridJNI.Host_get_idle_time(swigCPtr);
    }
    public double get_total_idle_time()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_link_load_init() before using this plugin");
      return simgridJNI.Host_get_total_idle_time(swigCPtr);
    }
    public double get_computed_flops()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_link_load_init() before using this plugin");
      return simgridJNI.Host_get_computed_flops(swigCPtr);
    }
    public void reset()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_link_load_init() before using this plugin");
      simgridJNI.Host_load_reset(swigCPtr);
    }
  }
  public LoadPlugin load = new LoadPlugin();

  public double get_consumed_energy() { return simgridJNI.Host_get_consumed_energy(swigCPtr); }
  public double get_wattmin_at(int pstate) { return simgridJNI.Host_get_wattmin_at(swigCPtr, pstate); }
  public double get_wattmax_at(int pstate) { return simgridJNI.Host_get_wattmax_at(swigCPtr, pstate); }
}
