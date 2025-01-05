/* Copyright (c) 2024-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid.s4u;

public class simgrid {

  public static SWIGTYPE_p_std__vectorT_boost__intrusive_ptrT_simgrid__s4u__Activity_t_t create_DAG_from_dot(String filename) {
    return new SWIGTYPE_p_std__vectorT_boost__intrusive_ptrT_simgrid__s4u__Activity_t_t(simgridJNI.create_DAG_from_dot(filename), true);
  }

  public static SWIGTYPE_p_std__vectorT_boost__intrusive_ptrT_simgrid__s4u__Activity_t_t create_DAG_from_DAX(String filename) {
    return new SWIGTYPE_p_std__vectorT_boost__intrusive_ptrT_simgrid__s4u__Activity_t_t(simgridJNI.create_DAG_from_DAX(filename), true);
  }

  public static SWIGTYPE_p_std__vectorT_boost__intrusive_ptrT_simgrid__s4u__Activity_t_t create_DAG_from_json(String filename) {
    return new SWIGTYPE_p_std__vectorT_boost__intrusive_ptrT_simgrid__s4u__Activity_t_t(simgridJNI.create_DAG_from_json(filename), true);
  }

  public static NetZone create_full_zone(String name) {
    long cPtr = simgridJNI.create_full_zone(name);
    return (cPtr == 0) ? null : new NetZone(cPtr);
  }

  public static NetZone create_star_zone(String name) {
    long cPtr = simgridJNI.create_star_zone(name);
    return (cPtr == 0) ? null : new NetZone(cPtr);
  }

  public static NetZone create_dijkstra_zone(String name, boolean cache) {
    long cPtr = simgridJNI.create_dijkstra_zone(name, cache);
    return (cPtr == 0) ? null : new NetZone(cPtr);
  }

  public static NetZone create_empty_zone(String name) {
    long cPtr = simgridJNI.create_empty_zone(name);
    return (cPtr == 0) ? null : new NetZone(cPtr);
  }

  public static NetZone create_floyd_zone(String name) {
    long cPtr = simgridJNI.create_floyd_zone(name);
    return (cPtr == 0) ? null : new NetZone(cPtr);
  }

  public static NetZone create_vivaldi_zone(String name) {
    long cPtr = simgridJNI.create_vivaldi_zone(name);
    return (cPtr == 0) ? null : new NetZone(cPtr);
  }

  public static NetZone create_wifi_zone(String name) {
    long cPtr = simgridJNI.create_wifi_zone(name);
    return (cPtr == 0) ? null : new NetZone(cPtr);
  }
}
