-- A SimGrid Lua implementation of the Bittorrent protocol.

require("simgrid")
-- Common Constants
common = {
	FILE_SIZE = 5120,
	FILE_PIECE_SIZE = 512,
	FILE_PIECES = 10,
	
	PIECE_COMM_SIZE = 1,
	
	MESSAGE_SIZE = 1,
	MAXIMUM_PEERS = 50,
	
	TRACKER_QUERY_INTERVAL = 1000,
	TRACKER_COMM_SIZE = 0.01,
	
	GET_PEERS_TIMEOUT = 10000,
	TIMEOUT_MESSAGE = 10,
	MAX_UNCHOKED_PEERS = 4,
	UPDATE_CHOKED_INTERVAL = 50,
	MAX_PIECES = 1,
}


-- Peer main function
function peer(...)
	
	local args = {...}
	
	if #args ~= 2 and #args ~= 3 then
		simgrid.info("Wrong number of arguments")
	end
			
	-- Setting the peer data
	data = {
			-- Retrieving the peer id
			id = tonumber(args[1]),
			mailbox = tostring(tonumber(args[1])),
			mailbox_tracker = "tracker" .. args[1],
			peers = {},
			active_peers = {},
			current_pieces = {},
			pieces_requested = 0,
			bitfield = {},
			pieces_count = {},
			pieces = 0,
			deadline = tonumber(args[2]),
			round = 0
	}
	simgrid.info("Hi, I'm joining the network with id " .. data.id)

	-- Checking if the peer is a seed
	local bitfield_value = false
	if args[3] == "1" then
		data.pieces = common.PIECES_COUNT
		bitfield_value = true
	else
		data.pieces = 0
	end
	-- Building the peer bitfield and the pieces list
	for i = 1, common.FILE_PIECES do
		data.pieces_count[i] = 0	
		data.bitfield[i] = bitfield_value
	end
	
	if get_peers_data() == true then
		data.comm_received = simgrid.task.irecv(data.mailbox)
		if has_finished() then
			send_handshake_all()
			seed_loop()
		else
			leech_loop()
			seed_loop()
		end
	else
		simgrid.info("Couldn't contact the tracker")
	end
	simgrid.info("My status is now " .. get_status())
end
-- Peer main loop when it is leeching
function leech_loop()
	simgrid.info("Start downloading.")
	local now = simgrid.get_clock()
	local next_choked_update = now + common.UPDATE_CHOKED_INTERVAL;
	-- Send a "handshake" message to all the peers it got
	-- it couldn't have gotten more than 50 peers anyway)
	send_handshake_all()
	-- Wait for at leaast one bitfield message
	wait_for_pieces()
	
	simgrid.info("Starting main leech loop")
	local task, err
	while now < data.deadline and data.pieces < common.FILE_PIECES do
		task, err = data.comm_received:test()
		if task then
			handle_message(task)
			data.comm_received = simgrid.task.irecv(data.mailbox)
			now = simgrid.get_clock()
		elseif err then
			data.comm_received = simgrid.task.irecv(data.mailbox)		
		else
			-- If the user has a pending interesting
			if data.current_piece ~= -1 then
				send_interested_to_peers()
			else
				if table.getn(data.current_pieces) < common.MAX_PIECES then
					update_current_piece()
				end
			end
			-- We don't execute the choke algorithm if we don't already have a piece
			if now >= next_choked_update and data.pieces > 0 then
				update_choked_peers()
				next_choked_update = next_choked_update + common.UPDATE_CHOKED_INTERVAL
				now = simgrid.get_clock()
			else
				simgrid.process.sleep(1)
				now = simgrid.get_clock()
			end
		end		
	end
end
-- Peer main loop when it is seeding
function seed_loop()
	local now = simgrid.get_clock()
	local next_choked_update = now + common.UPDATE_CHOKED_INTERVAL;
	simgrid.debug("Start seeding.")
	-- Start the main seed loop
	while now < data.deadline do
		task, err = data.comm_received:test()		
		if task then
			handle_message(task)
			data.comm_received = simgrid.task.irecv(data.mailbox)		
			now = simgrid.get_clock()
		elseif err then
			data.comm_received = simgrid.task.irecv(data.mailbox)		
		else
			if now >= next_choked_update then
				update_choked_peers()
				next_choked_update = next_choked_update + common.UPDATE_CHOKED_INTERVAL
				now = simgrid.get_clock()
			else
				simgrid.process.sleep(1)
				now = simgrid.get_clock()
			end		
		end
	end
