/*
 * Copyright (c) 2007-2008 Fabrizio Frioli, Michele Pedrolli
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * --
 *
 * Please send your questions/suggestions to:
 * {fabrizio.frioli, michele.pedrolli} at studenti dot unitn dot it
 *
 */

package example.bittorrent;

import peersim.core.*;
import peersim.config.*;
import peersim.edsim.*;
import peersim.transport.*;

/**
 *	This is the class that implements the BitTorrent module for Peersim
 */
public class BitTorrent implements EDProtocol {
	/**
	 *	The size in Megabytes of the file being shared.
	 *	@config
	 */
	private static final String PAR_SIZE="file_size";
	/**
	 *	The Transport used by the the protocol.
	 *	@config
	 */
	private static final String PAR_TRANSPORT="transport";
	/**
	 *	The maximum number of neighbor that a node can have. 
	 *	@config
	 */
	private static final String PAR_SWARM="max_swarm_size";
	/**
	 *	The maximum number of peers returned by the tracker when a new
	 *	set of peers is requested through a <tt>TRACKER</tt> message.
	 *	@config
	 */
	private static final String PAR_PEERSET_SIZE="peerset_size";
	/**
	 *	Defines how much the network can grow with respect to the <tt>network.size</tt> 
	 *  when {@link NetworkDynamics} is used.
	 *	@config
	 */
	private static final String PAR_MAX_GROWTH="max_growth";
	/**
	 *	Is the number of requests of the same block sent to different peers.
	 *	@config
	 */
	private static final String PAR_DUP_REQ = "duplicated_requests";
	
	/**
	 *	KEEP_ALIVE message.
	 *	@see SimpleEvent#type "Event types"
	 */
	private static final int KEEP_ALIVE = 1;
	
	/**
	 *	CHOKE message.
	 *	@see SimpleEvent#type "Event types"
	 */
	private static final int CHOKE = 2;
	
	/**
	 *	UNCHOKE message.
	 *	@see SimpleEvent#type "Event types"
	 */
	private static final int UNCHOKE = 3;
	
	/**
	 *	INTERESTED message.
	 *	@see SimpleEvent#type "Event types"
	 */
	private static final int INTERESTED = 4;
	
	/**
	 *	NOT_INTERESTED message.
	 *	@see SimpleEvent#type "Event types"
	 */
	private static final int NOT_INTERESTED = 5;
	
	/**
	 *	HAVE message.
	 *	@see SimpleEvent#type "Event types"
	 */
	private static final int HAVE = 6;
	
	/**
	 *	BITFIELD message.
	 *	@see SimpleEvent#type "Event types"
	 */
	private static final int BITFIELD = 7;
	
	/**
	 *	REQUEST message.
	 *	@see SimpleEvent#type "Event types"
	 */
	private static final int REQUEST = 8;
	
	/**
	 *	PIECE message.
	 *	@see SimpleEvent#type "Event types"
	 */	
	private static final int PIECE = 9;

	/**
	 *	CANCEL message.
	 *	@see SimpleEvent#type "Event types"
	 */	
	private static final int CANCEL = 10;
	
	/**
	 *	TRACKER message.
	 *	@see SimpleEvent#type "Event types"
	 */	
	private static final int TRACKER = 11;
	
	/**
	 *	PEERSET message.
	 *	@see SimpleEvent#type "Event types"
	 */	
	private static final int PEERSET = 12;
	
	/**
	 *	CHOKE_TIME event.
	 *	@see SimpleEvent#type "Event types"
	 */	
	private static final int CHOKE_TIME = 13;
	
	/**
	 *	OPTUNCHK_TIME event.
	 *	@see SimpleEvent#type "Event types"
	 */	
	private static final int OPTUNCHK_TIME = 14;
	
	/**
	 *	ANTISNUB_TIME event.
	 *	@see SimpleEvent#type "Event types"
	 */	
	private static final int ANTISNUB_TIME = 15;
	
	/**
	 *	CHECKALIVE_TIME event.
	 *	@see SimpleEvent#type "Event types"
	 */	
	private static final int CHECKALIVE_TIME = 16;
	
	/**
	 *	TRACKERALIVE_TIME event.
	 *	@see SimpleEvent#type "Event types"
	 */	
	private static final int TRACKERALIVE_TIME = 17;
	
	/**
	 *	DOWNLOAD_COMPLETED event.
	 *	@see SimpleEvent#type "Event types"
	 */	
	private static final int DOWNLOAD_COMPLETED = 18;

	/**
	 *	The maxium connection speed of the local node.
	 */
	int maxBandwidth;
	
	/**
	 *	Stores the neighbors ordered by ID.
	 *  @see Element
	 */
	private example.bittorrent.Element byPeer[];
	
	/**
	 *	Contains the neighbors ordered by bandwidth as needed by the unchocking
	 *	algorithm.
	 */
	private example.bittorrent.Element byBandwidth[];
	
	/**
	 *	The Neighbors list.
	 */
	private Neighbor cache[];
	
	/**
	 *	Reference to the neighbors that unchocked the local node.
	 */
	private boolean unchokedBy[];
	
	/**
	 *	Number of neighbors in the cache. When it decreases under 20, a new peerset
	 *	is requested to the tracker.
	 */
	private int nNodes = 0;
	
	/**
	 *	Maximum number of nodes in the network.
	 */
	private int nMaxNodes;
	
	/**
	 *	The status of the local peer. 0 means that the current peer is a leecher, 1 a seeder.
	 */ 
	private int peerStatus;
	
	/**
	 *	Defines how much the network can grow with respect to the <tt>network.size</tt> 
	 *  when {@link NetworkDynamics} is used.
	 */
	public int maxGrowth;
	
	/**
	 *	File status of the local node. Contains the blocks owned by the local node.
	 */
	private int status[];
	
	/**
	 *	Current number of Bitfield request sent. It must be taken into account 
	 *	before sending another one.
	 */
	private int nBitfieldSent = 0;
	
	/**
	 *	Current number of pieces in upload from the local peer.
	 */
	public int nPiecesUp = 0;
	/**
	 *	Current number of pieces in download to the local peer.
	 */
	public int nPiecesDown = 0;
	
	/**
	 *	Current number of piece completed.
	 */
	private int nPieceCompleted = 0;
	
	/**
	 *	Current downloading piece ID, the previous lastInterested piece.
	 */
	int currentPiece = -1;
	
	/**
	 *	Used to compute the average download rates in choking algorithm. Stores the
	 *	number of <tt>CHOKE</tt> events.
	 */
	int n_choke_time = 0;
	
	/**
	 *	Used to send the <tt>TRACKER</tt> message when the local node has 20 neighbors
	 *	for the first time.
	 */
	boolean lock = false;
	
	/**
	 *	Number of peers interested to my pieces.
	 */
	int numInterestedPeers = 0;
	
	/**
	 *	Last piece for which the local node sent an <tt>INTERESTED</tt> message.
	 */
	int lastInterested = -1;
	
	/** 
	 *	The status of the current piece in download. Length 16, every time the local node
	 *	receives a PIECE message, it updates the corrisponding block's cell. The cell
	 *	contains the ID for that block of that piece. If an already owned
	 *	block is received this is discarded.
	 */
	private int pieceStatus[];
	
	/**	
	 *	Length of the file. Stored as number of pieces (256KB each one).
	 */
	int nPieces;
	
	/**
	 *	Contains the neighbors's status of the file. Every row represents a
	 *	node and every a cell has value O if the neighbor doesn't 
	 *	have the piece, 1 otherwise. It has {@link #swarmSize} rows and {@link #nPieces}
	 *	columns.
	 */
	int [][]swarm;
	
	/**	
	 *	The summation of the swarm's rows. Calculated every time a {@link #BITFIELD} message
	 *	is received and updated every time HAVE message is received.
	 */
	int rarestPieceSet[];
	
	/**
	 *	The five pending block requests.
	 */
	int pendingRequest[];
	
	/**
	 *	The maximum swarm size (default is 80)
	 */
	int swarmSize;
	
	/**
	 *	The size of the peerset. This is the number of "friends" nodes
	 *	sent from the tracker to each new node (default: 50)
	 */
	int peersetSize;
	
	/**
	 * The ID of the current node
	 */
	private long thisNodeID;
	
	/**
     *	Number of duplicated requests as specified in the configuration file.
	 *	@see BitTorrent#PAR_DUP_REQ
	 */
	private int numberOfDuplicatedRequests;
	
	/**
	 *	The queue where the requests to serve are stored.
	 *	The default dimension of the queue is 20.
	 */
	Queue requestToServe = null;
	
