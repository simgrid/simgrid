package bittorrent;

import java.util.Arrays;

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
	 * Constructor
	 */
	public Connection(int id) {
		this.id = id;
		this.mailbox = Integer.toString(id);
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
 