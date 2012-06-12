package suspend;

import org.simgrid.msg.Msg;

public class Suspend {
	public static void main(String[] args) {
		/* initialize the MSG simulation. Must be done before anything else (even logging). */
		Msg.init(args);
    	if(args.length < 2) {
    		Msg.info("Usage   : Suspend platform_file deployment_file");
        	Msg.info("example : Suspend platform.xml deployment.xml");
        	System.exit(1);
    	}
		/* construct the platform and deploy the application */
		Msg.createEnvironment(args[0]);
		Msg.deployApplication(args[1]);
			
		/*  execute the simulation. */
        Msg.run();		
	}
}
