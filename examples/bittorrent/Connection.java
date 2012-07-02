package bittorrent;

import java.util.Arrays;
import org.simgrid.msg.Msg;
public class Connection {
	/**
	 * Remote peer id
	 */
	public int id;
	/**
	 * Remote peer bitfield.
	 */
	public char bitfield[];
	/**
	 * Remote peer mailbox
	 */
	public String mailbox;
	/**
	 * Indicates if we are interested in something this peer has
	 */
	public boolean amInterested = false;
	/**
	 * Indicates if the peer is interested in one of our pieces
	 */
	public boolean interested = false;
	/**
	 * Indicates if the peer is choked for the current peer
	 */
	public boolean chokedUpload = true;
	/**
	 * Indicates if the peer has choked the current peer
	 */
	public boolean chokedDownload = true;
	/**
	 * Number of messages we have received from the peer
	 */
	public int messagesCount = 0;
	/**
	 * Peer speed.
	 */
	public double peerSpeed = 0;
	/**
	 * Last time the peer was unchoked
	 */
	public double lastUnchoke = 0;
	/**
	 * Constructor
	 */
	public Connection(int id) {
		this.id = id;
		this.mailbox = Integer.toString(id);
	}
	/**
	 * Add a new value to the peer speed average
	 */
	public void addSpeedValue(double speed) {
		peerSpeed = peerSpeed * 0.55 + speed * 0.45;
		//		peerSpeed = (peerSpeed * messagesCount + speed) / (++messagesCount);		
	}
		
	@Override
	public String toString() {
		return "Connection [id=" + id + ", bitfield="
				+ Arrays.toString(bitfield) + ", mailbox=" + mailbox
				+ ", amInterested=" + amInterested + ", interested="
				+ interested + ", chokedUpload=" + chokedUpload
				+ ", chokedDownload=" + chokedDownload + "]";
	}
	
	
}
 