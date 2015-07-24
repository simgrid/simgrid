package example.symphony;

import peersim.core.Node;

/**
 *
 * @author Andrea Esposito <and1989@gmail.com>
 */
public class Message implements Cloneable {

    public enum MessageType {

        ROUTE, ROUTE_RESPONSE, ROUTE_FAIL,
        JOIN, JOIN_RESPONSE,
        UPDATE_NEIGHBOURS, UPDATE_NEIGHBOURS_RESPONSE,
        REQUEST_LONG_RANGE_LINK, ACCEPTED_LONG_RANGE_LINK, REJECT_LONG_RANGE_LINK, DISCONNECT_LONG_RANGE_LINK, UNAVAILABLE_LONG_RANGE_LINK,
        UPDATE_STATUS, UPDATE_STATUS_RESPONSE,
        LEAVE,
        KEEP_ALIVE, KEEP_ALIVE_RESPONSE
    }
    private long hopCounter;
    private MessageType type;
    private Node src;
    private Node currentHop;
    private Object body;
    private static long globalID = 0;
    private final long id;

    public Message(Object body, Node src, MessageType type) {
        this.type = type;
        this.src = src;
        this.body = body;
        hopCounter = 0;
        id = globalID++;
        currentHop = src;
    }

    public long getID() {
        return id;
    }

    public Object getBody() {
        return body;
    }

    public void incrementHop() {
        hopCounter++;
    }

    public long getHop() {
        return hopCounter;
    }

    public MessageType getType() {
        return type;
    }

    public Node getSourceNode() {
        return src;
    }

    public Node getCurrentHop() {
        return currentHop;
    }

    public void setCurrentHop(Node currentHop) {
        this.currentHop = currentHop;
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

    @Override
    public String toString() {

        StringBuilder builder = new StringBuilder();
        builder.append("Message@").append(this.hashCode()).append("[\n");

        builder.append("\tID : ").append(id).append(",\n");
        builder.append("\tSource ID: ").append(src.getID()).append(",\n");
        builder.append("\tType : ").append(type).append(",\n");
        builder.append("\tBody : ").append(body).append(",\n");
        builder.append("\tCurrent Hop ID: ").append(currentHop.getID()).append(",\n");
        builder.append("\tHop Counter : ").append(hopCounter).append("\n]\n");

        return builder.toString();
    }
}
