package bittorrent;

import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;

public class Bittorrent {
	public static void main(String[] args) throws MsgException {
		/* initialize the MSG simulation. Must be done before anything else (even logging). */
		Msg.init(args);
    	if(args.length < 2) {
    		Msg.info("Usage   : Bittorrent platform_file deployment_file");
        	Msg.info("example : Bittorrent platform.xml deployment.xml");
        	System.exit(1);
    	}
		
		/* construct the platform and deploy the application */
		Msg.createEnvironment(args[0]);
		Msg.deployApplication(args[1]);
			
		/*  execute the simulation. */
        Msg.run();		
	}

}