	/**
	 *	The queue where the out of sequence incoming pieces are stored
	 *	waiting for the right moment to be processed.
     *	The default dimension of the queue is 100.
	 */
	Queue incomingPieces = null;
	
	/**
	 *	The Transport ID.
	 *	@see BitTorrent#PAR_TRANSPORT
	 */
	int tid;
	
	/**
	 *	The reference to the tracker node. If equals to <tt>null</tt>, the local
	 *	node is the tracker.
	 */
	private Node tracker = null;
	
	/**
	 *	The default constructor. Reads the configuration file and initializes the
	 *	configuration parameters.
	 *	@param prefix the component prefix declared in the configuration file
	 */
	public BitTorrent(String prefix){ // Used for the tracker's protocol
		tid = Configuration.getPid(prefix+"."+PAR_TRANSPORT);
		nPieces = (int)((Configuration.getInt(prefix+"."+PAR_SIZE))*1000000/256000);
		swarmSize = (int)Configuration.getInt(prefix+"."+PAR_SWARM);
		peersetSize = (int)Configuration.getInt(prefix+"."+PAR_PEERSET_SIZE);
		numberOfDuplicatedRequests = (int)Configuration.getInt(prefix+"."+PAR_DUP_REQ);
		maxGrowth = (int)Configuration.getInt(prefix+"."+PAR_MAX_GROWTH);
		nMaxNodes = Network.getCapacity()-1;
	}
	
	/**
	 *	Gets the reference to the tracker node.
	 *	@return the reference to the tracker
	 */
	public Node getTracker(){
		return tracker;
	}
	
	/**
	 *	Gets the number of neighbors currently stored in the cache of the local node.
	 *	@return the number of neighbors in the cache
	 */
	public int getNNodes(){
		return this.nNodes;
	}
	
	/**
	 *	Sets the reference to the tracker node.
	 *	@param t the tracker node
	 */
	public void setTracker(Node t){
		tracker = t;
	}
	
	/**
	 *	Sets the ID of the local node.
	 *	@param id the ID of the node
	 */
	public void setThisNodeID(long id) {
		this.thisNodeID = id;
	}
	
	/**
	 *	Gets the ID of the local node.
	 *	@return the ID of the local node
	 */
	public long getThisNodeID(){
		return this.thisNodeID;
	}
	
	/**
	 *	Gets the file status of the local node.
	 *	@return the file status of the local node
	 */
	public int[] getFileStatus(){
		return this.status;
	}
	
	/**
	 *	Initializes the tracker node. This method
	 *	only performs the initialization of the tracker's cache.
	 */
	public void initializeTracker() {
		cache = new Neighbor[nMaxNodes+maxGrowth];
		for(int i=0; i<nMaxNodes+maxGrowth; i++){
			cache[i]= new Neighbor();
		}
	}
	
	/**
	 *	<p>Checks the number of neighbors and if it is equal to 20
	 *	sends a TRACKER messages to the tracker, asking for a new
	 *	peer set.</p>
	 *
	 *	<p>This method *must* be called after every call of {@link #removeNeighbor}
	 *	in {@link #processEvent}.
	 *	</p>
	 */
	private void processNeighborListSize(Node node, int pid) {
		if (nNodes==20) {
			Object ev;
			long latency;
			ev = new SimpleMsg(TRACKER, node);
			Node tracker = ((BitTorrent)node.getProtocol(pid)).tracker;
			if(tracker != null){
//				latency = ((Transport)node.getProtocol(tid)).getLatency(node, tracker);
//				EDSimulator.add(latency,ev,tracker,pid);
				((Transport) node.getProtocol(tid)).send(node, tracker, ev, pid);
			}
		}
	}
	
