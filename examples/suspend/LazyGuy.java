package suspend;

import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.Process;
import org.simgrid.msg.MsgException;
public class LazyGuy extends Process {
	public LazyGuy(Host host, String name, String[]args) {
		super(host,name,args);
	} 
	public void main(String[] args) throws MsgException {
		Msg.info("Nobody's watching me ? Let's go to sleep.");
		suspend();
		Msg.info("Uuuh ? Did somebody call me ?");
		Msg.info("Mmmh, goodbye now.");
	}
}