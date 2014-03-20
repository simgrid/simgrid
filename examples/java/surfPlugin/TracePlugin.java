package surfPlugin;

import org.simgrid.surf.*;
import org.simgrid.msg.Msg;
import java.util.HashMap;

public class TracePlugin extends Plugin {

  public TracePlugin() {
    activateCpuCreatedCallback(); 
    //activateCpuDestructedCallback();
    activateCpuStateChangedCallback();                
    activateCpuActionStateChangedCallback();

    activateNetworkLinkCreatedCallback(); 
    //activateCpuDestructedCallback();
    activateNetworkLinkStateChangedCallback();                
    activateNetworkActionStateChangedCallback();
  }
 
  public void cpuCreatedCallback(Cpu cpu) {
    Msg.info("Trace: Cpu created "+cpu.getName());
  }

  public void cpuStateChangedCallback(Cpu cpu){
    Msg.info("Trace: Cpu state changed "+cpu.getName());
  }

  public void cpuActionStateChangedCallback(CpuAction action){
    Msg.info("Trace: CpuAction state changed "+action.getModel().getName());
  }

  public void networkLinkCreatedCallback(NetworkLink link) {
    Msg.info("Trace: NetworkLink created "+link.getName());
  }

  public void networkLinkStateChangedCallback(NetworkLink link){
    Msg.info("Trace: NetworkLink state changed "+link.getName());
  }

  public void networkActionStateChangedCallback(NetworkAction action){
    Msg.info("Trace: NetworkAction state changed "+action.getModel().getName());
  }

}
