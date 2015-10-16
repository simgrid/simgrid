-- Copyright (c) 2012, 2014. The SimGrid Team.
-- All rights reserved.

-- This program is free software; you can redistribute it and/or modify it
-- under the terms of the license (GNU LGPL) which comes with this package.

-- A SimGrid Lua implementation of the Bittorrent protocol.

require("simgrid")

common_tracker = {
	MAXIMUM_PEERS = 50
}



function tracker(...)
	tracker_data = {
		peers_list = {},
		deadline = 0,
		comm_received = nil
	}
  -- Check the arguments
  local args = {...}
  if #args ~= 1 then
  	simgrid.info("Wrong number of arguments for the tracker")
  end
  -- Retrieve the end time
  tracker_data.deadline = tonumber(args[1])
  
  simgrid.info("Tracker launched")
  
  local now = simgrid.get_clock()
  
  tracker_data.comm_received = simgrid.task.irecv("tracker")
  while now < tracker_data.deadline do
  	task, err = tracker_data.comm_received:test()
	if task then
  		simgrid.debug("Received a request from " .. task.mailbox)
  		tracker_data.comm_received = simgrid.task.irecv("tracker")
		-- Sending peers to the peer
		local peers = {}
		local i = 0 
		if #tracker_data.peers_list > 0 then
			i = math.random(1,#tracker_data.peers_list)
		end
		while #peers < #tracker_data.peers_list and #peers < common_tracker.MAXIMUM_PEERS do
			table.insert(peers,tracker_data.peers_list[i])
			i = (i % #tracker_data.peers_list) +1
		end
		task.type = "ANSWER"
		task.peers = peers
  		-- Add the peer to our peer list
  		table.insert(tracker_data.peers_list,task.peer_id)
		-- Setting the interval
		task.interval = TRACKER_QUERY_INTERVAL
		-- Sending the task back to the peer
		task:dsend(task.mailbox)
 	else
  		simgrid.process.sleep(1)
  		now = simgrid.get_clock()
  	end
  end
  
  simgrid.info("Tracker is leaving")
end