	/**
	 *	The standard method that processes incoming events.
	 *	@param node reference to the local node for which the event is going to be processed
	 *	@param pid BitTorrent's protocol id
	 *	@param event the event to process
	 */
	public void processEvent(Node node, int pid, Object event){
		
		Object ev;
		long latency;
		switch(((SimpleEvent)event).getType()){
			
			case KEEP_ALIVE: // 1
			{
				Node sender = ((IntMsg)event).getSender();
				int isResponse = ((IntMsg)event).getInt();
				//System.out.println("process, keep_alive: sender is "+sender.getID()+", local is "+node.getID());
				Element e = search(sender.getID());
				if(e!= null){ //if I know the sender
					cache[e.peer].isAlive();
					if(isResponse==0 && alive(sender)){
						Object msg = new IntMsg(KEEP_ALIVE,node,1,0);
//						latency = ((Transport)node.getProtocol(tid)).getLatency(node, sender);
//						EDSimulator.add(latency,msg,sender,pid);
						((Transport) node.getProtocol(tid)).send(node, sender, msg, pid);
						cache[e.peer].justSent();
					}
				}
				else{
					System.err.println("despite it should never happen, it happened");
					ev = new BitfieldMsg(BITFIELD, true, false, node, status, nPieces);
//					latency = ((Transport)node.getProtocol(tid)).getLatency(node,sender);
//					EDSimulator.add(latency,ev,sender,pid);					
					((Transport) node.getProtocol(tid)).send(node, sender, ev, pid);
					nBitfieldSent++;
				}
				
			};break;
				
			case CHOKE: // 2, CHOKE message.
			{
				Node sender = ((SimpleMsg)event).getSender();
				//System.out.println("process, choke: sender is "+sender.getID()+", local is "+node.getID());
				Element e = search(sender.getID());
				if(e!= null){ //if I know the sender
					cache[e.peer].isAlive();
					unchokedBy[e.peer]= false; // I'm choked by it
				}
				else{
					System.err.println("despite it should never happen, it happened");
					ev = new BitfieldMsg(BITFIELD, true, false, node, status, nPieces);
//					latency = ((Transport)node.getProtocol(tid)).getLatency(node,sender);
//					EDSimulator.add(latency,ev,sender,pid);
					((Transport) node.getProtocol(tid)).send(node, sender, ev, pid);
					nBitfieldSent++;
				}
			};break;
				
			case UNCHOKE: // 3, UNCHOKE message.
			{			
				Node sender = ((SimpleMsg)event).getSender();
				//System.out.println("process, unchoke: sender is "+sender.getID()+", local is "+node.getID());
				Element e = search(sender.getID());
				if(e != null){ // If I know the sender
					int senderIndex = e.peer;
					cache[senderIndex].isAlive();
					/* I send to it some of the pending requests not yet satisfied. */
					int t = numberOfDuplicatedRequests;
					for(int i=4;i>=0 && t>0;i--){
						if(pendingRequest[i]==-1)
							break;
						if(alive(cache[senderIndex].node) && swarm[senderIndex][decode(pendingRequest[i],0)]==1){ //If the sender has that piece
							ev = new IntMsg(REQUEST, node,pendingRequest[i],0);
//							latency = ((Transport)node.getProtocol(tid)).getLatency(node,sender);
//							EDSimulator.add(latency,ev, sender,pid);
							((Transport) node.getProtocol(tid)).send(node, sender, ev, pid);
							cache[senderIndex].justSent();
						}
						if(!alive(cache[senderIndex].node)){
							System.out.println("unchoke1 rm neigh "+ cache[i].node.getID() );
							removeNeighbor(cache[senderIndex].node);
							processNeighborListSize(node,pid);
							return;
						}
						t--;
					}
					// I request missing blocks to fill the queue
					int block = getBlock();
					int piece;
					while(block != -2){ //while still available request to send
						if(block < 0){ // No more block to request for the current piece 
							piece = getPiece();
							if(piece == -1){ // no more piece to request
								break;
							}
							for(int j=0; j<swarmSize; j++){// send the interested message to those  
													// nodes which have that piece
								lastInterested = piece;
								if(alive(cache[j].node) && swarm[j][piece]==1){									
									ev = new IntMsg(INTERESTED, node, lastInterested,0);
//									latency = ((Transport)node.getProtocol(tid)).getLatency(node,cache[j].node);
//									EDSimulator.add(latency,ev,cache[j].node,pid);	
									((Transport) node.getProtocol(tid)).send(node, cache[j].node, ev, pid);
									cache[j].justSent();
								}
								
								if(!alive(cache[j].node)){
									//System.out.println("unchoke2 rm neigh "+ cache[j].node.getID() );
									removeNeighbor(cache[j].node);
									processNeighborListSize(node,pid);
								}
							}
							block = getBlock();
						}
						else{ // block value referred to a real block
							if(alive(cache[senderIndex].node) && swarm[senderIndex][decode(block,0)]==1 && addRequest(block)){ // The sender has that block
								ev = new IntMsg(REQUEST, node, block,0);
//								latency = ((Transport)node.getProtocol(tid)).getLatency(node,sender);
//								EDSimulator.add(latency,ev,sender,pid);
								((Transport) node.getProtocol(tid)).send(node, sender, ev, pid);

								cache[senderIndex].justSent();
							}
							else{
								if(!alive(cache[senderIndex].node)){
									System.out.println("unchoke3 rm neigh "+ cache[senderIndex].node.getID() );
									removeNeighbor(cache[senderIndex].node);
									processNeighborListSize(node,pid);
								}
								return;
							}
							block = getBlock();
						}
					}
					unchokedBy[senderIndex] = true; // I add the sender to the list
				}
				else // It should never happen.
				{
					System.err.println("despite it should never happen, it happened");
					for(int i=0; i<swarmSize; i++)
						if(cache[i].node !=null)
							System.err.println(cache[i].node.getID());
					ev = new BitfieldMsg(BITFIELD, true, false, node, status, nPieces);
//					latency = ((Transport)node.getProtocol(tid)).getLatency(node,sender);
//					EDSimulator.add(latency,ev,sender,pid);
					((Transport) node.getProtocol(tid)).send(node, sender, ev, pid);
					nBitfieldSent++;
				}
			};break;
				
			case INTERESTED: // 4, INTERESTED message.
			{
				numInterestedPeers++;
				Node sender = ((IntMsg)event).getSender();
				//System.out.println("process, interested: sender is "+sender.getID()+", local is "+node.getID());
				int value = ((IntMsg)event).getInt();
				Element e = search(sender.getID());
				if(e!=null){
					cache[e.peer].isAlive();
					cache[e.peer].interested = value;
				}
				else{
					System.err.println("despite it should never happen, it happened");
					ev = new BitfieldMsg(BITFIELD, true, false, node, status, nPieces);
//					latency = ((Transport)node.getProtocol(tid)).getLatency(node,sender);
//					EDSimulator.add(latency,ev,sender,pid);
					((Transport) node.getProtocol(tid)).send(node, sender, ev, pid);
					nBitfieldSent++;
				}
				
			}; break;
				
			case NOT_INTERESTED: // 5, NOT_INTERESTED message.
			{
				numInterestedPeers--;
				Node sender = ((IntMsg)event).getSender();
				//System.out.println("process, not_interested: sender is "+sender.getID()+", local is "+node.getID());
				int value = ((IntMsg)event).getInt();
				Element e = search(sender.getID());
				if(e!=null){
					cache[e.peer].isAlive();
					if(cache[e.peer].interested == value)
						cache[e.peer].interested = -1; // not interested
				}
			}; break;
				
			case HAVE: // 6, HAVE message.
			{		
				Node sender = ((IntMsg)event).getSender();
				//System.out.println("process, have: sender is "+sender.getID()+", local is "+node.getID());
				int piece = ((IntMsg)event).getInt();
				Element e = search(sender.getID());
				if(e!=null){
					cache[e.peer].isAlive();
					swarm[e.peer][piece]=1;
					rarestPieceSet[piece]++;
					boolean isSeeder = true;
					for(int i=0; i<nPieces; i++){
						isSeeder = isSeeder && (swarm[e.peer][i]==1);	
					}
					e.isSeeder = isSeeder;
				}
				else{
					System.err.println("despite it should never happen, it happened");
					ev = new BitfieldMsg(BITFIELD, true, false, node, status, nPieces);
//					latency = ((Transport)node.getProtocol(tid)).getLatency(node,sender);
//					EDSimulator.add(latency,ev,sender,pid);
					((Transport) node.getProtocol(tid)).send(node, sender, ev, pid);
					nBitfieldSent++;
				}
			}; break;
				
			case BITFIELD: // 7, BITFIELD message
			{			
				Node sender = ((BitfieldMsg)event).getSender();
				int []fileStatus = ((BitfieldMsg)event).getArray();
				/*Response with NACK*/
				if(!((BitfieldMsg)event).isRequest && !((BitfieldMsg)event).ack){
					Element e = search(sender.getID());
					if(e == null) // if is a response with nack that follows a request
						nBitfieldSent--;
					// otherwise is a response with ack that follows a duplicate
					// insertion attempt
					//System.out.println("process, bitfield_resp_nack: sender is "+sender.getID()+", local is "+node.getID());
					return;
				}
				/*Request with NACK*/
				if(((BitfieldMsg)event).isRequest && !((BitfieldMsg)event).ack){
					//System.out.println("process, bitfield_req_nack: sender is "+sender.getID()+", local is "+node.getID());
					if(alive(sender)){
						Element e = search(sender.getID());
						ev = new BitfieldMsg(BITFIELD, false, true, node, status, nPieces); //response with ack
//						latency = ((Transport)node.getProtocol(tid)).getLatency(node,sender);
//						EDSimulator.add(latency,ev,sender,pid);
						((Transport) node.getProtocol(tid)).send(node, sender, ev, pid);
						cache[e.peer].justSent();
					}
				}
				/*Response with ACK*/
				if(!((BitfieldMsg)event).isRequest && ((BitfieldMsg)event).ack){
					nBitfieldSent--;
					//System.out.println("process, bitfield_resp_ack: sender is "+sender.getID()+", local is "+node.getID());
					if(alive(sender)){
						if(addNeighbor(sender)){
							Element e = search(sender.getID());
							cache[e.peer].isAlive();
							swarm[e.peer] = fileStatus;
							boolean isSeeder = true;
							for(int i=0; i<nPieces; i++){
								rarestPieceSet[i]+= fileStatus[i];
								isSeeder = isSeeder && (fileStatus[i]==1);
							}
							e.isSeeder = isSeeder;
							
							if(nNodes==10 && !lock){ // I begin to request pieces
								lock = true;
								int piece = getPiece();
								if(piece == -1)
									return;
								lastInterested = piece;
								currentPiece = lastInterested;
								ev = new IntMsg(INTERESTED, node, lastInterested,0);
								for(int i=0; i<swarmSize; i++){// send the interested message to those  
														// nodes which have that piece
									if(alive(cache[i].node) && swarm[i][piece]==1){										
//										latency = ((Transport)node.getProtocol(tid)).getLatency(node,cache[i].node);
//										EDSimulator.add(latency,ev,cache[i].node,pid);
										((Transport) node.getProtocol(tid)).send(node, cache[i].node, ev, pid);
										cache[i].justSent();
									}
								}
								
							}
							
						}
					}
					else
						System.out.println("Sender "+sender.getID()+" not alive");
				}
				/*Request with ACK*/
				if(((BitfieldMsg)event).isRequest && ((BitfieldMsg)event).ack){
					//System.out.println("process, bitfield_req_ack: sender is "+sender.getID()+", local is "+node.getID());
					if(alive(sender)){
						if(addNeighbor(sender)){
							Element e = search(sender.getID()); 
							cache[e.peer].isAlive();
							swarm[e.peer] = fileStatus;
							boolean isSeeder = true;
							for(int i=0; i<nPieces; i++){
								rarestPieceSet[i]+= fileStatus[i]; // I update the rarestPieceSet with the pieces of the new node
								isSeeder = isSeeder && (fileStatus[i]==1); // I check if the new node is a seeder
							}
							e.isSeeder = isSeeder;
							ev = new BitfieldMsg(BITFIELD, false, true, node, status, nPieces); //response with ack
//							latency = ((Transport)node.getProtocol(tid)).getLatency(node,sender);
//							EDSimulator.add(latency,ev,sender,pid);
							((Transport) node.getProtocol(tid)).send(node, sender, ev, pid);
							cache[e.peer].justSent();
							if(nNodes==10 && !lock){ // I begin to request pieces
								int piece = getPiece();
								if(piece == -1)
									return;
								lastInterested = piece;
								currentPiece = lastInterested;
								ev = new IntMsg(INTERESTED, node, lastInterested,0);
								for(int i=0; i<swarmSize; i++){// send the interested message to those  
														// nodes which have that piece
									if(alive(cache[i].node) && swarm[i][piece]==1){
//										latency = ((Transport)node.getProtocol(tid)).getLatency(node,cache[i].node);
//										EDSimulator.add(latency,ev,cache[i].node,pid);
										((Transport) node.getProtocol(tid)).send(node, cache[i].node, ev, pid);
										cache[i].justSent();
									}
								}
								
							}
						}
						else {
							Element e;
							if((e = search(sender.getID()))!=null){ // The sender was already in the cache
								cache[e.peer].isAlive();
								ev = new BitfieldMsg(BITFIELD, false, true, node, status, nPieces); //response with ack
//								latency = ((Transport)node.getProtocol(tid)).getLatency(node,sender);
//								EDSimulator.add(latency,ev,sender,pid);
								((Transport) node.getProtocol(tid)).send(node, sender, ev, pid);
								cache[e.peer].justSent();
							}
							else{ // Was not be possible add the sender (nBitfield+nNodes > swarmSize)
								ev = new BitfieldMsg(BITFIELD, false, false, node, status, nPieces); //response with nack
//								latency = ((Transport)node.getProtocol(tid)).getLatency(node,sender);
//								EDSimulator.add(latency,ev,sender,pid);
								((Transport) node.getProtocol(tid)).send(node, sender, ev, pid);
							}
						}
						
					}
					else
						System.out.println("Sender "+sender.getID()+" not alive");
				}
			};break;
				
			case REQUEST: // 8, REQUEST message.
			{
				Object evnt;
				Node sender = ((IntMsg)event).getSender();
				int value = ((IntMsg)event).getInt();
				Element e;
				BitTorrent senderP;
				int remoteRate;
				int localRate;
				int bandwidth;
				int downloadTime;
				
				e = search(sender.getID());
				if (e==null)
					return;
				cache[e.peer].isAlive();
				
				requestToServe.enqueue(value, sender);
								
				/*I serve the enqueued requests until 10 uploding pieces or an empty queue*/
				while(!requestToServe.empty() && nPiecesUp <10){ 
					Request req = requestToServe.dequeue();
					e = search(req.sender.getID());
					if(e!=null && alive(req.sender)){
//						ev = new IntMsg(PIECE, node, req.id);
						nPiecesUp++;
						e.valueUP++;
						senderP = ((BitTorrent)req.sender.getProtocol(pid));
						senderP.nPiecesDown++;
						remoteRate = senderP.maxBandwidth/(senderP.nPiecesUp + senderP.nPiecesDown);
						localRate = maxBandwidth/(nPiecesUp + nPiecesDown);
						bandwidth = Math.min(remoteRate, localRate);
						downloadTime = ((16*8)/(bandwidth))*1000; // in milliseconds
						
						ev = new IntMsg(PIECE, node, req.id, 16*8 * 1024);
						((Transport) node.getProtocol(tid)).send(node, req.sender, ev, pid);
						
//						latency = ((Transport)node.getProtocol(tid)).getLatency(node,req.sender);
//						EDSimulator.add(latency+downloadTime,ev,req.sender,pid);
						cache[e.peer].justSent();
						
						/*I send to me an event to indicate that the download is completed.
						This prevent that, when the receiver death occurres, my value nPiecesUp
						doesn't decrease.*/
						evnt = new SimpleMsg(DOWNLOAD_COMPLETED, req.sender);
//						EDSimulator.add(latency+downloadTime,evnt,node,pid); 
						((Transport) node.getProtocol(tid)).send(node, node, evnt, pid);
					}
				}
			}; break;
				
			case PIECE: // 9, PIECE message.
			{
				Node sender = ((IntMsg)event).getSender();
				/*	Set the correct value for the local uploading and remote 
				downloading number of pieces */
				nPiecesDown--;
				
				if(peerStatus == 1)// To save CPU cycles
					return;
				//System.out.println("process, piece: sender is "+sender.getID()+", local is "+node.getID());
				Element e = search(sender.getID());
				
				if(e==null){ //I can't accept a piece not wait
					return;
				}
				e.valueDOWN++;
				
				cache[e.peer].isAlive();
				
				int value = ((IntMsg)event).getInt();
				int piece = decode(value,0);
				int block = decode(value,1);
				/* If the block has not been already downloaded and it belongs to
					the current downloading piece.*/
				if(piece == currentPiece && decode(pieceStatus[block],0)!= piece){
					pieceStatus[block] = value;
					status[piece]++;
					removeRequest(value);					
					requestNextBlocks(node, pid, e.peer);
					
				}else{ // Either a future piece or an owned piece
					if(piece!=currentPiece && status[piece]!=16){ // Piece not owned, will be considered later
						incomingPieces.enqueue(value, sender);
					}
					
				}
				ev = new IntMsg(CANCEL, node, value,0);
				/* I send a CANCEL to all nodes to which I previously requested the block*/
				for(int i=0; i<swarmSize; i++){ 
					if(alive(cache[i].node) && unchokedBy[i]==true && swarm[i][decode(block,0)]==1 && cache[i].node != sender){
//						latency = ((Transport)node.getProtocol(tid)).getLatency(node,cache[i].node);
//						EDSimulator.add(latency,ev,cache[i].node,pid);
						((Transport) node.getProtocol(tid)).send(node, cache[i].node, ev, pid);
						cache[i].justSent();
					}
				}
				
				if(status[currentPiece]==16){ // if piece completed, I change the currentPiece to the next wanted					
					nPieceCompleted++;
					ev = new IntMsg(HAVE, node, currentPiece,0);
					for(int i=0; i<swarmSize; i++){ // I send the HAVE for the piece
						if(alive(cache[i].node)){
//							latency = ((Transport)node.getProtocol(tid)).getLatency(node,cache[i].node);
//							EDSimulator.add(latency,ev,cache[i].node,pid);
							((Transport) node.getProtocol(tid)).send(node, cache[i].node, ev, pid);
							cache[i].justSent();
						}
						if(!alive(cache[i].node)){
							//System.out.println("piece3 rm neigh "+ cache[i].node.getID() );							
							removeNeighbor(cache[i].node);
							processNeighborListSize(node,pid);
						}
					}
					ev = new IntMsg(NOT_INTERESTED, node, currentPiece,0);
					for(int i=0; i<swarmSize; i++){ // I send the NOT_INTERESTED to which peer I sent an INTERESTED
						if(swarm[i][piece]==1 && alive(cache[i].node)){
//							latency = ((Transport)node.getProtocol(tid)).getLatency(node,cache[i].node);
//							EDSimulator.add(latency,ev,cache[i].node,pid);
							((Transport) node.getProtocol(tid)).send(node, cache[i].node, ev, pid);
							cache[i].justSent();
						}
						if(!alive(cache[i].node)){
							//System.out.println("piece4 rm neigh "+ cache[i].node.getID() );							
							removeNeighbor(cache[i].node);
							processNeighborListSize(node,pid);
						}
					}
					if(nPieceCompleted == nPieces){
						System.out.println("FILE COMPLETED for peer "+node.getID());
						this.peerStatus = 1;	
					}
					
					/*	I set the currentPiece to the lastInterested. Then I extract 
						the queued received blocks
						*/
					
					currentPiece = lastInterested;
					int m = incomingPieces.dim;
					while(m > 0){ // I process the queue
						m--;
						Request temp = incomingPieces.dequeue();
						int p = decode(temp.id,0); // piece id
						int b = decode(temp.id,1); // block id
						Element s = search(temp.sender.getID());
						if(s==null) // if the node that sent the block in the queue is dead
							continue;
						if(p==currentPiece && decode(pieceStatus[b],0)!= p){
							pieceStatus[b] = temp.id;
							status[p]++;
							removeRequest(temp.id);
							requestNextBlocks(node, pid, s.peer);
						}
						else{ // The piece not currently desired will be moved to the tail
							if(p!= currentPiece) // If not a duplicate block but belongs to another piece
								incomingPieces.enqueue(temp.id,temp.sender);
							else // duplicate block
								requestNextBlocks(node, pid, s.peer);
						}
					}
				}
			}; break;
				
			case CANCEL:
			{
				Node sender = ((IntMsg)event).getSender();
				int value = ((IntMsg)event).getInt();
				requestToServe.remove(sender, value);
			};break;
				
			case PEERSET: // PEERSET message
			{
				Node sender = ((PeerSetMsg)event).getSender();
				//System.out.println("process, peerset: sender is "+sender.getID()+", local is "+node.getID());
				Neighbor n[] = ((PeerSetMsg)event).getPeerSet();
				
				for(int i=0; i<peersetSize; i++){
					if( n[i]!=null && alive(n[i].node) && search(n[i].node.getID())==null && nNodes+nBitfieldSent <swarmSize-2) {
						ev = new BitfieldMsg(BITFIELD, true, true, node, status, nPieces);
//						latency = ((Transport)node.getProtocol(tid)).getLatency(node,n[i].node);
//						EDSimulator.add(latency,ev,n[i].node,pid);
						((Transport) node.getProtocol(tid)).send(node, n[i].node, ev, pid);

						nBitfieldSent++;
						// Here I should call the Neighbor.justSent(), but here
						// the node is not yet in the cache.
					}
				}
			}; break;
				
			case TRACKER: // TRACKER message
			{
				
				int j=0;
				Node sender = ((SimpleMsg)event).getSender();
				//System.out.println("process, tracker: sender is "+sender.getID()+", local is "+node.getID());
				if(!alive(sender))
					return;
				Neighbor tmp[] = new Neighbor[peersetSize];
				int k=0;
				if(nNodes <= peersetSize){
					for(int i=0; i< nMaxNodes+maxGrowth; i++){
						if(cache[i].node != null && cache[i].node.getID()!= sender.getID()){
							tmp[k]=cache[i];
							k++;
						}
					}
					ev = new PeerSetMsg(PEERSET, tmp, node);
//					latency = ((Transport)node.getProtocol(tid)).getLatency(node, sender);
//					EDSimulator.add(latency,ev,sender,pid);
					((Transport) node.getProtocol(tid)).send(node,sender, ev, pid);
					return;
				}
				
				while(j < peersetSize){
					int i = CommonState.r.nextInt(nMaxNodes+maxGrowth);
					for (int z=0; z<j; z++){
						if(cache[i].node==null || tmp[z].node.getID() == cache[i].node.getID() || cache[i].node.getID() == sender.getID()){
							z=0;
							i= CommonState.r.nextInt(nMaxNodes+maxGrowth);
						}
					}
					if(cache[i].node != null){
						tmp[j] = cache[i];
						j++;
					}
				}
				ev = new PeerSetMsg(PEERSET, tmp, node);
//				latency = ((Transport)node.getProtocol(tid)).getLatency(node, sender);
//				EDSimulator.add(latency,ev,sender,pid);
				((Transport) node.getProtocol(tid)).send(node,sender, ev, pid);
			}; break;
				
			case CHOKE_TIME: //Every 10 secs.
			{	
				n_choke_time++;
				
				ev = new SimpleEvent(CHOKE_TIME);
				EDSimulator.add(10000,ev,node,pid);
				int j=0;
				/*I copy the interested nodes in the byBandwidth array*/
				for(int i=0;i< swarmSize && byPeer[i].peer != -1; i++){
					if(cache[byPeer[i].peer].interested > 0){
						byBandwidth[j]=byPeer[i]; //shallow copy
						j++;
					}
				}
				
				/*It ensures that in the next 20sec, if there are less nodes interested
					than now, those in surplus will not be ordered. */
				for(;j<swarmSize;j++){
					byBandwidth[j]=null;
				}
				sortByBandwidth();
				int optimistic = 3;
				int luckies[] = new int[3];
				try{ // It takes the first three neighbors
					luckies[0] = byBandwidth[0].peer;
					optimistic--;
					luckies[1] = byBandwidth[1].peer;
					optimistic--;
					luckies[2] = byBandwidth[2].peer;
				}
				catch(NullPointerException e){ // If not enough peer in byBandwidth it chooses the other romdomly 
					for(int z = optimistic; z>0;z--){
						int lucky = CommonState.r.nextInt(nNodes);
						while(cache[byPeer[lucky].peer].status ==1 && alive(cache[byPeer[lucky].peer].node) && 
							  cache[byPeer[lucky].peer].interested == 0)// until the lucky peer is already unchoked or not interested
							lucky = CommonState.r.nextInt(nNodes);
						luckies[3-z]= byPeer[lucky].peer;
					}
				}
				for(int i=0; i<swarmSize; i++){ // I perform the chokes and the unchokes
					if((i==luckies[0] || i==luckies[1] || i==luckies[2]) &&  alive(cache[i].node) && cache[i].status != 2){ //the unchokes
						cache[i].status = 1;
						ev = new SimpleMsg(UNCHOKE, node);
//						latency = ((Transport)node.getProtocol(tid)).getLatency(node, cache[i].node);
//						EDSimulator.add(latency,ev,cache[i].node,pid);
						((Transport) node.getProtocol(tid)).send(node,cache[i].node, ev, pid);
						cache[i].justSent();
						//System.out.println("average time, unchoked: "+cache[i].node.getID());
					}
					else{ // the chokes
						if(alive(cache[i].node) && (cache[i].status == 1 || cache[i].status == 2)){
							cache[i].status = 0;
							ev = new SimpleMsg(CHOKE, node);
//							latency = ((Transport)node.getProtocol(tid)).getLatency(node, cache[i].node);
//							EDSimulator.add(latency,ev,cache[i].node,pid);
							((Transport) node.getProtocol(tid)).send(node,cache[i].node, ev, pid);
							cache[i].justSent();
						}
					}
				}
				
				if(n_choke_time%2==0){ //every 20 secs. Used in computing the average download rates
					for(int i=0; i<nNodes; i++){
						if(this.peerStatus == 0){ // I'm a leeacher
							byPeer[i].head20 = byPeer[i].valueDOWN;
						}
						else{
							byPeer[i].head20 = byPeer[i].valueUP;
						}
					}
				}
			}; break;
				
			case OPTUNCHK_TIME:
			{
				
				//System.out.println("process, optunchk_time");
				
				ev = new SimpleEvent(OPTUNCHK_TIME);
				EDSimulator.add(30000,ev,node,pid);
				int lucky = CommonState.r.nextInt(nNodes);
				while(cache[byPeer[lucky].peer].status ==1)// until the lucky peer is already unchoked
					lucky = CommonState.r.nextInt(nNodes);
				if(!alive(cache[byPeer[lucky].peer].node))
					return;
				cache[byPeer[lucky].peer].status = 1;
				Object msg = new SimpleMsg(UNCHOKE,node);
//				latency = ((Transport)node.getProtocol(tid)).getLatency(node, cache[byPeer[lucky].peer].node);
//				EDSimulator.add(latency,msg,cache[byPeer[lucky].peer].node,pid);
				((Transport) node.getProtocol(tid)).send(node,cache[byPeer[lucky].peer].node, msg, pid);
				cache[byPeer[lucky].peer].justSent();
			}; break;
				
			case ANTISNUB_TIME:
			{
				if(this.peerStatus == 1) // I'm a seeder, I don't update the event
					return;
				//System.out.println("process, antisnub_time");
				for(int i=0; i<nNodes; i++){
					if(byPeer[i].valueDOWN >0 && (byPeer[i].valueDOWN - byPeer[i].head60)==0){// No blocks downloaded in 1 min
						cache[byPeer[i].peer].status = 2; // I'm snubbed by it
					}
					byPeer[i].head60 = byPeer[i].valueDOWN;
				}
				ev = new SimpleEvent(ANTISNUB_TIME);
				EDSimulator.add(60000,ev,node,pid);
				long time = CommonState.getTime();
			}; break;
				
			case CHECKALIVE_TIME:
			{
				
				//System.out.println("process, checkalive_time");
				
				long now = CommonState.getTime();
				for(int i=0; i<swarmSize; i++){
					/*If are at least 2 minutes (plus 1 sec of tolerance) that
					I don't send anything to it.*/
					if(alive(cache[i].node) && (cache[i].lastSent < (now-121000))){
						Object msg = new IntMsg(KEEP_ALIVE,node,0,0);
//						latency = ((Transport)node.getProtocol(tid)).getLatency(node, cache[i].node);
//						EDSimulator.add(latency,msg,cache[i].node,pid);
						((Transport) node.getProtocol(tid)).send(node,cache[i].node, msg, pid);
						cache[i].justSent();
					}
					/*If are at least 2 minutes (plus 1 sec of tolerance) that I don't
					receive anything from it though I sent a keepalive 2 minutes ago*/
					else{
						if(cache[i].lastSeen <(now-121000) && cache[i].node != null && cache[i].lastSent < (now-121000)){
							System.out.println("process, checkalive_time, rm neigh " + cache[i].node.getID());
							if(cache[i].node.getIndex() != -1){
								System.out.println("This should never happen: I remove a node that is not effectively died");
							}
							removeNeighbor(cache[i].node);
							processNeighborListSize(node,pid);
						}
					}
				}
				ev = new SimpleEvent(CHECKALIVE_TIME);
				EDSimulator.add(120000,ev,node,pid);
			}; break;
				
			case TRACKERALIVE_TIME:
			{
				//System.out.println("process, trackeralive_time");
				if(alive(tracker)){
					ev = new SimpleEvent(TRACKERALIVE_TIME);
					EDSimulator.add(1800000,ev,node,pid);
				}
				else
					tracker=null;
				
			}; break;
				
			case DOWNLOAD_COMPLETED:
			{
				nPiecesUp--;
			}; break;
				
		}
	}
	
