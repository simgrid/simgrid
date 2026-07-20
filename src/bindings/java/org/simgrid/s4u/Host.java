/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
package org.simgrid.s4u;

/**
 * Some physical resource with computing and networking capabilities on which Actors execute.
 *
 * All hosts are automatically created when the platform is loaded. You cannot create a host yourself. You can
 * retrieve a particular host using {@link Engine#host_by_name} and actors can retrieve the host on which they run
 * using {@link Host#current}.
 */
public class Host {
  private transient long swigCPtr;

  protected Host(long cPtr) { swigCPtr = cPtr; }

  protected static long getCPtr(Host obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  /** Retrieves the name of this host */
  public String get_name() {
    return simgridJNI.Host_get_name(swigCPtr, this);
  }

  /**
   * Get the peak computing speed in flops/s at the current pstate, NOT taking the external load into account.
   *
   * The amount of flops per second available for computing depends on several things: the current pstate
   * determines the maximal peak computing speed; if an external load was declared, you must multiply the result of
   * this function by the available speed ratio to retrieve what a new computation would get. The remaining speed is
   * then shared between the executions located on this host. The host may have multiple cores, and your executions
   * may be able to use more than a single core. Finally, executions of priority 2 get twice the amount of flops
   * than executions of priority 1.
   */
  public double get_speed() {
    return simgridJNI.Host_get_speed(swigCPtr, this);
  }
  /** Returns the current pstate of this host */
  public int get_pstate() { return simgridJNI.Host_get_pstate(swigCPtr); }
  /** Sets the pstate at which this host runs */
  public void set_pstate(int p) { simgridJNI.Host_set_pstate(swigCPtr, p); }
  /** Returns the total count of pstates defined for this host */
  public int get_pstate_count() { return simgridJNI.Host_get_pstate_count(swigCPtr, this); }
  /** Returns the peak computing speed in flops/s at the given pstate, NOT taking the external load into account */
  public double get_pstate_speed(int pstate) { return simgridJNI.Host_get_pstate_speed(swigCPtr, this, pstate); }
  /**
   * Returns the current computation load (in flops per second). The external load (coming from an availability
   * trace) is not taken in account. You may also be interested in the load plugin.
   */
  public double get_load() { return simgridJNI.Host_get_load(swigCPtr); }

  /** Returns the number of cores of the processor. */
  public int get_core_count() { return simgridJNI.Host_get_core_count(swigCPtr); }

  /** Returns if this host is currently up and running */
  public boolean is_on() { return simgridJNI.Host_is_on(swigCPtr, this); }
  /** Turns this host off. All actors are forcefully stopped. */
  public void turn_off() { simgridJNI.Host_turn_off(swigCPtr); }
  /**
   * Turns this host on if it was previously off. This call does nothing if the host is already on. If it was off,
   * all actors which were marked 'autorestart' on that host will be restarted automatically.
   */
  public void turn_on() { simgridJNI.Host_turn_on(swigCPtr); }

  /** Start an asynchronous computation on this host (possibly remote) */
  public Exec exec_init(double flops_amounts)
  {
    long cPtr = simgridJNI.Host_exec_init(swigCPtr, flops_amounts);
    return (cPtr == 0) ? null : new Exec(cPtr, true);
  }
  /** Start and detach an asynchronous computation on this host (possibly remote) */
  public Exec exec_async(double flops_amounts)
  {
    long cPtr = simgridJNI.Host_exec_async(swigCPtr, flops_amounts);
    return (cPtr == 0) ? null : new Exec(cPtr, true);
  }
  /** Retrieves the host on which the calling actor is running */
  public static Host current()
  {
    long cPtr = simgridJNI.Host_current();
    return (cPtr == 0) ? null : new Host(cPtr);
  }
  /** Add a disk to this host */
  public Disk add_disk(String name, double read_bandwidth, double write_bandwidth)
  {
    long cPtr = simgridJNI.Host_add_disk(swigCPtr, this, name, read_bandwidth, write_bandwidth);
    return (cPtr == 0) ? null : new Disk(cPtr);
  }

  /** Starts the given actor on this host */
  public Actor add_actor(String name, Actor actor)
  {
    if (Actor.getCPtr(actor) != 0)
      Engine.die("The cPtr of actor %s is not 0 as expected", name);
    var cPtr = simgridJNI.Actor_create(name, Host.getCPtr(this), this, actor);
    Actor.fire_creation_signal(actor, cPtr);
    return actor;
  }
  /** Returns the names of the properties defined for this host */
  public String[] get_properties_names() { return simgridJNI.Host_get_properties_names(swigCPtr, this); }
  /** Retrieve the value of a host property (or null if not set) */
  public String get_property(String name) { return simgridJNI.Host_get_property(swigCPtr, this, name); }

  /** Returns the list of disks attached to this host */
  public Disk[] get_disks() { return simgridJNI.Host_get_disks(swigCPtr, this); }

  /** Create a virtual machine on this host, with the given amount of cores */
  public VirtualMachine create_vm(String name, int core_amount)
  {
    long cPtr = simgridJNI.Host_create_vm(swigCPtr, this, name, core_amount);
    return (cPtr == 0) ? null : new VirtualMachine(cPtr);
  }

  /** Retrieves the list of links that are used when communicating with the given destination */
  public Link[] route_links_to(Host host) { return simgridJNI.Host_route_links_to(swigCPtr, Host.getCPtr(host)); }
  /** Retrieves the latency used when communicating with the given destination */
  public double route_latency_to(Host host) { return simgridJNI.Host_route_latency_to(swigCPtr, Host.getCPtr(host)); }
  /** Retrieve the user data associated to this host (or null if not set) */
  public Object get_data() { return simgridJNI.Host_get_data(swigCPtr); }
  /** Set the user data associated to this host */
  public void set_data(Object o) { simgridJNI.Host_set_data(swigCPtr, o); }
  /** Set the max amount of executions that can take place on this host at the same time. Use -1 to set no limit. */
  public void set_concurrency_limit(int i) { simgridJNI.Host_set_concurrency_limit(swigCPtr, i); }
  /**
   * Configure the factor callback to the CPU associated to this host. This callback takes the execution size in
   * flops and returns the multiply factor.
   * If the action runs on more than one Host, only the first one is returned
   */
  public void set_cpu_factor_cb(CallbackDHostDouble cb) { simgridJNI.Host_set_cpu_factor_cb(swigCPtr, cb); }

  static boolean LoadPluginInited = false;
  /**
   * Access to the plugin gathering the load-related statistics of this host. Requires
   * {@link Engine#plugin_host_load_init} to have been called before the platform was loaded.
   */
  public class LoadPlugin {
    /** Current load of this host */
    public double get_current()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_link_load_init() before using this plugin");
      return simgridJNI.Host_get_current_load(swigCPtr);
    }
    /** Average load of this host since the last reset */
    public double get_average()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_link_load_init() before using this plugin");
      return simgridJNI.Host_get_avg_load(swigCPtr);
    }
    /** Idle time of this host since the last reset */
    public double get_idle_time()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_link_load_init() before using this plugin");
      return simgridJNI.Host_get_idle_time(swigCPtr);
    }
    /** Total idle time of this host since the simulation start */
    public double get_total_idle_time()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_link_load_init() before using this plugin");
      return simgridJNI.Host_get_total_idle_time(swigCPtr);
    }
    /** Amount of flops computed by this host since the last reset */
    public double get_computed_flops()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_link_load_init() before using this plugin");
      return simgridJNI.Host_get_computed_flops(swigCPtr);
    }
    /** Reset the counters of the load plugin for this host */
    public void reset()
    {
      if (!LoadPluginInited)
        Engine.die("Please use Engine.plugin_link_load_init() before using this plugin");
      simgridJNI.Host_load_reset(swigCPtr);
    }
  }
  /** Access to the plugin gathering the load-related statistics of this host */
  public LoadPlugin load = new LoadPlugin();

  /**
   * Retrieve the total energy consumed by this host so far. Requires {@link Engine#plugin_host_energy_init} to
   * have been called before the platform was loaded.
   */
  public double get_consumed_energy() { return simgridJNI.Host_get_consumed_energy(swigCPtr); }
  /**
   * Retrieve the minimum power consumption of this host at the given pstate. Requires
   * {@link Engine#plugin_host_energy_init} to have been called before the platform was loaded.
   */
  public double get_wattmin_at(int pstate) { return simgridJNI.Host_get_wattmin_at(swigCPtr, pstate); }
  /**
   * Retrieve the maximum power consumption of this host at the given pstate. Requires
   * {@link Engine#plugin_host_energy_init} to have been called before the platform was loaded.
   */
  public double get_wattmax_at(int pstate) { return simgridJNI.Host_get_wattmax_at(swigCPtr, pstate); }
}
