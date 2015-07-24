/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package surfPlugin;

import org.simgrid.surf.*;
import org.simgrid.msg.Msg;
import java.util.HashMap;

public class TracePlugin extends Plugin {

  public TracePlugin() {
    activateCpuCreatedCallback();
    activateCpuDestructedCallback();
    activateCpuStateChangedCallback();
    activateCpuActionStateChangedCallback();

    activateLinkCreatedCallback();
    activateLinkDestructedCallback();
    activateLinkStateChangedCallback();
    activateNetworkActionStateChangedCallback();
  }

  @Override
  public void cpuCreatedCallback(Cpu cpu) {
    Msg.info("Trace: Cpu created "+cpu.getName());
  }

  @Override
  public void cpuDestructedCallback(Cpu cpu) {
    Msg.info("Trace: Cpu destructed "+cpu.getName());
  }

  @Override
  public void cpuStateChangedCallback(Cpu cpu, ResourceState old, ResourceState cur){
    Msg.info("Trace: Cpu state changed "+cpu.getName());
  }

  @Override
  public void cpuActionStateChangedCallback(CpuAction action, ActionState old, ActionState cur){
    Msg.info("Trace: CpuAction state changed");
  }

  @Override
  public void networkLinkCreatedCallback(Link link) {
    Msg.info("Trace: Link created "+link.getName());
  }

  @Override
  public void networkLinkDestructedCallback(Link link) {
    Msg.info("Trace: Link destructed "+link.getName());
  }

  @Override
  public void networkLinkStateChangedCallback(Link link, ResourceState old, ResourceState cur){
    Msg.info("Trace: Link state changed "+link.getName());
  }

  @Override
  public void networkActionStateChangedCallback(NetworkAction action, ActionState old, ActionState cur){
    Msg.info("Trace: NetworkAction state changed");
  }

}