	/**
	 *	Given a piece index and a block index it encodes them in an unique integer value.
	 *	@param piece the index of the piece to encode.
	 *	@param block the index of the block to encode.
	 *	@return the encoding of the piece and the block indexes.
	 */
	private int encode(int piece, int block){
		return (piece*100)+block;
		
	}
	/** 
	 *	Returns either the piece or the block that contained in the <tt>value</tt> depending
	 *	on <tt>part</tt>: 0 means the piece value, 1 the block value.
	 *	@param value the ID of the block to decode.
	 *	@param part the information to extract from <tt>value</tt>. 0 means the piece index, 1 the block index.
	 *	@return the piece or the block index depending about the value of <tt>part</tt>
	 */
	private int decode(int value, int part){
		if (value==-1) // Not a true value to decode
			return -1;
		if(part == 0) // I'm interested to the piece
			return value/100;
		else // I'm interested to the block
			return value%100;
	}
	
	/**
	 *	Used by {@link NodeInitializer#choosePieces(int, BitTorrent) NodeInitializer} to set 
	 *	the number of piece completed from the beginning in according with
	 *	the distribution in the configuration file.
	 *	@param number the number of piece completed
	 */
	public void setCompleted(int number){
		this.nPieceCompleted = number;
	}
	
	/**
	 *	Sets the status (the set of blocks) of the file for the current node.
	 *  Note that a piece is considered <i>completed</i> if the number
	 *  of downloaded blocks is 16.
	 *  @param index The index of the piece
	 *  @param value Number of blocks downloaded for the piece index.
	 */
	public void setStatus(int index, int value){
		status[index]=value;
	}
	
