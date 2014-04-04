package surfCpuModel;

import org.simgrid.surf.*;
import org.simgrid.msg.Msg;
import java.util.List;
import java.util.ArrayList;

public class CpuConstantModel extends CpuModel {

  private List<CpuConstant> cpus = new ArrayList<CpuConstant>();
  private List<CpuConstantAction> actions = new ArrayList<CpuConstantAction>();

  public CpuConstantModel() {
    super("Cpu Constant");
    Msg.info("Initialize Cpu Constant Model");
  }

  public Cpu createResource(String name, double[] power_peak, int pstate, double power_scale, TmgrTrace power_trace, int core, ResourceState state_initial, TmgrTrace state_trace, XbtDict cpu_properties) {
    Msg.info("Create Resource Name: "+name);
    CpuConstant res = new CpuConstant(this, name, cpu_properties, actions, 1, 1000, 1000);
    cpus.add(res);
    Surf.setCpu(name, res);
    return res;
  }

  public void setState(ResourceState state) {
    Msg.info("setState");
  }

  public double shareResources(double now) {
    Msg.info("shareResource of "+cpus.size()+" cpu and "+actions.size()+" actions");
    return now+1;
  }

  public void updateActionsState(double now, double delta) {
    Msg.info("updateActionState of "+cpus.size()+" cpu and "+actions.size()+" actions");
  }

  public void addTraces() {
  }

public class CpuConstant extends Cpu {
  private List<CpuConstantAction> actions;

  public CpuConstant(CpuConstantModel model, String name, XbtDict props,
                     List<CpuConstantAction> actions,
                     int core, double powerPeak, double powerScale) {
    super(model, name, props, core, powerPeak, powerScale);
    this.actions = actions;
  }

  public CpuAction execute(double size) {
    CpuConstantAction res = new CpuConstantAction(getModel(), size, false);
    Msg.info("Execute action of size "+size);
    actions.add(res);
    return res;
  }

  public CpuAction sleep(double duration) {
    CpuConstantAction res = new CpuConstantAction(getModel(), duration, false);
    Msg.info("Sleep action of duration "+duration);
    actions.add(res);
    return res;
  }

  public ResourceState getState(){
    return ResourceState.SURF_RESOURCE_ON;
  }
}

public class CpuConstantAction extends CpuAction {
  public CpuConstantAction(Model model, double cost, boolean failed) {
    super(model, cost, failed);
  }
}
}

