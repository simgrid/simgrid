import org.simgrid.msg.Host;
import org.simgrid.msg.HostNotFoundException;
import org.simgrid.msg.Msg;
import org.simgrid.msg.NativeException;

public class Main {
    private static boolean endOfTest = false;

    public static void setEndOfTest(){
        endOfTest=true;
    }

    public static boolean isEndOfTest(){
        return endOfTest;
    }

    public static void main(String[] args) throws NativeException {
       /* Init. internal values */
        Msg.init(args);

       /* construct the platform and deploy the application */
        Msg.createEnvironment("./CONFIG/platform_simple.xml");
        Msg.deployApplication("./CONFIG/deploy_simple.xml");

        Msg.run();


    }
}
