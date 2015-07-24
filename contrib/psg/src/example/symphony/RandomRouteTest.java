package example.symphony;

import java.util.logging.Level;
import java.util.logging.Logger;
import peersim.config.Configuration;
import peersim.core.CommonState;
import peersim.core.Control;
import peersim.core.Network;
import peersim.core.Node;

/**
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class RandomRouteTest implements Control, Handler {

    private static final String PAR_SYMPHONY = "symphony";
    private final int symphonyID;

    public RandomRouteTest(String prefix) {
        symphonyID = Configuration.getPid(prefix + "." + PAR_SYMPHONY);
    }

    public boolean execute() {

        Node src = Network.get(Math.abs(CommonState.r.nextInt()) % Network.size());

        SymphonyProtocol symphony = (SymphonyProtocol) src.getProtocol(symphonyID);
        try {
            symphony.route(src, CommonState.r.nextDouble(), this);
        } catch (RoutingException ex) {
            Logger.getLogger(RandomRouteTest.class.getName()).log(Level.SEVERE, ex.getMessage());
        }

        return false;

    }

    public void handle(SymphonyProtocol symphony, Tuple<Node, Double> tuple) {

        if (tuple == null) {
            Logger.getLogger(RandomRouteTest.class.getName()).log(Level.SEVERE, "FAIL ROUTE RANDOMTEST");
            return;
        }

        Logger.getLogger(RandomRouteTest.class.getName()).log(Level.FINE, symphony.getIdentifier() + " source find the manager of " + tuple.y + " and it is " + ((SymphonyProtocol) tuple.x.getProtocol(symphonyID)).getIdentifier());
    }
}
