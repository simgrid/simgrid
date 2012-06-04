package bittorrent;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map.Entry;

import org.simgrid.msg.Comm;
import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Process;
import org.simgrid.msg.Task;

/**
 * Main class for peers execution
 */
public class Peer extends Process {
	protected int round = 0;
	
	protected double deadline;
	
	protected int id;
	protected String mailbox;
	protected String mailboxTracker;
	protected String hostname;
	protected int pieces = 0;
	protected char[] bitfield = new char[Common.FILE_PIECES];
	protected short[] piecesCount = new short[Common.FILE_PIECES];
	
	protected int piecesRequested = 0;
	
	protected ArrayList<Integer> currentPieces = new ArrayList<Integer>();
	protected int currentPiece = -1;

	protected HashMap<Integer, Connection> activePeers = new HashMap<Integer, Connection>();	
	protected HashMap<Integer, Connection> peers = new HashMap<Integer, Connection>();
	
	protected Comm commReceived = null;

	public Peer(Host host, String name, String[]args) {
		super(host,name,args);
	}	
	
	@Override
	public void main(String[] args) throws MsgException {
		//Check arguments
		if (args.length != 3 && args.length != 2) {
			Msg.info("Wrong number of arguments");
		}
		if (args.length == 3) {
			init(Integer.valueOf(args[0]),true);
		}
		else {
			init(Integer.valueOf(args[0]),false);
		}
		//Retrieve the deadline
		deadline = Double.valueOf(args[1]);
		if (deadline < 0) {
			Msg.info("Wrong deadline supplied");
			return;
		}
		Msg.info("Hi, I'm joining the network with id " + id);
		//Getting peer data from the tracker
		if (getPeersData()) {
			Msg.debug("Got " + peers.size() + " peers from the tracker");
			Msg.debug("Here is my current status: " + getStatus());
			if (hasFinished()) {
				pieces = Common.FILE_PIECES;
				sendHandshakeAll();
				seedLoop();
			}
			else {
				leechLoop();
				seedLoop();
			}
		}
		else {
			Msg.info("Couldn't contact the tracker.");
		}
		Msg.info("Here is my current status: " + getStatus());
	}
	/**
	 * Peer main loop when it is leeching.
	 */
	private void leechLoop() {
		double nextChokedUpdate = Msg.getClock() + Common.UPDATE_CHOKED_INTERVAL;
		Msg.debug("Start downloading.");
		/**
		 * Send a "handshake" message to all the peers it got
		 * (it couldn't have gotten more than 50 peers anyway)
		 */
		sendHandshakeAll();
		//Wait for at least one "bitfield" message.
		waitForPieces();
		Msg.debug("Starting main leech loop");
		while (Msg.getClock() < deadline && pieces < Common.FILE_PIECES) {
			if (commReceived == null) {
				commReceived = Task.irecv(mailbox);
			}
			try {
				if (commReceived.test()) {
					handleMessage(commReceived.getTask());
					commReceived = null;
				}
				else {
					//If the user has a pending interesting
					if (currentPiece != -1) {
						sendInterestedToPeers();
					}
					else {
						if (currentPieces.size() < Common.MAX_PIECES) {
							updateCurrentPiece();
						}
					}
					//We don't execute the choke algorithm if we don't already have a piece
					if (Msg.getClock() >= nextChokedUpdate && pieces > 0) {
						updateChokedPeers();
						nextChokedUpdate += Common.UPDATE_CHOKED_INTERVAL;
					}
					else {
						waitFor(1);
					}
				}
			}
			catch (MsgException e) {
				commReceived = null;				
			}
		}
	}
	
