import org.simgrid.msg.*;
import org.simgrid.msg.Process;

public class Daemon extends Process {
	private Task currentTask;
    public Daemon(VM vm, int load) {
		super((Host)vm,"Daemon");
       currentTask = new Task(this.getHost().getName()+"-daemon-0", this.getHost().getSpeed()*100, 0);
    }
    public void main(String[] args) throws MsgException {
        int i = 1;
        while(!Main.isEndOfTest()) {
            // TODO the binding is not yet available
            try {
                currentTask.execute();
            } catch (HostFailureException e) {
                e.printStackTrace();
            } catch (TaskCancelledException e) {
                System.out.println("task cancelled");
                suspend(); // Suspend the process
            }
            currentTask = new Task(this.getHost().getName()+"-daemon-"+(i++), this.getHost().getSpeed()*100, 0);
            Msg.info(currentTask.getName());
        }
    }

    public double getRemaining(){
        return this.currentTask.getRemainingDuration();
    }
    }
