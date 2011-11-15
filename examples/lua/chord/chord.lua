-- A SimGrid Lua implementation of the Chord DHT

require("simgrid")

nb_bits = 24
nb_keys = 2^nb_bits
comp_size = 0
comm_size = 10
timeout = 50
max_simulation_time = 1000
stabilize_delay = 20
fix_fingers_delay = 120
check_predecessor_delay = 120
lookup_delay = 10

-- current node (don't worry, globals are duplicated in each process)
my_node = {
  id = my_id,
  next_finger_to_fix = 1,
  fingers = {},
  predecessor = nil,
  comm_recv = nil
}

-- Main function of each Chord process
-- Arguments:
-- - my id
-- - the id of a guy I know in the system (except for the first node)
function node(my_id, known_id)

  simgrid.debug("Hello, I'm a node with id " .. my_id .. " and I know " .. known_id)

  -- join the ring
  local success = false
  if known_id == nil then
    -- first node
    create()
    success = true
  else
    success = join(known_id)
  end

  -- main loop
  if join_success then

    local now = simgrid.get_clock()
    local next_stabilize_date = now + stabilize_delay
    local next_fix_fingers_date = now + fix_fingers_delay
    local next_check_predecessor_date = now + check_predecessor_delay
    local next_lookup_date = now + lookup_delay

    local task
    my_node.comm_recv = simgrid.task.irecv(my_node.id)

    while now < max_simulation_time do

      task = simgrid.comm.test(node.comm_recv)

      if task then
	-- I received a task: answer it
        my_node.comm_recv = simgrid.task.irecv(my_node.id)
	handle_task(task)
      else
        -- no task was received: do periodic calls
	if now >= next_stabilize_date then
          stabilize()
	  next_stabilize_date = simgrid.get_clock() + stabilize_delay

	elseif now >= next_fix_fingers_date then
	  fix_fingers()
	  next_fix_fingers_date = simgrid.get_clock() + fix_fingers_delay

	elseif now >= next_check_predecessor_date then
	  check_predecessor()
	  next_check_predecessor_date = simgrid.get_clock() + check_predecessor_delay
	elseif now >= next_lookup_date then
	  random_lookup()
	  next_lookup_date = simgrid.get_clock() + lookup_delay

	else
	  -- nothing to do: sleep for a while
	  simgrid.process.sleep(5)
	end
      end
      now = simgrid.get_clock()
    end -- while

    -- leave the ring
    leave()
  end
end

-- This function is called when the current node receives a task.
-- - task: the task received
function handle_task(task)

  local type = task.type

  if type == "find successor" then

    -- is my successor the successor?
    if is_in_interval(task.request_id, my_node.id + 1, my_node.fingers[1]) then
      task.type = "find successor answer"
      task.answer = my_node.fingers[1]
      task:dsend(task.answer_to)
    else
      -- forward the request to the closest preceding finger in my table
      task:dsend(closest_preceding_node(task.request_id))
    end

  elseif type == "get predecessor" then
    task.answer = my_node.predecessor
    task:dsend(task.answer_to)

  elseif type == "notify" then
    -- someone is telling me that he may be my new predecessor
    notify(task.request_id)

  elseif type == "predecessor leaving" then
    -- TODO

  elseif type == "successor_leaving" then
    -- TODO

  elseif type == "find successor answer" then
    -- ignoring

  elseif type == "get predecessor answer" then
    -- ignoring

  else
    error("Unknown type of task received: " .. task.type)
  end
end

-- Returns whether an id belongs to the interval [a, b[.
-- The parameters are normalized to make sure they are between 0 and nb_keys - 1.
-- 1 belongs to [62, 3].
-- 1 does not belong to [3, 62].
-- 63 belongs to [62, 3].
-- 63 does not belong to [3, 62].
-- 24 belongs to [21, 29].
-- 24 does not belong to [29, 21].
function is_in_interval(id, a, b)

  -- normalize the parameters
  id = id % nb_bits
  a = a % nb_bits
  b = b % nb_bits
 
  -- make sure a <= b and a <= id
  if b < a then
    b = b + nb_keys
  end

  if id < a then
    id = id + nb_keys
  end

  return id <= b
end

-- Attemps to join the Chord ring.
-- - known_id: id of a node already in the ring
-- - return value: true if the join was successful
function join(known_id)

  local successor = remote_find_successor(known_id, my_node.id)
  if successor == nil then
    return false
  end

  my_node.finger[1] = successor
  return true
end

-- Returns the closest preceding finger of an id with respect to the finger
-- table of the current node.
-- - id: the id to find
-- - return value: the closest preceding finger of that id
function closest_preceding_node(id)

  for i = nb_bits, 1, -1 do
    if is_in_interval(my_node.fingers[i].id, my_node.id + 1, id - 1) then
      -- finger i is the first one before id
      return my_node.fingers[i].id
    end
  end
end

-- Finds the successor of an id
-- id: the id to find
-- return value: the id of the successor, or -1 if the request failed
function find_successor(id)

  if is_in_interval(id, my_node.id + 1, my_node.fingers[1]) then
    -- my successor is the successor
    return my_node.fingers[1]
  end

  -- ask to the closest preceding finger in my table
  local ask_to = closest_preceding_node(id)
  return remote_find_successor(ask_to, id)
end

-- Asks a remote node the successor of an id.
-- ask_to: id of a remote node to ask to
-- id: the id to find
-- return value: the id of the successor, or nil if the request failed
function remote_find_successor(ask_to, id)

  local task = simgrid.task.new("", comp_size, comm_size)
  task.type = "find successor"
  task.request_id = id
  task.answer_to = my_node.id

  if task:send(ask_to, timeout) then
    -- request successfully sent: wait for an answer
    while true do
      task = simgrid.comm.wait(my_node.comm_recv, timeout)
      my_node.comm_recv = simgrid.task.irecv(my_node.id)
    
      if not task then
	-- failed to receive the answer
	return nil
      else
	-- a task was received: is it the expected answer?
	if task.type ~= "find successor answer" or task[request_id] ~= id then
          -- this is not our answer
	  handle_task(task)
	else
	  -- this is our answer
	  return task.answer
	end
      end
    end
  end

  return successor
end

-- Asks a remote node its predecessor.
-- ask_to: id of a remote node to ask to
-- return value: the id of its predecessor, or nil if the request failed
function remote_get_predecessor(ask_to)

  local task = simgrid.task.new("", comp_size, comm_size)
  task.type = "get predecessor"
  task.answer_to = my_node.id

  if task:send(ask_to, timeout) then
    -- request successfully sent: wait for an answer
    while true do
      task = simgrid.comm.wait(my_node.comm_recv, timeout)
      my_node.comm_recv = simgrid.task.irecv(my_node.id)
    
      if not task then
	-- failed to receive the answer
	return nil
      else
	-- a task was received: is it the expected answer?
	if task.type ~= "get predecessor answer" then
          -- this is not our answer
	  handle_task(task)
	else
	  -- this is our answer
	  -- FIXME make sure the message answers to this particular request
	  return task.answer
	end
      end
    end
  end

  return successor
end

-- Checks the immediate successor of the current node.
function stabilize()

  local candidate
  local successor = my_node.fingers[1]
  if successor ~= my_node.id then
    candidate = remote_get_predecessor(successor)
  else
    candidate = my_node.predecessor
  end

  -- this node is a candidate to become my new successor
  if candidate ~= nil and is_in_interval(candidate, my_node.id + 1, successor -1) then
    my_node.fingers[1] = candidate
  end

  if successor ~= my_node.id then
    remote_notify(successor, my_node.id)
  end
end

-- Notifies the current node that its predecessor my have changed
-- - candidate_predecessor: the possible new predecessor
function notify(candidate_predecessor)

  if my_node.predecessor == nil or is_in_interval(candidate_predecessor,
      my_node.predecessor + 1, my_node.id - 1) then
    my_node.predecessor = candidate_predecessor
  end
end

-- Notifies a remote node that its predecessor my have changed.
-- - notify_to
-- - candidate the possible new predecessor
function remote_notify(notify_to, candidate_predecessor)

  local task = simgrid.task.new("", comp_size, comm_size)
  task.type = "notify"
  task.request_id = candidate_predecessor
  task:dsend(notify_to)
end

-- Refreshes the finger table of the current node.
function fix_fingers()

  local i = my_node.next_finger_to_fix
  local id = find_successor(my_node.id + 2^i)
  if id ~= nil then
    if id ~= my_node.fingers[i] then
      fingers[i] = id
    end
    my_node.next_finger_to_fix = (i % nb_bits) + 1
  end
end

-- Checks whether the predecessor of the current node has failed.
function check_predecessor()
  -- TODO
end

-- Performs a find successor request to an arbitrary id.
function random_lookup()

  find_successor(1337)
end

simgrid.platform(arg[1] or "../../msg/msg_platform.xml")
simgrid.application(arg[2] or "../../msg/chord/chord90.xml")
simgrid.run()