	/**
	 *	Sets the status of the local node.
	 *	@param status The status of the node: 1 means seeder, 0 leecher
	 */
	public void setPeerStatus(int status){
		this.peerStatus = status;
	}
	
	/**
	 *	Gets the status of the local node.
	 *	@return The status of the local node: 1 means seeder, 0 leecher
	 */
	public int getPeerStatus(){
		return peerStatus;
	}
	
	/**
	 *  Gets the number of blocks for a given piece owned by the local node.
	 *	@param index The index of the piece
	 *	@return Number of blocks downloaded for the piece index
	 */
	public int getStatus(int index){
		return status[index];	
	}
	
	/**
	 *	Sets the maximum bandwdith for the local node.
	 *	@param value The value of bandwidth in Kbps
	 */
	public void setBandwidth(int value){
		maxBandwidth = value;
	}
	
	/**
	 *	Checks if a node is still alive in the simulated network.
	 *	@param node The node to check
	 *	@return true if the node <tt>node</tt> is up, false otherwise
	 *	@see peersim.core.GeneralNode#isUp
	 */
	public boolean alive(Node node){
		if(node == null)
			return false;
		else
			return node.isUp();
	}
		
	/**	
	 *	Adds a neighbor to the cache of the local node.
	 *  The new neighbor is put in the first null position.
	 *	@param neighbor The neighbor node to add
	 *  @return <tt>false</tt> if the neighbor is already present in the cache (this can happen when the peer requests a
	 *	new peer set to the tracker an there is still this neighbor within) or no place is available.
	 *	Otherwise, returns true if the node is correctly added to the cache.
	 */
	public boolean addNeighbor(Node neighbor){
		if(search(neighbor.getID()) !=null){// if already exists
		//	System.err.println("Node "+neighbor.getID() + " not added, already exist.");
			return false;
		}
		if(this.tracker == null){ // I'm in the tracker's BitTorrent protocol
			for(int i=0; i< nMaxNodes+maxGrowth; i++){
				if(cache[i].node == null){
					cache[i].node = neighbor;
					cache[i].status = 0; //chocked
					cache[i].interested = -1; //not interested
					this.nNodes++;
					
					//System.err.println("i: " + i +" nMaxNodes: " + nMaxNodes);
					return true;
				}
			}
		}
		else{
			if((nNodes+nBitfieldSent) < swarmSize){
				//System.out.println("I'm the node " + this.thisNodeID + ", trying to add node "+neighbor.getID());
				for(int i=0; i<swarmSize; i++){
					if(cache[i].node == null){
						cache[i].node = neighbor;
						cache[i].status = 0; //choked
						cache[i].interested = -1; // not interested
						byPeer[nNodes].peer = i;
						byPeer[nNodes].ID = neighbor.getID();
						sortByPeer();
						this.nNodes++;
						//System.out.println(neighbor.getID()+" added!");
						return true;
					}
				}
				System.out.println("Node not added, no places available");
			}
		}
		return false;
	}
	
