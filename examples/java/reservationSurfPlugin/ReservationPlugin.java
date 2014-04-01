/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package reservationSurfPlugin;

import org.simgrid.surf.*;
import org.simgrid.msg.Msg;
import java.util.HashMap;

public class ReservationPlugin extends Plugin {

  public ReservationPlugin() {
    activateNetworkCommunicateCallback();
  }

  //HashMap<String,Reservation> reservations;
  double bandwidth = 0;
  String src = "";
  String dst = "";

  public void limitBandwidthActions(String src, String dst, double bandwidth){
    this.bandwidth = bandwidth;
    this.src = src;
    this.dst = dst;
  }

  public void updateBandwidthRoute(String src, String dst, double bandwidth){
    NetworkLink[] route = Surf.getRoute(src, dst);
    for (int i =0; i<route.length; i++){
      Msg.info("Trace: bandwidth of "+route[i].getName()+" before "+route[i].getBandwidth());
      route[i].updateBandwidth(bandwidth);//getName();
      Msg.info("Trace: bandwidth of "+route[i].getName()+" after "+route[i].getBandwidth());
    }
  }

  public void networkCommunicateCallback(NetworkAction action, RoutingEdge src, RoutingEdge dst, double size, double rate){
    if (src.getName().equals(this.src) && dst.getName().equals(this.dst)) {
      action.setBound(this.bandwidth);
    }
    Msg.info("Trace: Communicate message of size "+size+" with rate "+rate+" and bound "+action.getBound()+" from "+src.getName()+" to "+dst.getName());
  }

}
