package bittorrent;

import org.simgrid.msg.Task;
/**
 * Tasks sent between peers
 */
public class MessageTask extends Task {
	public enum Type {
		HANDSHAKE,
		CHOKE,
		UNCHOKE,
		INTERESTED,
		NOTINTERESTED,
		HAVE,
		BITFIELD,
		REQUEST,
		PIECE
	};
	public Type type;
	public String issuerHostname;
	public String mailbox;
	public int peerId;
	public char bitfield[];
	public int index;
	public boolean stalled;
	/**
	 * Constructor, builds a value-less message
	 * @param type
	 * @param issuerHostname
	 * @param mailbox
	 * @param peerId
	 */
	public MessageTask(Type type, String issuerHostname, String mailbox, int peerId) {
		this.type = type;
		this.issuerHostname = issuerHostname;
		this.mailbox = mailbox;
		this.peerId = peerId;
	}
	/**
	 * Constructor, builds a new "have/request/piece" message
	 * @param type
	 * @param issuerHostname
	 * @param mailbox
	 * @param peerId
	 * @param index
	 */
	public MessageTask(Type type, String issuerHostname, String mailbox, int peerId, int index) {
		this.type = type;
		this.issuerHostname = issuerHostname;
		this.mailbox = mailbox;
		this.peerId = peerId;
		this.index = index;
	}
	/**
	 * Constructor, builds a new bitfield message
	 * @param type
	 * @param issuerHostname
	 * @param mailbox
	 * @param peerId
	 * @param bitfield
	 */
	public MessageTask(Type type, String issuerHostname, String mailbox, int peerId, char bitfield[]) {
		this.type = type;
		this.issuerHostname = issuerHostname;
		this.mailbox = mailbox;
		this.peerId = peerId;
		this.bitfield = bitfield;
	}
	/**
	 * Constructor, build a new "piece" message
	 * @param type
	 * @param issuerHostname
	 * @param mailbox
	 * @param peerId
	 * @param index
	 * @param stalled
	 */
	public MessageTask(Type type, String issuerHostname, String mailbox, int peerId, int index, boolean stalled) {
		this.type = type;
		this.issuerHostname = issuerHostname;
		this.mailbox = mailbox;
		this.peerId = peerId;
		this.index = index;
		this.stalled = stalled;
	}	
}