	/**
	 *	Removes a neighbor from the cache of the local node.
	 *	@param neighbor The node to remove
	 *	@return true if the node is correctly removed, false otherwise.
     */
	public boolean removeNeighbor(Node neighbor) {
		
		if (neighbor == null)
			return true;
		
		// this is the tracker's bittorrent protocol
		if (this.tracker == null) {
			for (int i=0; i< (nMaxNodes+maxGrowth); i++) {
				
				// check the feasibility of the removal
				if ( (cache[i] != null) && (cache[i].node != null) &&
					 (cache[i].node.getID() == neighbor.getID()) ) {
					cache[i].node = null;
					this.nNodes--;
					return true;
				}
			}
			return false;
		}
		// this is the bittorrent protocol of a peer
		else {
			
			Element e = search(neighbor.getID());
			
			if (e != null) {
				for (int i=0; i<nPieces; i++) {
					rarestPieceSet[i] -= swarm[e.peer][i];
					swarm[e.peer][i] = 0;
				}
				
				cache[e.peer].node = null;
				cache[e.peer].status = 0;
				cache[e.peer].interested = -1;
				unchokedBy[e.peer] = false;
				this.nNodes--;
				e.peer = -1;
				e.ID = Integer.MAX_VALUE;
				e.valueUP = 0;
				e.valueDOWN = 0;
				e.head20 = 0;
				e.head60 = 0;
				sortByPeer();
				
				return true;
			}
		}
		return false;
	}
	
	/**
     *	Adds a request to the pendingRequest queue.
	 *	@param block The requested block
	 *	@return true if the request has been successfully added to the queue, false otherwise
	 */
	private boolean addRequest(int block){
		int i=4;
		while(i>=0 && pendingRequest[i]!=-1){
			i--;
		}
		if(i>=0){
			pendingRequest[i] = block;
			return true;
		}
		else { // It should never happen
			   //System.err.println("pendingRequest queue full");
			return false;		
		}
	}
	
	/**
	 *	Removes the block with the given <tt>id</tt> from the {@link #pendingRequest} queue
	 *  and sorts the queue leaving the empty cell at the left.
	 *	@param id the id of the requested block
	 */
	private void removeRequest(int id){
		int i = 4;
		for(; i>=0; i--){
			if(pendingRequest[i]==id)
				break;
		}
		for(; i>=0; i--){
			if(i==0)
				pendingRequest[i] = -1;
			else
				pendingRequest[i] = pendingRequest[i-1];
		}
	}
	
	/**
	 *	Requests new block until the {@link #pendingRequest} is full to the sender of the just received piece.
	 *	It calls {@link #getNewBlock(Node, int)} to implement the <i>strict priority</i> strategy. 
	 *	@param node the local node
	 *	@param pid the BitTorrent protocol id
	 *	@param sender the sender of the just received received piece. 
	 */
	private void requestNextBlocks(Node node, int pid, int sender){
		int block = getNewBlock(node, pid);
		while(block != -2){
			if(unchokedBy[sender]==true && alive(cache[sender].node) && addRequest(block)){
				Object ev = new IntMsg(REQUEST, node, block,0);
				
//				long latency = ((Transport)node.getProtocol(tid)).getLatency(node,cache[sender].node);
//				EDSimulator.add(latency,ev,cache[sender].node,pid);
				
				((Transport) node.getProtocol(tid)).send(node,cache[sender].node, ev, pid);
				cache[sender].justSent();
			}
			else{ // I cannot send request
				if(!alive(cache[sender].node) && cache[sender].node!=null){
					System.out.println("piece2 rm neigh "+ cache[sender].node.getID() );
					removeNeighbor(cache[sender].node);
					processNeighborListSize(node,pid);
				}
				return;
			}
			block = getNewBlock(node, pid);
		}
	}
	