	/**
	 * Peer main loop when it is seeding
	 */
	private void seedLoop() {
		double nextChokedUpdate = Msg.getClock() + Common.UPDATE_CHOKED_INTERVAL;
		Msg.debug("Start seeding.");
		//start the main seed loop
		while (Msg.getClock() < deadline) {
			if (commReceived == null) {
				commReceived = Task.irecv(mailbox);
			}
			try {
				if (commReceived.test()) {
					handleMessage(commReceived.getTask());
					commReceived = null;
				}
				else {
					if (Msg.getClock() >= nextChokedUpdate) {
						updateChokedPeers();
						//TODO: Change the choked peer algorithm when seeding
						nextChokedUpdate += Common.UPDATE_CHOKED_INTERVAL;
					}
					else {
						waitFor(1);
					}
				}
			}
			catch (MsgException e) {
				commReceived = null;				
			}

		}
	}
	
	/**
	 * Initialize the various peer data
	 * @param id id of the peer to take in the network
	 * @param seed indicates if the peer is a seed
	 */
	private void init(int id, boolean seed) {
		this.id = id;
		this.mailbox = Integer.toString(id);
		this.mailboxTracker = "tracker_" + Integer.toString(id);
		if (seed) {
			for (int i = 0; i < bitfield.length; i++) {
				bitfield[i] = '1';
			}
		}
		else {
			for (int i = 0; i < bitfield.length; i++) {
				bitfield[i] = '0';
			}			
		}
		this.hostname = host.getName();
	}
	/**
	 * Retrieves the peer list from the tracker
	 */
	private boolean getPeersData() {
		
		boolean success = false, sendSuccess = false;
		double timeout = Msg.getClock() + Common.GET_PEERS_TIMEOUT;
		//Build the task to send to the tracker
		TrackerTask taskSend = new TrackerTask(hostname, mailboxTracker, id);
			
		while (!sendSuccess && Msg.getClock() < timeout) {
			try {
				Msg.debug("Sending a peer request to the tracker.");
				taskSend.send(Common.TRACKER_MAILBOX,Common.GET_PEERS_TIMEOUT);
				sendSuccess = true;
			}
			catch (MsgException e) {
				
			}
		}
		while (!success && Msg.getClock() < timeout) {
			commReceived = Task.irecv(this.mailboxTracker);
			try {
				commReceived.waitCompletion(Common.GET_PEERS_TIMEOUT);
				if (commReceived.getTask() instanceof TrackerTask) {
					TrackerTask task = (TrackerTask)commReceived.getTask();
					for (Integer peerId: task.peers) {
						if (peerId != this.id) {
							peers.put(peerId, new Connection(peerId));
						}	
					}
					success = true;
				}
			}
			catch (MsgException e) {
				
			}
			commReceived = null;
		}
		commReceived = null;
		return success;
	}
	/**
	 * Handle a received message sent by another peer
 	 * @param task task received.
	 */
	void handleMessage(Task task) {
		MessageTask message = (MessageTask)task;
		Connection remotePeer = peers.get(message.peerId);
		switch (message.type) {
			case HANDSHAKE:
				Msg.debug("Received a HANDSHAKE message from " + message.mailbox);
				//Check if the peer is in our connection list
				if (remotePeer == null) {
					peers.put(message.peerId, new Connection(message.peerId));
					sendHandshake(message.mailbox);
				}
				//Send our bitfield to the pair
				sendBitfield(message.mailbox);
			break;
			case BITFIELD:
				Msg.debug("Received a BITFIELD message from " + message.peerId + " (" + message.issuerHostname + ")");
				//update the pieces list
				updatePiecesCountFromBitfield(message.bitfield);
				//Update the current piece
				if (currentPiece == -1 && pieces < Common.FILE_PIECES && currentPieces.size() < Common.MAX_PIECES) {
					updateCurrentPiece();
				}				
				remotePeer.bitfield  = message.bitfield.clone();
			break;
			case INTERESTED:
				Msg.debug("Received an INTERESTED message from " + message.peerId + " (" + message.issuerHostname + ")");
				assert remotePeer != null;
				remotePeer.interested = true;
			break;
			case NOTINTERESTED:
				Msg.debug("Received a NOTINTERESTED message from " + message.peerId + " (" + message.issuerHostname + ")");
				assert remotePeer != null;
				remotePeer.interested = false;
			break;
			case UNCHOKE:
				Msg.debug("Received an UNCHOKE message from " + message.peerId + "(" + message.issuerHostname + ")");
				assert remotePeer != null;
				remotePeer.chokedDownload = false;
				activePeers.put(remotePeer.id,remotePeer);
				sendRequestsToPeer(remotePeer);
			break;
			case CHOKE:
				Msg.debug("Received a CHOKE message from " + message.peerId + " (" + message.issuerHostname + ")");
				assert remotePeer != null;
				remotePeer.chokedDownload = true;
				activePeers.remove(remotePeer.id);
			break;
			case HAVE:
				if (remotePeer.bitfield == null) {
					return;
				}
				Msg.debug("Received a HAVE message from " + message.peerId + " (" + message.issuerHostname + ")");
				assert message.index >= 0 && message.index < Common.FILE_PIECES;
				assert remotePeer.bitfield != null;
				remotePeer.bitfield[message.index] = '1';
				piecesCount[message.index]++; 
				//Send interested message to the peer if he has what we want
				if (!remotePeer.amInterested && currentPieces.contains(message.index) ) {
					remotePeer.amInterested = true;
					sendInterested(remotePeer.mailbox);
				}
				
				if (currentPieces.contains(message.index)) {
					sendRequest(message.mailbox,message.index);
				}
			break;
			case REQUEST:
				assert message.index >= 0 && message.index < Common.FILE_PIECES;
				if (!remotePeer.chokedUpload) {
					Msg.debug("Received a REQUEST from " + message.peerId + "(" + message.issuerHostname + ") for " + message.peerId);
					if (bitfield[message.index] == '1') {
						sendPiece(message.mailbox,message.index,false);	
					}
					else {
						Msg.debug("Received a REQUEST from " + message.peerId + " (" + message.issuerHostname + ") but he is choked" );
					}
				}
			break;
			case PIECE:
				if (message.stalled) {
					Msg.debug("The received piece " + message.index + " from " + message.peerId + " (" + message.issuerHostname + ")");
				}
				else {
					Msg.debug("Received piece " + message.index + " from " + message.peerId + " (" + message.issuerHostname + ")");
					if (bitfield[message.index] == '0') {
						piecesRequested--;
						//Removing the piece from our piece list.
						//TODO: It can not work, I should test it
						if (!currentPieces.remove((Object)Integer.valueOf(message.index))) {
						}
						//Setting the fact that we have the piece
						bitfield[message.index] = '1';
						pieces++;
						Msg.debug("My status is now " + getStatus());
						//Sending the information to all the peers we are connected to
						sendHave(message.index);
						//sending UNINTERSTED to peers that doesn't have what we want.
						updateInterestedAfterReceive();
					}
					else {
						Msg.debug("However, we already have it.");
					}
				}
			break;
		}
	}
	/**
	 * Wait for the node to receive interesting bitfield messages (ie: non empty)
	 * to be received
	 */
	void waitForPieces() {
		boolean finished = false;
		while (Msg.getClock() < deadline && !finished) {
			if (commReceived == null) {
				commReceived = Task.irecv(mailbox);
			}
			try {
				commReceived.waitCompletion(Common.TIMEOUT_MESSAGE);
				handleMessage(commReceived.getTask());
				if (currentPiece != -1) {
					finished = true;
				}
				commReceived = null;
			}
			catch (MsgException e) {
				commReceived = null;
			}
		}
	}
	
