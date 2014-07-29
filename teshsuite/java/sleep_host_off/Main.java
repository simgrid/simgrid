package sleep_host_off;

import org.simgrid.msg.Host;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.NativeException;

public class Main {

    public static void main(String[] args) throws NativeException {
       /* Init. internal values */
        Msg.init(args);

        if (args.length < 2) {
            Msg.info("Usage  : Main platform_file.xml dployment_file.xml");
            System.exit(1);
        }

       /* construct the platform and deploy the application */
        Msg.createEnvironment(args[0]);
        Msg.deployApplication(args[1]);

        try {
            Msg.run();
        } catch (Exception e){
            System.out.println("Bye bye the program crashes !");
        }

    }
}
