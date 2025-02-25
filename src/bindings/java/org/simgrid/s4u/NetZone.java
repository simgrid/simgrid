/* Copyright (c) 2024-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class NetZone {
  private transient long swigCPtr;

  protected NetZone(long cPtr) { swigCPtr = cPtr; }

  protected static long getCPtr(NetZone obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  public String get_name() {
    return simgridJNI.NetZone_get_name(swigCPtr, this);
  }

  public NetZone get_parent() {
    long cPtr = simgridJNI.NetZone_get_parent(swigCPtr, this);
    return (cPtr == 0) ? null : new NetZone(cPtr);
  }

  public NetZone[] get_children() { return simgridJNI.NetZone_get_children(swigCPtr, this); }

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

  public static void on_seal_cb(CallbackNetzone cb) { simgridJNI.NetZone_on_seal_cb(cb); }

  public Host add_host(String name, double[] speed_per_pstate)
  {
    long cPtr = simgridJNI.NetZone_add_host__SWIG_0(swigCPtr, this, name, speed_per_pstate);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public Host add_host(String name, double speed) {
    long cPtr = simgridJNI.NetZone_add_host__SWIG_1(swigCPtr, this, name, speed);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public Host add_host(String name, String[] speed_per_pstate)
  {
    long cPtr = simgridJNI.NetZone_add_host__SWIG_2(swigCPtr, this, name, speed_per_pstate);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public Host add_host(String name, String speed) {
    long cPtr = simgridJNI.NetZone_add_host__SWIG_3(swigCPtr, this, name, speed);
    return (cPtr == 0) ? null : new Host(cPtr);
  }

  public Link add_link(String name, double[] bandwidths)
  {
    long cPtr = simgridJNI.NetZone_add_link__SWIG_0(swigCPtr, this, name, bandwidths);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  public Link add_link(String name, double bandwidth) {
    long cPtr = simgridJNI.NetZone_add_link__SWIG_1(swigCPtr, this, name, bandwidth);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  public Link add_link(String name, String[] bandwidths)
  {
    long cPtr = simgridJNI.NetZone_add_link__SWIG_2(swigCPtr, this, name, bandwidths);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  public Link add_link(String name, String bandwidth) {
    long cPtr = simgridJNI.NetZone_add_link__SWIG_3(swigCPtr, this, name, bandwidth);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }
  public Link add_split_duplex_link(String name, double bw)
  {
    long cPtr = simgridJNI.NetZone_add_splitlink_from_double(swigCPtr, this, name, bw, bw);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }
  public Link add_split_duplex_link(String name, double up, double down)
  {
    long cPtr = simgridJNI.NetZone_add_splitlink_from_double(swigCPtr, this, name, up, down);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  public Link add_split_duplex_link(String name, String bw)
  {
    long cPtr = simgridJNI.NetZone_add_splitlink_from_string(swigCPtr, this, name, bw, bw);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }
  public Link add_split_duplex_link(String name, String up, String down)
  {
    long cPtr = simgridJNI.NetZone_add_splitlink_from_string(swigCPtr, this, name, up, down);
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }
  public Link add_split_duplex_link(String name, Link up, Link down)
  {
    long cPtr =
        simgridJNI.NetZone_add_splitlink_from_links(swigCPtr, this, name, Link.getCPtr(up), Link.getCPtr(down));
    return (cPtr == 0) ? null : new Link(cPtr, false);
  }

  public void add_route(NetZone src, NetZone dst, Link[] links)
  {
    long[] clinks = new long[links.length];
    for (int i = 0; i < links.length; i++)
      clinks[i] = Link.getCPtr(links[i]);
    simgridJNI.NetZone_add_route_netzones(swigCPtr, this, NetZone.getCPtr(src), NetZone.getCPtr(dst), clinks);
  }
  public void add_route(Host src, Host dst, Link[] links)
  {
    long[] clinks = new long[links.length];
    for (int i = 0; i < links.length; i++)
      clinks[i] = Link.getCPtr(links[i]);
    simgridJNI.NetZone_add_route_hosts(swigCPtr, this, Host.getCPtr(src), Host.getCPtr(dst), clinks);
  }

  public NetZone seal() {
    simgridJNI.NetZone_seal(swigCPtr, this);
    return this;
  }
}
