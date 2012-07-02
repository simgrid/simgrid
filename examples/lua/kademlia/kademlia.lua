-- A SimGrid Lua implementation of the Kademlia protocol.
require("simgrid")

-- Common constants
common = {
	COMM_SIZE = 1,
	RANDOM_LOOKUP_INTERVAL = 100,
	ALPHA = 3,
	IDENTIFIER_SIZE = 32,
	BUCKET_SIZE = 20,
	MAX_JOIN_TRIALS = 4,
	FIND_NODE_TIMEOUT = 10,
	FIND_NODE_GLOBAL_TIMEOUT = 50,
	PING_TIMEOUT = 35,
	MAX_STEPS = 10,
	JOIN_BUCKETS_QUERIES = 5
}
require("tools")
-- Routing table 
require("routing_table")

data = {
	id = -1,
	mailbox = "",
	deadline = 0,
	comm = nil,
	find_node_succedded = 0,
	find_node_failed = 0
	
}

function node(...) 
	local args = {...}
	
	if #args ~= 2 and #args ~= 3 then
		simgrid.info("Wrong argument count: " .. #args)
		return
	end
	data.id = tonumber(args[1])
	routing_table_init(data.id)
	data.mailbox = tostring(data.id)
	if #args == 3 then
		data.deadline = tonumber(args[3])
		simgrid.info("Hi, I'm going to join the network with the id " .. data.id .. " !")
		if join_network(tonumber(args[2])) then
			main_loop()
		else
			simgrid.info("Couldn't join the network")
		end
	else
		data.deadline = tonumber(args[2])
		routing_table_update(data.id)
		data.comm = simgrid.task.irecv(data.mailbox)		
		main_loop()
	end
	simgrid.info(data.find_node_succedded .. "/" .. (data.find_node_succedded + data.find_node_failed) .. " FIND_NODE have succedded");
	simgrid.process.sleep(10000)
end	
function main_loop()
	local next_lookup_time = simgrid.get_clock() + common.RANDOM_LOOKUP_INTERVAL
	local now = simgrid.get_clock() 
	while now < data.deadline do
		task,err = data.comm:test()
		if task then
			handle_task(task)
			data.comm = simgrid.task.irecv(data.mailbox)		
		elseif err then
			data.comm = simgrid.task.irecv(data.mailbox)				
		else
			if now >= next_lookup_time then
				random_lookup()
				next_lookup_time = next_lookup_time + common.RANDOM_LOOKUP_INTERVAL
				now = simgrid.get_clock()
			else
				simgrid.process.sleep(1)
				now = simgrid.get_clock()
			end
		end
	end
end
function random_lookup()
	find_node(0,true)
end
function join_network(id_known)
	local answer_got = false
	local time_begin = simgrid.get_clock()
	
	simgrid.debug("Joining the network knowing " .. id_known)
	
	routing_table_update(data.id)
	routing_table_update(id_known)
	
	-- Send a FIND_NODE to the node we know
	send_find_node(id_known,data.id)
	-- Wait for the answer
	local trials = 0
	
	data.comm = simgrid.task.irecv(data.mailbox)
	
	repeat 
		task,err = data.comm:test()
		if task then
			if task.type == "FIND_NODE_ANSWER" then
				answer_got = true
				local answer = task.answer
				-- Add the nodes we received to our routing table
				for i,v in pairs(answer.nodes) do
					routing_table_update(v.id)
				end
			else
				handle_task(task)				
			end
			data.comm = simgrid.task.irecv(data.mailbox)
		elseif err then	
			data.comm = simgrid.task.irecv(data.mailbox)
		else
			simgrid.process.sleep(1)
		end
	until answer_got or trials >= common.MAX_JOIN_TRIALS
	-- Second step: Send a FIND_NODE in a node in each bucket
	local bucket_id = find_bucket(id_known).id
	local i = 0
	while ((bucket_id - i) > 0 or (bucket_id + i) <= common.IDENTIFIER_SIZE) and i < common.JOIN_BUCKETS_QUERIES do
		if bucket_id - i > 0 then
			local id_in_bucket = get_id_in_prefix(data.id,bucket_id - i)
			find_node(id_in_bucket,false)
		end
		if bucket_id + i <= common.IDENTIFIER_SIZE then
			local id_in_bucket = get_id_in_prefix(data.id,bucket_id + i)
			find_node(id_in_bucket,false)
		end
		i = i + 1
	end
	return answer_got
end
-- Send a request to find a node in the node's routing table.
function find_node(destination, counts)
	local queries, answers
	local total_answers = 0
	total_queries = 0
	local nodes_added = 0
	local destination_found = false
	local steps = 0
	local timeout
	local global_timeout = simgrid.get_clock() + common.FIND_NODE_GLOBAL_TIMEOUT
	-- Build a list of the closest nodes we already know.
	local node_list = find_closest(destination)
	
	simgrid.debug("Doing a FIND_NODE on " .. destination)
	repeat
		answers = 0
		queries = send_find_node_to_best(node_list)
		nodes_added = 0
		timeout = simgrid.get_clock() + common.FIND_NODE_TIMEOUT
		steps = steps + 1
		repeat
			task, err = data.comm:test()
			if task then
				if task.type == "FIND_NODE_ANSWER" and task.answer.destination == destination then
					routing_table_update(task.sender_id)
					for i,v in pairs(task.answer.nodes) do
						routing_table_update(v.id)
					end
					nodes_added = merge_answer(node_list,task.answer)
				else
					handle_task(task)
				end
				data.comm = simgrid.task.irecv(data.mailbox)			
			elseif err then
				data.comm = simgrid.task.irecv(data.mailbox)
			else
				simgrid.process.sleep(1)
			end
			
		until answers >= queries or simgrid.get_clock() >= timeout
		if (#node_list.nodes > 0) then
			destination_found = (node_list.nodes[1].distance == 0)
		else
			destination_found = false
		end
	until destination_found or (nodes_added > 0 and answers == 0) or simgrid.get_clock() >= global_timeout or steps >= common.MAX_STEPS
	if destination_found then
		simgrid.debug("Find node on " .. destination .. " succedded")
		if counts then
			data.find_node_succedded = data.find_node_succedded + 1
		end
	else
		simgrid.debug("Find node on " .. destination .. " failed")
		if counts then
			data.find_node_failed = data.find_node_failed + 1
		end
	end

	return destination_found
end
-- Sends a "FIND_NODE" request (task) to a node we know.
function send_find_node(id, destination)
	simgrid.debug("Sending a FIND_NODE to " .. id .. " to find " .. destination);
	
	local task = simgrid.task.new("",0, common.COMM_SIZE)
	task.type = "FIND_NODE"
	task.sender_id = data.id
	task.destination = destination
		
	task:dsend(tostring(id))
	
end
-- Sends a "FIND_NODE" request to the best "alpha" nodes in a node
-- list
function send_find_node_to_best(node_list)
	destination = node_list.destination
	local i = 1
	while i <= common.ALPHA and i < #node_list.nodes do
		if data.id ~= node_list.nodes[i].id then
			send_find_node(node_list.nodes[i].id,destination)
		end
		i = i + 1
	end
	return i - 1
end
-- Handles an incomming task
function handle_task(task)
	routing_table_update(task.sender_id)
	if task.type == "FIND_NODE" then
		handle_find_node(task)
	elseif task.type == "PING" then
		handle_ping(task)
	end
end
function handle_find_node(task)
	simgrid.debug("Received a FIND_NODE from " .. task.sender_id)
	local answer = find_closest(task.destination)
	local task_answer = simgrid.task.new("",0, common.COMM_SIZE)
	task_answer.type = "FIND_NODE_ANSWER"
	task_answer.sender_id = data.id
	task_answer.destination = task.destination
	task_answer.answer = answer
	task_answer:dsend(tostring(task.sender_id))	
end	
function handle_ping(task)
	simgrid.info("Received a PING from " .. task.sender_id)
	local task_answer = simgrid.task.new("",0, common.COMM_SIZE)
	task_answer.type = "PING_ANSWER"
	task_answer.sender_id = data.id
	task_answer.destination = task.destination
	task_answer:dsend(tostring(task.sender_id))
end
function merge_answer(m1, m2)
	local nb_added = 0
	for i,v in pairs(m2.nodes) do
		table.insert(m1.nodes,v)
		nb_added = nb_added + 1
	end
	return nb_added
end
simgrid.platform(arg[1] or  "../../msg/msg_platform.xml")
simgrid.application(arg[2] or "kademlia.xml")
simgrid.run()
