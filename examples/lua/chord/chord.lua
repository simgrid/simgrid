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

-- current node (globals are duplicated in each process)
my_node = {
  id = my_id,
  next_finger_to_fix = 1,
  fingers = {},
  predecessor = nil
  comm_recv = nil
}

-- Main function of each Chord process
-- Arguments:
-- - my id
-- - the id of a guy I know in the system (except for the first node)
function node(my_id, known_id)

  simgrid.info("Hello, I'm a node")
  --
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

    while simgrid.get_clock() < max_simulation_time do

      task = simgrid.comm.test(node.comm_recv)

      if task then
	-- I received a task: answer it
        my_node.comm_recv = simgrid.task.irecv(my_node.id)
	handle_task(task)
      else
        -- no task was received: do periodic calls
	if simgrid.get_clock() >= next_fix_fingers_date then
	
	end
      end
    end
    leave()
  end
end

-- Returns whether an id belongs to the interval [a, b[.
-- The parameters are noramlized to make sure they are between 0 and nb_keys - 1).
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

-- Finds the successor of an id
-- id: the id to find
-- return value: the id of the successor, or -1 if the request failed
function find_successor(d)

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
-- return value: the id of the successor, or -1 if the request failed
function remote_find_successor(ask_to, id)

  local successor
  local task = simgrid.task.new(comp_size, comm_size)
  task[type] = "find successor"
  task[request_id] = id
  task[issuer] = my_node.id

  if task:send(ask_to, timeout) then
    -- request successfully sent: wait for an answer
    local stop = false
    repeat
      task = simgrid.comm.wait(my_node.comm_recv, timeout)
      my_node.comm_recv = simgrid.task.irecv(my_node.id)
    
      if not task then
	-- failed to receiver the answer
        stop = true
      else
	-- a task was received: is it the expected answer?
	if task[type] ~= "find successor" or task[request_id] ~= id then
          -- this is not our anwser
	  handle_task(task)
	else
	  -- this is our answer
	  successor = task[answer_id]
	  stop = true
	end
      end
    until stop
  end

  return successor
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

simgrid.platform(arg[1] or "../../msg/msg_platform.xml")
simgrid.application(arg[2] or "../../msg/chord/chord90.xml")
simgrid.run()