	/**
	 *	It returns the id of the next block to request. Sends <tt>INTERESTED</tt> if the new
	 *	block belongs to a new piece.
	 *	It uses {@link #getBlock()} to get the next block of a piece and calls {@link #getPiece()}
	 *	when all the blocks for the {@link #currentPiece} have been requested.
	 *	@param node the local node
	 *	@param pid the BitTorrent protocol id
	 *	@return -2 if no more places available in the <tt>pendingRequest</tt> queue;<br/>
	 *			the value of the next block to request otherwise</p>
	 */
	private int getNewBlock(Node node, int pid){
		int block = getBlock();
		if(block < 0){ // No more block to request for the current piece 
			
			if(block ==-2) // Pending request queue full
				return -2;
			
			int newPiece = getPiece();
			if(newPiece == -1){ // no more piece to request
				return -2;
			}
			
			lastInterested = newPiece;
			Object ev = new IntMsg(INTERESTED, node, lastInterested,0);
			
			for(int j=0; j<swarmSize; j++){// send the interested message to those  
									// nodes which have that piece
				if(alive(cache[j].node) && swarm[j][newPiece]==1){
//					long latency = ((Transport)node.getProtocol(tid)).getLatency(node,cache[j].node);
//					EDSimulator.add(latency,ev,cache[j].node,pid);
					((Transport) node.getProtocol(tid)).send(node,cache[j].node, ev, pid);
					cache[j].justSent();
				}
				if(!alive(cache[j].node)){
					//System.out.println("piece1 rm neigh "+ cache[j].node.getID() );
					
					removeNeighbor(cache[j].node);
					processNeighborListSize(node,pid);
				}
			}
			block = getBlock();
			return block;
		}
		else{
			// block value referred to a real block
			return block;
		}
	}
	
	/**
	 *	Returns the next block to request for the {@link #currentPiece}.
	 *	@return an index of a block of the <tt>currentPiece</tt> if there are still 
	 *			available places in the {@link #pendingRequest} queue;<br/>
	 *			-2 if the <tt>pendingRequest</tt> queue is full;<br/>
	 *			-1 if no more blocks to request for the current piece. 
	 */
	private int getBlock(){
		int i=4;
		while(i>=0 && pendingRequest[i]!=-1){ // i is the first empty position from the head
			i--;
		}
		if(i==-1){// No places in the pendingRequest available
				  //System.out.println("Pendig request queue full!");
			return -2;
		}
		int j;
		//The queue is not empty & last requested block belongs to lastInterested piece 
		if(i!=4 && decode(pendingRequest[i+1],0)==lastInterested)
			j=decode(pendingRequest[i+1],1)+1; // the block following the last requested
		else // I don't know which is the next block, so I search it.
			j=0; 
		/*	I search another block until the current has been already received. 
			*	If in pieceStatus at position j there is a block that belongs to
			*	lastInterested piece, means that the block j has been already
			*	received, otherwise I can request it.
			*/
		while(j<16 && decode(pieceStatus[j],0)==lastInterested){
			j++;
		}
		if(j==16) // No more block to request for lastInterested piece
			return -1;
		return encode(lastInterested,j);
	}
	
	/**
	 *	Returns the next correct piece to download. It choose the piece by using the
	 *	<i>random first</i> and <i>rarest first</i> policy. For the beginning 4 pieces
	 *	of a file the first one is used then the pieces are chosen using <i>rarest first</i>.
	 *	@see "Documentation about the BitTorrent module"
	 *	@return the next piece to download. If the whole file has been requested
	 *	-1 is returned.
	 */
	private int getPiece(){
		int piece = -1;
		if(nPieceCompleted < 4){ //Uses random first piece
			piece = CommonState.r.nextInt(nPieces);
			while(status[piece]==16 || piece == currentPiece) // until the piece is owned
				piece = CommonState.r.nextInt(nPieces);
			return piece;
		}
		else{ //Uses rarest piece first
			int j=0;
			for(; j<nPieces; j++){ // I find the first not owned piece
				if(status[j]==0){
					piece = j;
					if(piece != lastInterested) // teoretically it works because
												// there should be only one interested 
												// piece not yet downloaded
						break;
				}
			}
			if(piece==-1){ // Never entered in the previous 'if' statement; for all
						   // pieces an has been sent
				return -1;
			}
			
			int rarestPieces[] = new int[nPieces-j]; // the pieces with the less number of occurences\
			rarestPieces[0] = j;
			int nValues = 1; // number of pieces less distributed in the network
			for(int i=j+1; i<nPieces; i++){ // Finds the rarest piece not owned
				if(rarestPieceSet[i]< rarestPieceSet[rarestPieces[0]] && status[i]==0){ // if strictly less than the current one
					rarestPieces[0] = i; 
					nValues = 1;
				}
				if(rarestPieceSet[i]==rarestPieceSet[rarestPieces[0]] && status[i]==0){ // if equal
					rarestPieces[nValues] = i;
					nValues++;
				}
			}
			
			piece = CommonState.r.nextInt(nValues); // one of the less owned pieces
			return rarestPieces[piece];
		}
	}
	
	/**
	 *	Returns the file's size as number of pieces of 256KB.
	 *	@return number of pieces that compose the file.
	 */
	public int getNPieces(){
		return nPieces;	
	}
	/**
	 *	Clone method of the class. Returns a deep copy of the BitTorrent class. Used
	 *	by the simulation to initialize the {@link peersim.core.Network}
	 *	@return the deep copy of the BitTorrent class.
	 */
	public Object clone(){
		Object prot = null;
		try{
			prot = (BitTorrent)super.clone();
		}
		catch(CloneNotSupportedException e){};
		
		((BitTorrent)prot).cache = new Neighbor[swarmSize];
		for(int i=0; i<swarmSize; i++){
			((BitTorrent)prot).cache[i] = new Neighbor();
		}
		
		((BitTorrent)prot).byPeer = new Element[swarmSize];
		for(int i=0; i<swarmSize; i++){
			((BitTorrent)prot).byPeer[i] = new Element();
		}
		
		((BitTorrent)prot).unchokedBy = new boolean[swarmSize];
		
		((BitTorrent)prot).byBandwidth = new Element[swarmSize];
		((BitTorrent)prot).status = new int[nPieces];
		((BitTorrent)prot).pieceStatus = new int[16];
		for(int i=0; i<16;i++)
			((BitTorrent)prot).pieceStatus[i] = -1;
		((BitTorrent)prot).pendingRequest = new int[5];
		for(int i=0; i<5;i++)
			((BitTorrent)prot).pendingRequest[i] = -1;
		((BitTorrent)prot).rarestPieceSet = new int[nPieces];
		for(int i=0; i<nPieces;i++)
			((BitTorrent)prot).rarestPieceSet[i] = 0;
		((BitTorrent)prot).swarm = new int[swarmSize][nPieces];
		((BitTorrent)prot).requestToServe = new Queue(20);
		((BitTorrent)prot).incomingPieces = new Queue(100);
		return prot;
	}
	
	/**
	 *	Sorts {@link #byPeer} array by peer's ID. It implements the <i>InsertionSort</i>
	 *	algorithm. 
	 */
	public void sortByPeer(){
		int i;
		
		for(int j=1; j<swarmSize; j++)   // out is dividing line
		{
			Element key = new Element(); 
			byPeer[j].copyTo(key) ;    // remove marked item
			i = j-1;           // start shifts at out
			while(i>=0 && (byPeer[i].ID > key.ID)) // until one is smaller,
			{
				byPeer[i].copyTo(byPeer[i+1]);      // shift item right,
				i--;            // go left one position
			}
			key.copyTo(byPeer[i+1]);         // insert marked item
		} 
		
	}
	
	/**
	 *	Sorts the array {@link #byBandwidth} using <i>QuickSort</i> algorithm.
	 *	<tt>null</tt> elements and seeders are moved to the end of the array.
	 */
	public void sortByBandwidth() {
        quicksort(0, swarmSize-1);
    }
	
	/**
	 *	Used by {@link #sortByBandwidth()}. It's the implementation of the
	 *	<i>QuickSort</i> algorithm.
	 *	@param left the leftmost index of the array to sort.
	 *	@param right the rightmost index of the array to sort.
	 */
	private void quicksort(int left, int right) {
        if (right <= left) return;
        int i = partition(left, right);
        quicksort(left, i-1);
        quicksort(i+1, right);
    }
	
	/**
	 *	Used by {@link #quicksort(int, int)}, partitions the subarray to sort returning
	 *	the splitting point as stated by the <i>QuickSort</i> algorithm.
	 *	@see "The <i>QuickSort</i> algorithm".
	 */
	private int partition(int left, int right) {
        int i = left - 1;
        int j = right;
        while (true) {
            while (greater(byBandwidth[++i], byBandwidth[right]))      // find item on left to swap
                ;                               // a[right] acts as sentinel
            while (greater(byBandwidth[right], byBandwidth[--j])) {      // find item on right to swap
                if (j == left) break;  // don't go out-of-bounds
			}
            if (i >= j) break;                  // check if pointers cross
            swap(i, j);                      // swap two elements into place
        }
        swap(i, right);                      // swap with partition element
        return i;
    }
	