end
-- Retrieve the peers list from the tracker
function get_peers_data() 
	local success = false
	local now = simgrid.get_clock()
	local timeout = now + common.GET_PEERS_TIMEOUT
	-- Build the task
	local task_send = simgrid.task.new("", 0, common.MESSAGE_SIZE)
	task_send.type = "REQUEST"
	task_send.peer_id = data.id
	task_send.mailbox = data.mailbox_tracker
	-- Send the task
	while not(success) and now < timeout do
		simgrid.debug("Sending a peer request to the tracker.")
		if task_send:send("tracker") then
			success = true
		end
	end
	now = simgrid.get_clock()
	success = false
	-- Wait for the answer
	local comm_received = simgrid.task.irecv(data.mailbox_tracker)
	while not(success) and now < timeout do
		local task_received = comm_received:wait(timeout)
		comm_received = simgrid.task.irecv(data.mailbox_tracker)
		if task_received then
			simgrid.info("Received an answer from the tracker with " .. #task_received.peers .. " peers inside")
			-- Add what we received to our peer list
			for i,v in pairs(task_received.peers) do
				if v ~= data.id then
					--Add the peer to our list and build its data
					local peer_data = {}
					peer_data.id = v;
					peer_data.bitfield = nil
					peer_data.mailbox = tostring(v);
					peer_data.am_interested = false
					peer_data.interested = false
					peer_data.choked_upload = true
					peer_data.choked_download = true
					data.peers[v] = peer_data
				end
			end
		else
			success = false
		end
		success = true
	end
	return success;	
end
-- Returns if the peer has finished downloading the piece
function has_finished() 
	for i,v in pairs(data.bitfield) do
		if v == false then
			return false
		end
	end
	return true
end
-- Handle a received message sent by another peer
function handle_message(task)
	local remote_peer = data.peers[task.peer_id]
	
	if task.type == "HANDSHAKE" then
		simgrid.debug("Received a HANDSHAKE message from " .. task.mailbox)
		-- Check if the peer is in our connection list
		if data.peers[task.peer_id] == nil then
			local peer_data = {}
			peer_data.mailbox = task.mailbox
			peer_data.id = task.peer_id
			peer_data.am_interested = false
			peer_data.interested = false
			peer_data.choked_upload = true
			peer_data.choked_download = true
			peer_data.bitfield = nil
			data.peers[task.peer_id] = peer_data
			send_handshake(task.mailbox)
		end
		-- Send our bitfield to the peer
		send_bitfield(task.mailbox)
	elseif task.type == "BITFIELD" then
		simgrid.debug("Received a BITFIELD from " .. task.mailbox)
		-- Update the pieces list
		update_piece_count_from_bitfield(task.bitfield)
		-- Update the current piece
		if data.current_piece == -1 and data.pieces < common.FILE_PIECES then
			update_current_piece()
		end
		data.peers[task.peer_id].bitfield = task.bitfield
	elseif task.type == "INTERESTED" then
		simgrid.debug("Received an INTERESTED message from " .. task.mailbox)
		data.peers[task.peer_id].interested = true
	elseif task.type == "NOTINTERESTED" then
		simgrid.debug("Received an NOTINTERESTED message from " .. task.mailbox)
		data.peers[task.peer_id].interested = false
	elseif task.type == "UNCHOKE" then
		simgrid.debug("Received an UNCHOKE message from " .. task.mailbox)
		data.peers[task.peer_id].choked_download = false
		send_requests_to_peer(data.peers[task.peer_id])
	elseif task.type == "CHOKE" then
		simgrid.debug("Recevied a CHOKE message from " .. task.mailbox)
		data.peers[task.peer_id].choked_download = true		
	elseif task.type == "HAVE" then
		local remote_peer = data.peers[task.peer_id] 
		if remote_peer == nil or remote_peer.bitfield == nil then
			return
		end
		simgrid.debug("Received a HAVE message from " .. task.mailbox)
		data.pieces_count[task.piece] = data.pieces_count[task.piece] + 1
		-- Send interested message to the peer if he has what we want
		if not(remote_peer.am_interested) and data.current_pieces[task.piece] ~= nil then
			remote_peer.am_interested = true
			send_interested(remote_peer.mailbox)
		end
		if data.current_pieces[task.piece] ~= nil then
			send_request(task.mailbox,task.piece)
		end
	elseif task.type == "REQUEST" then
		simgrid.debug("Received REQUEST from " .. task.mailbox .. " for " .. task.piece)
		local remote_peer = data.peers[task.peer_id] 
		if remote_peer.choked_upload == false then
			if data.bitfield[task.piece] == true then
				send_piece(task.mailbox,task.piece,false)
			end
		end
	elseif task.type == "PIECE" then
		if task.stalled == true then
			simgrid.debug("The received piece is stalled")
		else
			simgrid.debug("Received piece " .. task.piece .. " from " .. task.mailbox)
			if data.bitfield[task.piece] ~= true then
				data.pieces_requested = data.pieces_requested - 1
				-- Removing the piece from our piece list
				data.current_pieces[task.piece] = nil
				data.bitfield[task.piece] = true
				data.pieces = data.pieces + 1
				simgrid.debug("My status is now:" .. get_status())
				-- Sending the information to all the peers we are connected to
				send_have(task.piece)
				-- Sending UNINTERESTED to the peers that doesn't have any more pieces
				update_interested_after_receive()
			end
		end
	end
end
-- Update the piece the peer is currently interested in.
-- There is two cases (as described in "Bittorrent Architecture Protocol", Ryan Toole :
-- If the peer has less than 3 pieces, he chooses a piece at random.
-- If the peer has more than pieces, he downloads the pieces that are the less
-- replicated
function update_current_piece() 
	if data.pieces_requested >= (common.FILE_PIECES - data.pieces) then
		return
	end
	if data.pieces < 3 or true then
		repeat
			data.current_piece = math.random(1,common.FILE_PIECES)
--			simgrid.info("The new piece is:" .. data.current_piece)
		until data.bitfield[data.current_piece] ~= true and data.current_pieces[data.current_piece] == nil
		data.current_pieces[data.current_piece] = true
		data.pieces_requested = data.pieces_requested + 1
	end		
	
end
-- Updates the list of who has a piece from a bitfield
function update_piece_count_from_bitfield(bitfield)
	for i,v in pairs(bitfield) do
		if v == true then	
			data.pieces_count[i] = data.pieces_count[i] + 1
		end
	end
end
-- Wait for the node to receive interesting bitfield messages (ie: non empty)
function wait_for_pieces() 
	local finished = false
	local now = simgrid.get_clock()
	local task
	while now < data.deadline and not(finished) do
		task = data.comm_received:wait(common.TIMEOUT_MESSAGE)
		if task then
			handle_message(task)
			if data.current_piece ~= -1 then
				finished = true
			end	
		end
		data.comm_received = simgrid.task.irecv(data.mailbox)		
	end
end
-- Update the list of current choked and unchoked peers, using the
-- choke algorithm
function update_choked_peers()
	data.round = (data.round + 1) % 3
	-- Remove a peer from the list
	for i,v in pairs(data.active_peers) do
		data.active_peers[i] = nil
		send_choked(v.mailbox)
		break
	end
	-- Random optimistic unchoking
	if true then
		local values = {}
		for key, value in pairs(data.peers) do
			values[#values + 1] = value
		end
		local peer_choosed = nil
		local j = 0

		repeat
			peer_choosed = values[math.random(#values)]		
			if peer_choosed.interested ~= true then
				peer_choosed = nil
			end
			j = j + 1
		until peer_choosed ~= nil or j < common.MAXIMUM_PEERS
		if peer_choosed ~= nil then
			data.active_peers[peer_choosed.id] = peer_choosed
			peer_choosed.choked_upload = false
			send_unchoked(peer_choosed.mailbox)
		end
	end
	-- TODO: Use the leecher choke algorithm
end
--  Updates our "interested" state about peers: send "not interested" to peers
--  that don't have any more pieces we want.
function update_interested_after_receive()
	local interested = false
	for i,v in pairs(data.peers) do
		if v.am_interested then
			for piece,j in pairs(data.current_pieces) do
				if v.bitfield ~= nil then
					if v.bitfield[piece] == true then
						interested = true
						break
					else
					end
				end
			end
			if not(interested) then
				v.am_interested = false
				send_not_interested(v.mailbox)
			end
		end
	end
end
-- Find the peers that have the current interested piece and send them
-- the "interested" message
function send_interested_to_peers()
	if data.current_piece == -1 then
		return
	end
	for i,v in pairs(data.peers) do
		if v.bitfield ~= nil then
			v.am_interested = true
			send_interested(v.mailbox)
		end
	end
	data.current_piece = -1
end 
-- Send a "interested" message to a peer.
function send_interested(mailbox)
	simgrid.debug("Sending a INTERESTED to " .. mailbox)
	local task = new_task("INTERESTED")
	task:dsend(mailbox)
end
-- Send a "not interested" message to a peer.
function send_not_interested(mailbox)
	local task = new_task("NOTINTERESTED")
	task:dsend(mailbox)
end
-- Send a handshake message to all the peers the peer has
function send_handshake_all()
	for i,v in pairs(data.peers) do
		local task = new_task("HANDSHAKE")
		task:dsend(v.mailbox)
	end
end
-- Send a "handshake" message to an user
function send_handshake(mailbox)
	simgrid.debug("Sending a HANDSHAKE to " .. mailbox)
	local task = new_task("HANDSHAKE")
	task:dsend(mailbox)		
end
-- Send a "choked" message to a peer
function send_choked(mailbox)
	simgrid.debug("Sending a CHOKE to " .. mailbox)
	local task = new_task("CHOKE")
	task:dsend(mailbox)	
end
-- Send a "unchoked" message to a peer
function send_unchoked(mailbox)
	simgrid.debug("Sending a UNCHOKE to " .. mailbox)
	local task = new_task("UNCHOKE")
	task:dsend(mailbox)	
end
-- Send a "HAVE" message to all peers we are connected to
function send_have(piece)
	for i,v in pairs(data.peers) do
		local task = new_task("HAVE")
		task.piece = piece
		task:dsend(v.mailbox)
	end
end
-- Send request messages to a peer that have unchoked us	
function send_requests_to_peer(remote_peer)
	for i,v in pairs(data.current_pieces) do
		send_request(remote_peer.mailbox,i)
	end
end
-- Send a bitfield message to a peer
function send_bitfield(mailbox)
	simgrid.debug("Sending a BITFIELD to " .. mailbox)
	local task = new_task("BITFIELD")
	task.bitfield = data.bitfield
	task:dsend(mailbox)
end
-- Send a "request" message to a pair, containing a request for a piece
function send_request(mailbox, piece)
	simgrid.debug("Sending a REQUEST to " .. mailbox .. " for " .. piece)
	local task =  new_task("REQUEST")
	task.piece = piece
	task:dsend(mailbox)
end	
-- Send a "piece" messageto a pair, containing a piece of the file
function send_piece(mailbox, piece, stalled)
	simgrid.debug("Sending the PIECE " .. piece .. " to " .. mailbox)
	local task = new_task("PIECE")
	task.piece = piece
	task.stalled = stalled
	task:dsend(mailbox)	
end
function new_task(type)
	local task = simgrid.task.new("", 0, common.MESSAGE_SIZE)
	task.type = type
	task.mailbox = data.mailbox
	task.peer_id = data.id	
	return task
end
function get_status()
	local s = ""
	for i,v in pairs(data.bitfield) do
		if v == true then
			 s = s .. '1'
		else
			s = s .. '0'
		end
	end
	return s
end