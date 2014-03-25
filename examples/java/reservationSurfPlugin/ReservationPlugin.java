package reservationSurfPlugin;

import org.simgrid.surf.*;
import org.simgrid.msg.Msg;
import java.util.HashMap;

public class ReservationPlugin extends Plugin {

  public ReservationPlugin() {
    activateCpuCreatedCallback();
    activateCpuDestructedCallback();
    activateCpuStateChangedCallback();
    activateCpuActionStateChangedCallback();

    activateNetworkLinkCreatedCallback();
    activateNetworkLinkDestructedCallback();
    activateNetworkLinkStateChangedCallback();
    activateNetworkActionStateChangedCallback();

  }

  public void init() {
    NetworkLink[] route = Surf.getRoute("Jacquelin", "Boivin");
    Msg.info("RouteLength:"+route.length);
    Msg.info("RouteName0:"+route[0].getName());
    Msg.info("RouteName1:"+route[1].getName());
  }

  public void cpuCreatedCallback(Cpu cpu) {
    Msg.info("Trace: Cpu created "+cpu.getName());
  }

  public void cpuDestructedCallback(Cpu cpu) {
    Msg.info("Trace: Cpu destructed "+cpu.getName());
  }

  public void cpuStateChangedCallback(Cpu cpu, ResourceState old, ResourceState cur){
    Msg.info("Trace: Cpu state changed "+cpu.getName());
  }

  public void cpuActionStateChangedCallback(CpuAction action, ActionState old, ActionState cur){
    Msg.info("Trace: CpuAction state changed "+action.getModel().getName());
  }

  public void networkLinkCreatedCallback(NetworkLink link) {
    Msg.info("Trace: NetworkLink created "+link.getName());
  }

  public void networkLinkDestructedCallback(NetworkLink link) {
    Msg.info("Trace: NetworkLink destructed "+link.getName());
  }

  public void networkLinkStateChangedCallback(NetworkLink link, ResourceState old, ResourceState cur){
    Msg.info("Trace: NetworkLink state changed "+link.getName());
  }

  public void networkActionStateChangedCallback(NetworkAction action, ActionState old, ActionState cur){
    Msg.info("Trace: NetworkAction state changed "+action.getModel().getName());
  }

}
