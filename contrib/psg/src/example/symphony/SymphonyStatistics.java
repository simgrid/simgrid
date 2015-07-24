package example.symphony;

import peersim.config.Configuration;
import peersim.core.Control;
import peersim.core.Network;
import peersim.core.Node;

/**
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class SymphonyStatistics implements Control {

    private static final String PAR_SYMPHONY = "symphony";
    private final int symphonyID;
    private long totalMsg = 0;
    private long numRouteResposeMsg = 0;
    private long numRouteMsg = 0;
    private long numRouteFailMsg = 0;
    private long numRouteFoundManagerMsg = 0;
    private double mediaHopRouteResponseMsg = 0.0;

    public SymphonyStatistics(String prefix) {
        symphonyID = Configuration.getPid(prefix + "." + PAR_SYMPHONY);
    }

    public boolean execute() {

        AdapterIterableNetwork itNetwork = new AdapterIterableNetwork();
        for (Node node : itNetwork) {
            SymphonyProtocol symphony = (SymphonyProtocol) node.getProtocol(symphonyID);
            Tuple<Message, Message>[] tupleMessages = symphony.getHistoryMessage();
            totalMsg += tupleMessages.length;

            for (Tuple<Message, Message> tupleMessage : tupleMessages) {

                Message message = tupleMessage.x;

                if (message != null) {
                    switch (message.getType()) {
                        case ROUTE:
                            numRouteMsg++;
                            if (tupleMessage.y != null && tupleMessage.y.getType() == Message.MessageType.ROUTE_RESPONSE) {
                                numRouteFoundManagerMsg++;
                                mediaHopRouteResponseMsg = ((mediaHopRouteResponseMsg * (numRouteFoundManagerMsg - 1)) + message.getHop()) / (double) numRouteFoundManagerMsg;
                            }
                            break;
                        case ROUTE_FAIL:
                            numRouteFailMsg++;
                            break;
                        case ROUTE_RESPONSE:
                            numRouteResposeMsg++;
                            break;
                    }
                }
            }

            symphony.clearHistoryMessage();
        }

        printStatistics();

        return false;
    }

    private void printStatistics() {
        System.out.println("### Statistics ###");
        System.out.println("- Total Messages: " + totalMsg);
        System.out.println("- Total Route Messages: " + numRouteMsg);
        System.out.println("- Found Manager Route Message: " + numRouteFoundManagerMsg);
        System.out.println("- Response Message: " + numRouteResposeMsg);
        System.out.println("- Fail Message: " + numRouteFailMsg);
        System.out.println();
        System.out.println("Average Hop:" + mediaHopRouteResponseMsg + " Expected value (k = log n): " + (Math.log(Network.size()) / Math.log(2)));
        System.out.println("### END ###\n");
    }
}
