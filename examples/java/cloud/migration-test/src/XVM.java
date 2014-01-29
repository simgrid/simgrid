import org.simgrid.msg.Host;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.VM;

/**
 * A stupid VM extension to associate a daemon to the VM
 */
public class XVM extends VM {


    private int dpIntensity;
    private int netBW;
    private int ramsize;
    private int currentLoad;

    private Daemon daemon;

    public XVM(Host host, String name,
               int nbCores, int ramsize, int netBW, String diskPath, int diskSize, int migNetBW, int dpIntensity){
        super(host, name, nbCores, ramsize, netBW, diskPath, diskSize, (int)(migNetBW*0.9), dpIntensity);
        this.currentLoad = 0;
        this.netBW = netBW ;
        this. dpIntensity = dpIntensity ;
        this.ramsize= ramsize;
        this.daemon = new Daemon(this, 100);

    }

    public void setLoad(int load){  
        if (load >0) {
            this.setBound(load);
        //    this.getDaemon().setLoad(load);
            daemon.resume();
        }
        else{
            daemon.suspend();
        }
        currentLoad = load ;
    }

    public void start(){
        super.start();
         try {
            daemon.start();
        } catch (HostNotFoundException e) {
            e.printStackTrace();
        }
        this.setLoad(0);

    }
    public Daemon getDaemon(){
        return this.daemon;
    }
    public int getLoad(){
       System.out.println("Remaining comp:" + this.daemon.getRemaining());
        return this.currentLoad;
    }

    public void migrate(Host host){
        Msg.info("Start migration of VM " + this.getName() + " to " + host.getName());
        Msg.info("    currentLoad:" + this.currentLoad + "/ramSize:" + this.ramsize + "/dpIntensity:" + this.dpIntensity + "/remaining:" + this.daemon.getRemaining());
        super.migrate(host);
        this.setLoad(this.currentLoad); //Fixed the fact that setBound is not propagated to the new node.
        Msg.info("End of migration of VM " + this.getName() + " to node " + host.getName());
    }
}
