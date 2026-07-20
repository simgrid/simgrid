/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

/**
 * A netzone is a network container, in charge of routing information between elements (hosts) and to the nearby
 * netzones. In SimGrid, there is a hierarchy of netzones, with a unique root zone (that you can retrieve from the
 * Engine).
 */
public class NetZone {
  private transient long swigCPtr;

  protected NetZone(long cPtr) { swigCPtr = cPtr; }

  protected static long getCPtr(NetZone obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  /** Retrieves the name of this netzone */
  public String get_name() {
    return simgridJNI.NetZone_get_name(swigCPtr, this);
  }

  /** Retrieve the parent netzone (or null for the root netzone) */
  public NetZone get_parent() {
    long cPtr = simgridJNI.NetZone_get_parent(swigCPtr, this);
    return (cPtr == 0) ? null : new NetZone(cPtr);
  }

  /** Retrieve the list of direct children netzones */
  public NetZone[] get_children() { return simgridJNI.NetZone_get_children(swigCPtr, this); }

  /** Retrieve the list of all hosts included in this netzone (and its children netzones) */
  public Host[] get_all_hosts() { return simgridJNI.NetZone_get_all_hosts(swigCPtr, this); }

  /** Retrieve the amount of hosts included in this netzone (and its children netzones) */
  public long get_host_count() {
    return simgridJNI.NetZone_get_host_count(swigCPtr, this);
  }

  /** Retrieve the property value (or null if not set) */
  public String get_property(String key) {
    return simgridJNI.NetZone_get_property(swigCPtr, this, key);
  }

  /** Set a property (old values will be overwritten) */
  public void set_property(String key, String value) {
    simgridJNI.NetZone_set_property(swigCPtr, this, key, value);
  }

  /** Add a callback fired on each newly sealed NetZone */
  public static void on_seal_cb(CallbackNetzone cb) { simgridJNI.NetZone_on_seal_cb(cb); }

  /** Add a host to this NetZone */
  public Host add_host(String name, double[] speed_per_pstate)
  {
    long cPtr = simgridJNI.NetZone_add_host__SWIG_0(swigCPtr, this, name, speed_per_pstate);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  /** Add a host to this NetZone */
  public Host add_host(String name, double speed) {
    long cPtr = simgridJNI.NetZone_add_host__SWIG_1(swigCPtr, this, name, speed);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  /** Add a host to this NetZone (string version, accepting units such as "1Gf") */
  public Host add_host(String name, String[] speed_per_pstate)
  {
    long cPtr = simgridJNI.NetZone_add_host__SWIG_2(swigCPtr, this, name, speed_per_pstate);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  /** Add a host to this NetZone (string version, accepting units such as "1Gf") */
  public Host add_host(String name, String speed) {
    long cPtr = simgridJNI.NetZone_add_host__SWIG_3(swigCPtr, this, name, speed);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  /**
   * Add a link to this NetZone. The bandwidths array is used to provide several bandwidths for wifi links (one
   * per SNR level).
   */
  public Link add_link(String name, double[] bandwidths)
  {
    long cPtr = simgridJNI.NetZone_add_link__SWIG_0(swigCPtr, this, name, bandwidths);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  /** Add a link to this NetZone */
  public Link add_link(String name, double bandwidth) {
    long cPtr = simgridJNI.NetZone_add_link__SWIG_1(swigCPtr, this, name, bandwidth);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  /**
   * Add a link to this NetZone (string version, accepting units such as "1Gbps"). The bandwidths array is used to
   * provide several bandwidths for wifi links (one per SNR level).
   */
  public Link add_link(String name, String[] bandwidths)
  {
    long cPtr = simgridJNI.NetZone_add_link__SWIG_2(swigCPtr, this, name, bandwidths);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  /** Add a link to this NetZone (string version, accepting units such as "1Gbps") */
  public Link add_link(String name, String bandwidth) {
    long cPtr = simgridJNI.NetZone_add_link__SWIG_3(swigCPtr, this, name, bandwidth);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }
  /**
   * Create a split-duplex link, that is a composition of 2 regular (shared) links (up/down) with the same
   * bandwidth. We append a suffix "_UP" and "_DOWN" to your link name to identify each of them.
   */
  public Link add_split_duplex_link(String name, double bw)
  {
    long cPtr = simgridJNI.NetZone_add_splitlink_from_double(swigCPtr, this, name, bw, bw);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }
  /**
   * Create a split-duplex link, that is a composition of 2 regular (shared) links (up/down). We append a suffix
   * "_UP" and "_DOWN" to your link name to identify each of them.
   */
  public Link add_split_duplex_link(String name, double up, double down)
  {
    long cPtr = simgridJNI.NetZone_add_splitlink_from_double(swigCPtr, this, name, up, down);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  /**
   * Create a split-duplex link (string version, accepting units such as "1Gbps"), with the same bandwidth used in
   * both directions. We append a suffix "_UP" and "_DOWN" to your link name to identify each of them.
   */
  public Link add_split_duplex_link(String name, String bw)
  {
    long cPtr = simgridJNI.NetZone_add_splitlink_from_string(swigCPtr, this, name, bw, bw);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }
  /**
   * Create a split-duplex link (string version, accepting units such as "1Gbps"). We append a suffix "_UP" and
   * "_DOWN" to your link name to identify each of them.
   */
  public Link add_split_duplex_link(String name, String up, String down)
  {
    long cPtr = simgridJNI.NetZone_add_splitlink_from_string(swigCPtr, this, name, up, down);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }
  /** Create a split-duplex link out of two already existing links */
  public Link add_split_duplex_link(String name, Link up, Link down)
  {
    long cPtr =
        simgridJNI.NetZone_add_splitlink_from_links(swigCPtr, this, name, Link.getCPtr(up), Link.getCPtr(down));
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  /** Add a route between 2 netzones, and its symmetrical in the other direction */
  public void add_route(NetZone src, NetZone dst, Link[] links)
  {
    long[] clinks = new long[links.length];
    for (int i = 0; i < links.length; i++)
      clinks[i] = Link.getCPtr(links[i]);
    simgridJNI.NetZone_add_route_netzones(swigCPtr, this, NetZone.getCPtr(src), NetZone.getCPtr(dst), clinks);
  }
  /** Add a route between 2 hosts, and its symmetrical in the other direction */
  public void add_route(Host src, Host dst, Link[] links)
  {
    long[] clinks = new long[links.length];
    for (int i = 0; i < links.length; i++)
      clinks[i] = Link.getCPtr(links[i]);
    simgridJNI.NetZone_add_route_hosts(swigCPtr, this, Host.getCPtr(src), Host.getCPtr(dst), clinks);
  }

  /** Seals this netzone, finishing its configuration */
  public NetZone seal() {
    simgridJNI.NetZone_seal(swigCPtr, this);
    return this;
  }
}