	private boolean hasFinished() {
		for (int i = 0; i < bitfield.length; i++) {
			if (bitfield[i] == '1') {
				return true;
			}
		}
		return false;
	}
	/**
	 * Updates the list of who has a piece from a bitfield
	 * @param bitfield bitfield
	 */
	private void updatePiecesCountFromBitfield(char bitfield[]) {
		for (int i = 0; i < Common.FILE_PIECES; i++) {
			if (bitfield[i] == '1') {
				piecesCount[i]++;
			}
		}
	}
	/**
	 * Update the piece the peer is currently interested in.
	 * There is two cases (as described in "Bittorrent Architecture Protocol", Ryan Toole :
	 * If the peer has less than 3 pieces, he chooses a piece at random.
	 * If the peer has more than pieces, he downloads the pieces that are the less
	 * replicated
	 */
	void updateCurrentPiece() {
		if (currentPieces.size() >= (Common.FILE_PIECES - pieces)) {
			return;
		}
		if (true || pieces < 3) {
			int i = 0, peerPiece;
			do {
				currentPiece = ((int)Msg.getClock() + id + i) % Common.FILE_PIECES;
				i++;
			} while (!(bitfield[currentPiece] == '0' && !currentPieces.contains(currentPiece)));
		}
		else {
			//trivial min algorithm.
			//TODO
		}
		currentPieces.add(currentPiece);
		Msg.debug("New interested piece: " + currentPiece);
		assert currentPiece >= 0 && currentPiece < Common.FILE_PIECES;
	}
	/**
	 * Update the list of current choked and unchoked peers, using the
	 * choke algorithm
	 */
	private void updateChokedPeers() {
		round = (round + 1) % 3;
		if (peers.size() == 0) {
			return;
		}
		//remove a peer from the list
		Iterator<Entry<Integer, Connection>> it = activePeers.entrySet().iterator();
		if (it.hasNext()) {
			Entry<Integer,Connection> e = it.next();
			Connection peerChoked = e.getValue();
			sendChoked(peerChoked.mailbox);
			peerChoked.chokedUpload = true;
			activePeers.remove(e.getKey());
		}
		//Random optimistic unchoking
		if (round == 0 || true) {
			int j = 0, i;
			Connection peerChoosed = null;
			do {
				i = 0;
				int idChosen = ((int)Msg.getClock() + j) % peers.size();
				for (Connection connection : peers.values()) {
					if (i == idChosen) {
						peerChoosed = connection;
						break;
					}
					i++;
				} //TODO: Not really the best way ever
				if (!peerChoosed.interested) {
					peerChoosed = null;
				}
				j++;
			} while (peerChoosed == null && j < Common.MAXIMUM_PAIRS);
			if (peerChoosed != null) {
				activePeers.put(peerChoosed.id,peerChoosed);
				peerChoosed.chokedUpload = false;
				sendUnchoked(peerChoosed.mailbox);
			}
		}
	}
	/**	
	 * Updates our "interested" state about peers: send "not interested" to peers
	 * that don't have any more pieces we want.
	 */
	private void updateInterestedAfterReceive() {
		boolean interested;
		for (Connection connection : peers.values()) {
			interested = false;
			if (connection.amInterested) {
				for (Integer piece : currentPieces) {
					if (connection.bitfield[piece] == '1') {
						interested = true;
						break;
					}
				}	
				if (!interested) {
					connection.amInterested = false;
					sendNotInterested(connection.mailbox);
				}
			}
		}
	}
	/**
	 * Send request messages to a peer that have unchoked us
	 * @param remotePeer peer data to the peer we want to send the request
	 */
	private void sendRequestsToPeer(Connection remotePeer) {
		for (Integer piece : currentPieces) {
			if (remotePeer.bitfield != null && remotePeer.bitfield[piece] == '1') {
				sendRequest(remotePeer.mailbox, piece);
			}			
		}
	}	
	/**
	 * Find the peers that have the current interested piece and send them
	 * the "interested" message
	 */
	private void sendInterestedToPeers() {
		if (currentPiece == -1) {
			return;
		}
		for (Connection connection : peers.values()) {
			if (connection.bitfield != null && connection.bitfield[currentPiece] == '1' && !connection.amInterested) {
				connection.amInterested = true;				
				MessageTask task = new MessageTask(MessageTask.Type.INTERESTED, hostname, this.mailbox, this.id);
				task.dsend(connection.mailbox);				
			}
		}
		currentPiece = -1;
		piecesRequested++;
	}
	/**
	 * Send a "interested" message to a peer.
	 */
	private void sendInterested(String mailbox) {
		MessageTask task = new MessageTask(MessageTask.Type.INTERESTED, hostname, this.mailbox, this.id);
		task.dsend(mailbox);						
	}
	/**
	 * Send a "not interested" message to a peer
	 * @param mailbox mailbox destination mailbox
	 */
	private void sendNotInterested(String mailbox) {
		MessageTask task = new MessageTask(MessageTask.Type.NOTINTERESTED, hostname, this.mailbox, this.id);
		task.dsend(mailbox);				
	}
	/**
	 * Send a handshake message to all the peers the peer has.
	 * @param peer peer data
	 */
	private void sendHandshakeAll() {
		for (Connection remotePeer : peers.values()) {
			MessageTask task = new MessageTask(MessageTask.Type.HANDSHAKE, hostname, mailbox,
			id);
			task.dsend(remotePeer.mailbox);
		}
	}
	/**
	 * Send a "handshake" message to an user
	 * @param mailbox mailbox where to we send the message
	 */
	private void sendHandshake(String mailbox) {
		Msg.debug("Sending a HANDSHAKE to " + mailbox);
		MessageTask task = new MessageTask(MessageTask.Type.HANDSHAKE, hostname, this.mailbox, this.id);
		task.dsend(mailbox);		
	}
	/**
	 * Send a "choked" message to a peer
	 */
	private void sendChoked(String mailbox) {
		Msg.debug("Sending a CHOKE to " + mailbox);
		MessageTask task = new MessageTask(MessageTask.Type.CHOKE, hostname, this.mailbox, this.id);
		task.dsend(mailbox);
	}
	/**
	 * Send a "unchoked" message to a peer
	 */
	private void sendUnchoked(String mailbox) {
		Msg.debug("Sending a UNCHOKE to " + mailbox);
		MessageTask task = new MessageTask(MessageTask.Type.UNCHOKE, hostname, this.mailbox, this.id);
		task.dsend(mailbox);
	}
	/**
	 * Send a "HAVE" message to all peers we are connected to
	 */
	private void sendHave(int piece) {
		Msg.debug("Sending HAVE message to all my peers");
		for (Connection remotePeer : peers.values()) {
			MessageTask task = new MessageTask(MessageTask.Type.HAVE, hostname, this.mailbox, this.id, piece);
			task.dsend(remotePeer.mailbox);
		}
	}
	/**
	 * Send a bitfield message to all the peers the peer has.
	 * @param peer peer data
	 */
	private void sendBitfield(String mailbox) {
		Msg.debug("Sending a BITFIELD to " + mailbox);
		MessageTask task = new MessageTask(MessageTask.Type.BITFIELD, hostname, this.mailbox, this.id, this.bitfield);
		task.dsend(mailbox);
	}
	/**
	 * Send a "request" message to a pair, containing a request for a piece
	 */
	private void sendRequest(String mailbox, int piece) {
		Msg.debug("Sending a REQUEST to " + mailbox + " for piece " + piece);
		MessageTask task = new MessageTask(MessageTask.Type.REQUEST, hostname, this.mailbox, this.id, piece);
		task.dsend(mailbox);
	}
	/**
	 * Send a "piece" message to a pair, containing a piece of the file
	 */
	private void sendPiece(String mailbox, int piece, boolean stalled) {
		Msg.debug("Sending the PIECE " + piece + " to " + mailbox);
		MessageTask task = new MessageTask(MessageTask.Type.PIECE, hostname, this.mailbox, this.id, piece, stalled);
		task.dsend(mailbox);
	}
	
	private String getStatus() {
		String s = "";
		for (int i = 0; i < Common.FILE_PIECES; i++) {
			s = s + bitfield[i];
		}
		return s;
	}
}
	
