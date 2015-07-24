package example.symphony;

import peersim.core.Network;
import peersim.core.Node;

/**
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class SimpleNetworkSizeEstimatorProtocol implements NetworkSizeEstimatorProtocolInterface {

    public SimpleNetworkSizeEstimatorProtocol(String prefix) {
    }

    public int getNetworkSize(Node node) {
        return Network.size();
    }

    @Override
    public Object clone() {
        return this;
    }
}
