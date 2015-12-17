package surfCpuModel;

import org.simgrid.surf.*;
import org.simgrid.msg.Msg;
import java.util.List;
import java.util.ArrayList;

public class CpuConstantModel extends CpuModel {

  private List<CpuConstant> cpus = new ArrayList<CpuConstant>();

  public CpuConstantModel() {
    Msg.info("Initialize Cpu Constant Model");
  }

  public Cpu createCpu(String name, double[] power_peak, int pstate, double power_scale, TmgrTrace power_trace, int core, ResourceState state_initial, TmgrTrace state_trace) {
    Msg.info("New Cpu("+name+", "+power_peak[pstate]+", "+power_scale+")");

    CpuConstant res = new CpuConstant(this, name, core, power_peak[pstate], power_scale);
    cpus.add(res);
    return res;
  }

  public double shareResources(double now) {
    double res = -1;
    for (int i=0; i<cpus.size();i++){
      double tmp = cpus.get(i).share(now);
      if (tmp!=-1 && (res==-1 || tmp<res))
        res = tmp;
    }
    Msg.info("ShareResources at time "+res);
    return res;
  }

  public void updateActionsState(double now, double delta) {
    Action[] actions = getRunningActionSet().getArray();
    Msg.info("UpdateActionState of "+actions.length+" actions");
    for (int i=0; i<actions.length; i++) {
      CpuConstantAction action = (CpuConstantAction)actions[i];
      action.update(delta);
      if (!action.sleeping()) {
        Msg.info("action remains "+action.getRemains()+" after delta of "+delta);
        if (action.getRemains()==0.0){
          action.setState(ActionState.SURF_ACTION_DONE);
          Msg.info("action DONE");
        }
      }
    }
  }

  public void addTraces() {
  }

public class CpuConstant extends Cpu {
  private List<CpuConstantAction> actions = new ArrayList<CpuConstantAction>();

  public CpuConstant(CpuConstantModel model, String name, 
                     int core, double powerPeak, double powerScale) {
    super(model, name, core, powerPeak, powerScale);
  }

  public void remove(CpuConstantAction action){
    actions.remove(action);
  }

  public CpuAction execute(double size) {
    CpuConstantAction res = new CpuConstantAction(getModel(), size, false, false, this);
    Msg.info("Execute action of size "+size+" sleeping "+res.sleeping());
    actions.add(res);
    return res;
  }

  public CpuAction sleep(double duration) {
    CpuConstantAction res = new CpuConstantAction(getModel(), duration, false, true, this);
    Msg.info("Sleep action of duration "+duration+" sleeping "+res.sleeping());
    return res;
  }

  public double share(double now){
    double res = -1;
    for (int i=0; i<actions.size();i++){
      double tmp = actions.get(i).getRemains()/getCurrentPowerPeak();
      Msg.info("Share action with new time "+tmp);
      if (res==-1 || tmp<res)
        res = tmp;
    }
    return res;
  }

  public ResourceState getState(){
    return ResourceState.SURF_RESOURCE_ON;
  }
}

public class CpuConstantAction extends CpuAction {
  boolean sleep;
  CpuConstant cpu;
  public CpuConstantAction(Model model, double cost, boolean failed, boolean sleep, CpuConstant cpu) {
    super(model, cost, failed);
    this.sleep = sleep;
    this.cpu = cpu;
  }

  public void update(double delta){
    updateRemains(delta*cpu.getCurrentPowerPeak());
  }
  public synchronized void delete() {
    cpu.remove(this);
    super.delete();
  }

  public boolean sleeping() {return sleep;}
  public void setPriority(double priority) {}
}
}
