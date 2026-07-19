/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

import java.lang.ref.WeakReference;

public class Engine {
  private transient long swigCPtr;
  protected transient boolean swigCMemOwn;

  protected Engine(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(Engine obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected static long swigRelease(Engine obj) {
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
        simgridJNI.delete_Engine(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  /** Create the simulation engine, passing the command-line parameters of your main function */
  public Engine(String[] args) { this(simgridJNI.new_Engine(args), true); }
  /** Retrieve the command-line parameters that were passed to the engine's constructor */
  public String[] get_args() { return simgridJNI.Engine_get_args(swigCPtr); }

  /** Run the simulation until its end */
  public void run() {
    simgridJNI.Engine_run(swigCPtr, this);
  }

  /** Run the simulation until the given date, given in seconds since the simulation start */
  public void run_until(double max_date) {
    simgridJNI.Engine_run_until(swigCPtr, this, max_date);
  }

  /** Retrieve the simulation time (in seconds since the simulation start) */
  public static double get_clock() {
    return simgridJNI.Engine_get_clock();
  }

  /** Retrieve the engine singleton */
  public static Engine get_instance() {
    long cPtr = simgridJNI.Engine_get_instance();
    return (cPtr == 0) ? null : new Engine(cPtr, false);
  }

  /**
   * Creates a new platform, including hosts, links, and the routing table.
   *
   * See also: the documentation on platform files.
   */
  public void load_platform(String platf) {
    simgridJNI.Engine_load_platform(swigCPtr, this, platf);
  }

  /**
   * Seals the platform, finishing the creation of its resources.
   *
   * This method is optional. The seal() is done automatically when you call Engine.run().
   */
  public void seal_platform() {
    simgridJNI.Engine_seal_platform(swigCPtr, this);
  }

  /**
   * Get a debug output of the platform.
   *
   * It looks like a XML platform file, but it may be very different from the input platform file: all netzones are
   * flattened into a unique zone. This representation is mostly useful to debug your platform configuration and
   * ensure that your assumptions over your configuration hold. Do not use the resulting file as an input platform
   * file: it is very verbose, and thus much less efficient than a regular platform file.
   */
  public String flatify_platform() {
    return simgridJNI.Engine_flatify_platform(swigCPtr, this);
  }

  /** Load a deployment file, launching the actors that it contains. */
  public void load_deployment(String deploy) { simgridJNI.Engine_load_deployment(swigCPtr, deploy); }

  /** Returns the amount of hosts existing in the platform. */
  public long get_host_count() {
    return simgridJNI.Engine_get_host_count(swigCPtr, this);
  }

  /**
   * Returns a vector of all hosts found in the platform.
   *
   * The order is generally different from the creation/declaration order in the XML platform because we use a hash
   * table internally.
   */
  public Host[] get_all_hosts() { return simgridJNI.Engine_get_all_hosts(swigCPtr, this); }

  /** Returns the hosts listed in the given MPI hostfile, in the order they appear in the file. */
  public Host[] get_hosts_from_MPI_hostfile(String hostfile)
  {
    return simgridJNI.Engine_get_hosts_from_MPI_hostfile(swigCPtr, this, hostfile);
  }

  /** Find a host from its name. If no host of that name exists, throws an IllegalArgumentException. */
  public Host host_by_name(String name) {
    long cPtr = simgridJNI.Engine_host_by_name(swigCPtr, this, name);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  /** Returns the amount of links existing in the platform. */
  public long get_link_count() {
    return simgridJNI.Engine_get_link_count(swigCPtr, this);
  }

  /** If not null, the provided set will be filled with all activities that fail to start because of a veto */
  public void track_vetoed_activities() { simgridJNI.Engine_track_vetoed_activities(swigCPtr); }
  /** Retrieve the activities that failed to start because of a veto since the last {@link #track_vetoed_activities}. */
  public Activity[] get_vetoed_activities() { return simgridJNI.Engine_get_vetoed_activities(swigCPtr); }
  /** Clear the list of activities that failed to start because of a veto. */
  public void clear_vetoed_activities() { simgridJNI.Engine_clear_vetoed_activities(swigCPtr); }

  /** Returns a vector of all links found in the platform. */
  public Link[] get_all_links() { return simgridJNI.Engine_get_all_links(swigCPtr, this); }

  /** Find a link from its name. If no link of that name exists, throws an IllegalArgumentException. */
  public Link link_by_name(String name) {
    long cPtr = simgridJNI.Engine_link_by_name(swigCPtr, this, name);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  /** Find a mailbox from its name, creating it if it does not exist yet. */
  public Mailbox mailbox_by_name(String name)
  {
    long cPtr = simgridJNI.Engine_mailbox_by_name_or_create(swigCPtr, this, name);
    return (cPtr == 0) ? null : new Mailbox(cPtr);
  }

  /** Find a message queue from its name, creating it if it does not exist yet. */
  public MessageQueue message_queue_by_name(String name)
  {
    long cPtr = simgridJNI.Engine_message_queue_by_name_or_create(swigCPtr, this, name);
    return (cPtr == 0) ? null : new MessageQueue(cPtr, false);
  }

  /** Returns the amount of actors existing in the platform. */
  public long get_actor_count() {
    return simgridJNI.Engine_get_actor_count(swigCPtr, this);
  }

  /** Returns the highest PID ever given to an actor so far. */
  public int get_actor_max_pid() {
    return simgridJNI.Engine_get_actor_max_pid(swigCPtr, this);
  }

  /** Returns a vector of all actors found in the platform. */
  public Actor[] get_all_actors() { return simgridJNI.Engine_get_all_actors(swigCPtr, this); }

  /** Retrieve the root netzone, containing all others. */
  public NetZone get_netzone_root()
  {
    long cPtr = simgridJNI.Engine_get_netzone_root(swigCPtr, this);
    return (cPtr == 0) ? null : new NetZone(cPtr);
  }

  /** Returns a vector of all netzones found in the platform. */
  public NetZone[] get_all_netzones() { return simgridJNI.Engine_get_all_netzones(swigCPtr, this); }

  /** Find a netzone from its name. If no netzone of that name exists, throws an IllegalArgumentException. */
  public NetZone netzone_by_name(String name)
  {
    long cPtr = simgridJNI.Engine_netzone_by_name_or_null(swigCPtr, this, name);
    if (cPtr == 0)
      throw new IllegalArgumentException("NetZone '" + name + "' does not exist. Did you spell its name correctly?");

    return (cPtr == 0) ? null : new NetZone(cPtr);
  }

  /**
   * Set a configuration variable to the given value.
   *
   * Do --help on any SimGrid binary to see the list of currently existing configuration variables.
   *
   * Example: {@code Engine.set_config("host/model:ptask_L07");}
   */
  public static void set_config(String str) {
    simgridJNI.Engine_set_config__SWIG_0(str);
  }

  /** Set an integer configuration variable to the given value. */
  public static void set_config(String name, int value) {
    simgridJNI.Engine_set_config__SWIG_1(name, value);
  }

  /** Set a boolean configuration variable to the given value. */
  public static void set_config(String name, boolean value) {
    simgridJNI.Engine_set_config__SWIG_2(name, value);
  }

  /** Set a double configuration variable to the given value. */
  public static void set_config(String name, double value) {
    simgridJNI.Engine_set_config__SWIG_3(name, value);
  }

  /** Set a string configuration variable to the given value. */
  public static void set_config(String name, String value) {
    simgridJNI.Engine_set_config__SWIG_4(name, value);
  }

  /** Activates the VM live migration plugin. Must be called before the platform is loaded. */
  public void plugin_vm_live_migration_init() { simgridJNI.Engine_plugin_vm_live_migration_init(swigCPtr); }
  /** Activates the host energy plugin. Must be called before the platform is loaded. */
  public void plugin_host_energy_init() { simgridJNI.Engine_plugin_host_energy_init(swigCPtr); }
  /** Activates the link energy plugin. Must be called before the platform is loaded. */
  public void plugin_link_energy_init() { simgridJNI.Engine_plugin_link_energy_init(swigCPtr); }
  /** Activates the wifi energy plugin. Must be called before the platform is loaded. */
  public void plugin_wifi_energy_init() { simgridJNI.Engine_plugin_wifi_energy_init(swigCPtr); }
  /** Activates the host load plugin. Must be called before the platform is loaded. */
  public void plugin_host_load_init()
  {
    Host.LoadPluginInited = true;
    simgridJNI.Engine_plugin_host_load_init(swigCPtr);
  }
  /** Activates the link load plugin. Must be called before the platform is loaded. */
  public void plugin_link_load_init()
  {
    Link.LoadPluginInited = true;
    simgridJNI.Engine_plugin_link_load_init(swigCPtr);
  }

  /** Add a callback fired when the platform is created (ie, the xml file parsed). */
  public static void on_platform_created_cb(CallbackVoid cb) { simgridJNI.Engine_on_platform_created_cb(cb); }

  /**
   * Add a callback fired when the platform is about to be created (ie, after any configuration change and just
   * before the resource creation).
   */
  public static void on_platform_creation_cb(CallbackVoid cb) { simgridJNI.Engine_on_platform_creation_cb(cb); }

  /** Add a callback fired when the main simulation loop starts, at the beginning of the first call to Engine.run(). */
  public static void on_simulation_start_cb(CallbackVoid cb) { simgridJNI.Engine_on_simulation_start_cb(cb); }

  /** Add a callback fired when the main simulation loop ends, just before the end of Engine.run(). */
  public static void on_simulation_end_cb(CallbackVoid cb) { simgridJNI.Engine_on_simulation_end_cb(cb); }

  /**
   * Add a callback fired when the time jumps into the future.
   *
   * It is fired right after the time change (use get_clock() to get the new timestamp). The callback parameter is
   * the time delta since the previous timestamp.
   */
  public static void on_time_advance_cb(CallbackDouble cb) { simgridJNI.Engine_on_time_advance_cb(cb); }

  /**
   * Add a callback fired when the time cannot advance because of inter-actors deadlock. Note that the on_exit of
   * each actor is also executed on deadlock.
   */
  public static void on_deadlock_cb(CallbackVoid cb) { simgridJNI.Engine_on_deadlock_cb(cb); }

  /** Display a logging message of 'critical' priority, then abort the simulation. */
  public static void die(String fmt, Object... args) { simgridJNI.Engine_die(String.format(fmt, args)); }

  /** Display a logging message of 'critical' priority. */
  public static void critical(String fmt, Object... args) { simgridJNI.Engine_critical(String.format(fmt, args)); }

  /** Display a logging message of 'error' priority. */
  public static void error(String fmt, Object... args) { simgridJNI.Engine_error(String.format(fmt, args)); }

  /** Display a logging message of 'warning' priority. */
  public static void warn(String fmt, Object... args) { simgridJNI.Engine_warn(String.format(fmt, args)); }

  /** Display a logging message of 'info' priority. */
  public static void info(String fmt, Object... args) { simgridJNI.Engine_info(String.format(fmt, args)); }

  /** Display a logging message of 'verbose' priority. */
  public static void verbose(String fmt, Object... args) { simgridJNI.Engine_verbose(String.format(fmt, args)); }

  /** Display a logging message of 'debug' priority. */
  public static void debug(String fmt, Object... args) { simgridJNI.Engine_debug(String.format(fmt, args)); }

  /** Load a DAG of activities described as a graphviz dot file. */
  public Activity[] create_DAG_from_dot(String filename) { return simgridJNI.create_DAG_from_dot(filename); }

  /** Load a DAG of activities described as a DAX (workflow) file. */
  public Activity[] create_DAG_from_DAX(String filename) { return simgridJNI.create_DAG_from_DAX(filename); }

  /** Load a DAG of activities described as a JSON file. */
  public Activity[] create_DAG_from_json(String filename) { return simgridJNI.create_DAG_from_json(filename); }

  /** Example launcher. You can use it or provide your own launcher, as you wish */
  public static void main(String[] args)
  {

    /* initialize the SimGrid simulation. Must be done before anything else */
    Engine e = new Engine(args);

    if (args.length < 2) {
      Engine.info("Usage: org.simgrid.s4u.Engine platform_file deployment_file");
      System.exit(1);
    }

    /* Load the platform and deploy the application */
    e.load_platform(args[0]);
    e.load_deployment(args[1]);
    /* Execute the simulation */
    e.run();
  }

  /* Class initializer, to initialize various JNI stuff */
  static { org.simgrid.s4u.NativeLib.nativeInit(); }

  /** Force a full garbage collection pass. Mostly useful to help debugging memory-related issues. */
  public void force_garbage_collection()
  {
    Object obj                = new Object();
    WeakReference<Object> ref = new WeakReference<>(obj);
    obj                       = null;
    while (ref.get() != null) {
      System.gc();
    }
  }
}