	/**
	 *	Aswers to the question "is x > y?". Compares the {@link Element}s given as 
	 *	parameters. <tt>Element x</tt> is greater than <tt>y</tt> if isn't <tt>null</tt>
	 *	and in the last 20 seconds the local node has downloaded ("uploaded" if the local node is a 
	 *	seeder) more blocks than from <tt>y</tt>.
	 *	@param x the first <tt>Element</tt> to compare.
	 *	@param y the second <tt>Element</tt> to compare
	 *	@return <tt>true</tt> if x > y;<br/>
	 *			<tt>false</tt> otherwise.
	 */
    private boolean greater(Element x, Element y) {
		/*
		 * Null elements and seeders are shifted at the end
		 * of the array
		 */
		if (x==null) return false;
		if (y==null) return true;
		if (x.isSeeder) return false;
		if (y.isSeeder) return true;
		
		// if the local node is a leecher
		if (peerStatus==0) {
			if ((x.valueDOWN - x.head20) >
				(y.valueDOWN -y.head20))
				return true;
			else return false;
		}
		
		// if peerStatus==1 (the local node is a seeder)
		else {			
			if ((x.valueUP - x.head20) >
				(y.valueUP -y.head20))
				return true;
			else return false;
		}
    }
	
	/**
	 *	Swaps {@link Element} <tt>i</tt> with <tt>j</tt> in the {@link #byBandwidth}.<br/>
	 *	Used by {@link #partition(int, int)}
	 *	@param i index of the first element to swap
	 *	@param j index of the second element to swap
	 */
	private void swap(int i, int j) {
        Element swap = byBandwidth[i];
        byBandwidth[i] = byBandwidth[j];
        byBandwidth[j] = swap;
    }
	
	/**	Searches the node with the given ID. It does a dychotomic 
	 *	search.
	 *	@param ID ID of the node to search.
	 *	@return the {@link Element} in {@link #byPeer} which represents the node with the
	 *	given ID.
	 */
	public Element search(long ID){
		int low = 0;
		int high = swarmSize-1;
		int p = low+((high-low)/2);              //Initial probe position
		while ( low <= high) {
			if ( byPeer[p] == null || byPeer[p].ID > ID)
				high = p - 1;
			else { 
				if( byPeer[p].ID < ID )  //Wasteful second comparison forced by syntax limitations.
					low = p + 1;
				else
					return byPeer[p];
			}
			p = low+((high-low)/2);         //Next probe position.
		}
		return null; 	
	}
}

/**
 *	This class is used to store the main informations about a neighbors regarding
 *	the calculation of the Downloading/Uploading rates. Is the class of items in 
 *	{@link example.bittorrent.BitTorrent#byPeer} and {@link example.bittorrent.BitTorrent#byBandwidth}.
 */
class Element{
	/**
	 *	ID of the represented node.
	 */
	public long ID = Integer.MAX_VALUE;
	/**
	 *	Index position of the node in the {@link example.bittorrent.BitTorrent#cache} array.
	 */
	public int peer = -1; 
	/**
	 *	Number of blocks uploaded to anyone since the beginning.
	 */
	public int valueUP = 0;
	/**
	 *	Number of blocks downloaded from anyone since the beginning.
	 */
	public int valueDOWN = 0; 
	/**
	 *	Value of either {@link #valueUP} or {@link #valueDOWN} (depending by 
	 *	{@link example.bittorrent.BitTorrent#peerStatus}) 20 seconds before.
	 */
	public int head20 = 0;
	/**
	 *	Value of either {@link #valueUP} or {@link #valueDOWN} (depending by 
	 *	{@link example.bittorrent.BitTorrent#peerStatus}) 60 seconds before.
	 */
	public int head60 = 0; 
	/**
	 *	<tt>true</tt> if the node is a seeder, <tt>false</tt> otherwise.
	 */
	public boolean isSeeder = false;
	/**
	 *	Makes a deep copy of the Element to <tt>destination</tt>
	 *	@param destination Element instance where to make the copy
	 */
	public void copyTo(Element destination){
		destination.ID = this.ID;
		destination.peer = this.peer;
		destination.valueUP = this.valueUP;
		destination.valueDOWN = this.valueDOWN;
		destination.head20 = this.head20;
		destination.head60 = this.head60;
	}
}

/**
 *	This class stores information about the neighbors regarding their status. It is 
 *	the type of the items in the {@link example.bittorrent.BitTorrent#cache}.
 */
class Neighbor{
	/**
	 *	Reference to the node in the {@link peersim.core.Network}.
	 */
	public Node node = null;
	/**
	 *	-1 means not interested<br/>
	 *	Other values means the last piece number for which the node is interested.
	 */
	public int interested;
	/**
	 *	0 means CHOKED<br/>
	 *	1 means UNCHOKED<br/>
	 *	2 means SNUBBED_BY. If this value is set and the node is to be unchocked,
	 *	value 2 has the priority.
	 */
	public int status;
	/**
	 *	Last time a message from the node represented has been received.
	 */
	public long lastSeen = 0; 
	/**
	 *	Last time a message to the node represented has been sent.
	 */
	public long lastSent = 0;
	
	/**
	 * Sets the last time the neighbor was seen.
	 */
	public void isAlive(){
		long now = CommonState.getTime();
		this.lastSeen = now;
	}
	
	/*
	 * Sets the last time the local peer sent something to the neighbor.
	 */
	public void justSent(){
		long now = CommonState.getTime();
		this.lastSent = now;
	}
	
}

/**
 *	Class type of the queues's items in {@link example.bittorrent.BitTorrent#incomingPieces} 
 *	and {@link example.bittorrent.BitTorrent#requestToServe}.
 */
class Queue{
	int maxSize;
	int head = 0;
	int tail = 0;
	int dim = 0;
	Request queue[];
	
	/**
	 *	Public constructor. Creates a queue of size <tt>size</tt>.
	 */
	public Queue(int size){
		maxSize = size;
		queue = new Request[size];
		for(int i=0; i< size; i++)
			queue[i]= new Request();
	}
	
	/**
	 *	Enqueues the request of the block <tt>id</tt> and its <tt>sender</tt>
	 *	@param id the id of the block in the request
	 *	@param sender a reference to the sender of the request
	 *	@return <tt>true</tt> if the request has been correctly added, <tt>false</tt>
	 *	otherwise.
	 */
	public boolean enqueue(int id, Node sender){
		if(dim < maxSize){
			queue[tail%maxSize].id = id;
			queue[tail%maxSize].sender = sender;
			tail++;
			dim++;
			return true;
		}
		else return false;
	}
	
	/**
	 *	Returns the {@link Request} in the head of the queue.
	 *	@return the element in the head.<br/>
	 *			<tt>null</tt> if the queue is empty.
	 */
	public Request dequeue(){
		Request value;
		if(dim > 0){
			value = queue[head%maxSize];
			head++;
			dim--;
			return value;
		}
		else return null; //empty queue
	}
	
	/**
	 *	Returns the status of the queue.
	 *	@return <tt>true</tt> if the queue is empty, <tt>false</tt>
	 *	otherwise.
	 */
	public boolean empty(){
		return (dim == 0);
	}
	
	/**
	 *	Returns <tt>true</tt> if block given as parameter is in.
	 *	@param	value the id of the block to search.
	 *	@return <tt>true</tt> if the block <tt>value</tt> is in the queue, <tt>false</tt>
	 *	otherwise.
	 */
	public boolean contains(int value){
		if(empty())
			return false;
		for(int i=head; i<head+dim; i++){
			if(queue[i%maxSize].id == value)
				return true;
		}
		return false;
	}
	
	/**
	 *	Removes a request from the queue.
	 *	@param sender the sender of the request.
	 *	@param value the id of the block requested.
	 *	@return <tt>true</tt> if the request has been correctly removed, <tt>false</tt>
	 *	otherwise.
	 */
	public boolean remove(Node sender, int value){
		if(empty())
			return false;
		for(int i=head; i<head+dim; i++){
			if(queue[i%maxSize].id == value && queue[i%maxSize].sender == sender){
				for(int j=i; j>head; j--){ // Shifts the elements for the removal
					queue[j%maxSize]= queue[(j-1)%maxSize];
				}
				head++;
				dim--;
				return true;
			}
		}
		return false;
	}
}

/**
 *	This class represent an enqueued request of a block.
 */
class Request{
	/**
	 *	The id of the block.
	 */
	public int id;
	/**
	 *	The sender of the request.
	 */
	public Node sender;
}