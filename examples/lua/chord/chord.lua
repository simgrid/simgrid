-- A SimGrid Lua implementation of the Chord DHT

-- Copyright (c) 2011-2012, 2014. The SimGrid Team.
-- All rights reserved.

-- This program is free software; you can redistribute it and/or modify it
-- under the terms of the license (GNU LGPL) which comes with this package.

require("simgrid")

nb_bits                 = 24
nb_keys                 = 2^nb_bits
comp_size               = 0
comm_size               = 10
timeout                 = 50
max_simulation_time     = 1000
stabilize_delay         = 20
fix_fingers_delay       = 120
check_predecessor_delay = 120
lookup_delay            = 10

-- current node (don't worry, globals are duplicated in each simulated process)
my_node = {
  -- FIXME: my_id does not exist.
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
function node(...)

  simgrid.info("Hi! This is my first message; I just entered the program!")
  -- TODO simplify the deployment file
  local known_id
  local args = {...}
  my_node.id = math.tointeger(args[1])
  simgrid.info("My id is now " .. my_node.id)
  if #args == 4 then
    known_id = math.tointeger(args[2])
    simgrid.info("Updated my known_id to " .. known_id)
  end

  -- initialize the node and the fingertable;
  -- at the beginning, this node only knows itself (we need to discover others)
  for i = 1, nb_bits, 1 do
    my_node.fingers[i] = my_node.id
  end

  -- Let's make sure we can receive messages!
  my_node.comm_recv = simgrid.task.irecv(my_node.id)

  -- join the ring
  local join_success = false
  --simgrid.info(known_id)

  if known_id == nil then
    -- only the first node ("Jacqueline") will enter here
    -- as configured in file ../../msg/chord/chord.xml
    simgrid.info("First node here. Going to create everything.")
    create()
    join_success = true
  else
    -- Communicate to the first node and join the ring there
    -- This will also initialize
    -- my_node.predecessor and my_node.successor
    join_success = join(known_id)
  end

  -- At this point, finger[1] does not necessarily actually point
  -- to the *real* successor; it might be that the first node still
  -- didn't notify us that another node joined with
  -- an ID that satisfies my_id <= ID <= current_successorId

  -- TODO Remove this, but make sure my_node.predecessor is initialized somewhere
  --if my_node.id == 1 then
      --simgrid.info("YUHU!")
      --my_node.predecessor = 1
  --end

  -- main loop
  if join_success then

    local now                         = simgrid.get_clock()
    local next_stabilize_date         = now + stabilize_delay
    local next_fix_fingers_date       = now + fix_fingers_delay
    local next_check_predecessor_date = now + check_predecessor_delay
    local next_lookup_date            = now + lookup_delay

    local task, err

    simgrid.debug("I'm now entering the main loop.")

    while now < max_simulation_time do

      task, err = my_node.comm_recv:test()
      simgrid.info(now .. " " .. next_stabilize_date .. " " .. now + stabilize_delay .. " " .. next_fix_fingers_date .. " " .. next_check_predecessor_date .. " " .. next_lookup_date)

      if task then
        -- I received a task: answer it
        simgrid.info("I received a task of type '" .. task.type .."'! My id is " .. my_node.id)
        my_node.comm_recv = simgrid.task.irecv(my_node.id)
        handle_task(task)
      elseif err then
        -- the communication has failed: nevermind, we'll try again
        simgrid.info("Error while receiving a task! My id is " .. my_node.id)
        my_node.comm_recv = simgrid.task.irecv(my_node.id)
      else
        -- no task was received: do periodic calls

        if now >= next_stabilize_date then
          simgrid.debug("Stabilizing...")
          stabilize()
          simgrid.debug("Finished stabilizing!")
          next_stabilize_date = simgrid.get_clock() + stabilize_delay

        --elseif now >= next_fix_fingers_date then
          --fix_fingers()
          --next_fix_fingers_date = simgrid.get_clock() + fix_fingers_delay

        --elseif now >= next_check_predecessor_date then
          --check_predecessor()
          --next_check_predecessor_date = simgrid.get_clock() + check_predecessor_delay

        --elseif now >= next_lookup_date then
          --random_lookup()
          --simgrid.debug("I'm now executing a lookup, as lookup_delay makes me do this. " .. simgrid.get_clock())
          --next_lookup_date = simgrid.get_clock() + lookup_delay

        else
          ---- nothing to do: sleep for a while
          --simgrid.debug("Didn't have to stabilize, update my fingers, check my predecessors or do a random lookup; hence, I'm starting to sleep now...")
          simgrid.process.sleep(5)
          --simgrid.debug("Slept for 5s")
        end
      end
      now = simgrid.get_clock()
    end -- while

    -- leave the ring
    leave()
  end
end

-- Makes the current node leave the ring
function leave()

  simgrid.info("Leaving the ring, because max_simulation_time was reached.")
  -- TODO: notify others
end

-- This function is called when the current node receives a task
-- and can not immediately deal with it; for instance, if the host
-- waits on a response for a 'find successor' query but receives a
-- 'get predecessor' message instead; we cannot just discard this
-- message so we deal with it here.
--
-- - task: the task received
function handle_task(task)

  simgrid.debug("Handling task in handle_task()")
  local type = task.type

  if type == "find successor" then

    task.answer_to = math.tointeger(task.answer_to)
    task.request_id = math.tointeger(task.request_id)

    simgrid.info("Received a 'find successor' request from " .. string.format("%d", task.answer_to) ..
        " for id " .. string.format("%d", task.request_id))

    -- Is my successor have the right host? This can happen if there are holes
    -- in the ring; for instance, if my id is 13 and my successor is 17 and
    -- 14,15,16 don't exist but I'm asked for 15, then yes, 17 is the right
    -- answer to the request.
    --
    -- Test: my_node.id + 1 <= task.request_id <= my_node.fingers[1]
    --                  ^^^
    -- TODO: Why the +1? We could receive this message from a host that forwarded
    --       this message (and the original sender doesn't know us),
    --       so why do we exclude ourselves?
    if is_in_interval(task.request_id, my_node.id + 1, my_node.fingers[1]) then

      simgrid.info("Sending back a 'find successor answer' to " ..
          string.format("%d", task.answer_to) .. ": the successor of " .. string.format("%d", task.request_id) ..
	  " is " .. string.format("%d", my_node.fingers[1]))

      task.type = "find successor answer"
      -- TODO: Can we remove the "" here?
      task.answer = math.tointeger(my_node.fingers[1])
      simgrid.info("Answer" .. task.answer)
      task:dsend(math.tointeger(task.answer_to))
    else
      -- forward the request to the closest preceding finger in my table

      simgrid.info("Forwarding the 'find successor' request to my closest preceding finger")
      task:dsend(closest_preceding_node(task.request_id))
    end

  elseif type == "get predecessor" then
    simgrid.info("Received a 'find predecessor' request from " .. string.format("%d", task.answer_to) ..
        " for id. Sending back an answer.")

    task.type = "get predecessor answer"

    --for i,v in pairs(my_node) do
        --print(my_node.id, i, v)
    --end
    --print(my_node.predecessor)
    if my_node.predecessor ~= nil then
      task.answer = math.tointeger(my_node.predecessor)
      --print(my_node.predecessor)
    else
      -- FIXME: This is completely wrong here. Fix this;
      --        we need to figure out what to send if we don't know our
      --        predecessor yet
      simgrid.critical("Don't know my predecessor yet!")
      my_node.predecessor = remote_get_predecessor(my_node.fingers[1])
      task.answer = my_node.predecessor
    end

    --print("It will break now, since task.answer is nil here.")
    --task.answer = my_node.predecessor
    --print(task.answer)
    --print("Before")
    task:dsend(math.tointeger(task.answer_to))
    --print("After dsend returned")

  elseif type == "notify" then
    -- someone is telling me that he may be my new predecessor
    simgrid.info("Host id " .. task.request_id .. " wants to be my predecessor")
    notify(math.tointeger(task.request_id))

  elseif type == "predecessor leaving" then
    -- TODO
    simgrid.debug("predecessor leaving")

  elseif type == "successor_leaving" then
    -- TODO: We could / should use something like table.remove(my_node.fingers, 1) here
    simgrid.debug(type)

  elseif type == "find successor answer" then
    -- ignoring, this is handled in remote_find_successor
    simgrid.debug(type)

  elseif type == "get predecessor answer" then
    -- ignoring, this is already handled in

  else
    error("Unknown type of task received: " .. task.type)
  end

  simgrid.info("I'm leaving handle_task() now.")
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
  id= math.tointeger(id)
  a = math.tointeger(a)
  b = math.tointeger(b)

  -- normalize the parameters
  -- TODO: Currently, nb_bits = 24; so a,b,id < 24! Really?
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

-- Returns whether the current node is in the ring.
function has_joined()

  return my_node.fingers[my_node.id] ~= nil
end

-- Creates a new Chord ring.
function create()
  simgrid.debug("I've now initialized my predecessor and fingertable.")
  --my_node.predecessor = my_node.id
  my_node.predecessor = nil
  my_node.fingers[1]  = my_node.id
end

-- Attemps to join the Chord ring.
-- - known_id: id of a node already in the ring
-- - return value: true if the join was successful
function join(known_id)

  simgrid.info("Joining the ring with id " .. my_node.id .. ", knowing node " .. known_id)

  local successor = remote_find_successor(known_id, my_node.id)
  simgrid.info("Returned from remote_find_successor; my successor is " .. successor)
  if successor == nil then
    simgrid.critical("Cannot join the ring.")
    return false
  end

  -- We don't know the predecessor yet, so we initialize it with NULL
  my_node.predecessor = nil
  my_node.fingers[1] = successor

  -- Everything was successfull!
  return true
end

-- Returns the closest preceding finger of an id with respect to the finger
-- table of the current node.
-- - id: the id to find
-- - return value: the closest preceding finger of that id
function closest_preceding_node(id)

  for i = nb_bits, 1, -1 do
    if is_in_interval(my_node.fingers[i], my_node.id + 1, id - 1) then
      -- finger i is the first one before id
      simgrid.info("fix_fingers: The closest preceding node for " .. id .. " is finger " .. i .. " (node " .. my_node.fingers[i] .. ")")
      return my_node.fingers[i]
    end
  end
end

-- Finds the successor of an id
-- id: the id to find
-- return value: the id of the successor, or nil if the request failed
function find_successor(id)

  if is_in_interval(id, my_node.id + 1, my_node.fingers[1]) then
    -- my successor is the successor
    simgrid.info("Looking for successor of " .. id .. ", but I determined it's my own successor: " .. my_node.fingers[1])
    return my_node.fingers[1]
  else
    -- ask to the closest preceding finger in my table
    simgrid.info("fix_fingers: Looking for successor of " .. id .. ", checking closest preceding node")
    local ask_to = closest_preceding_node(id)
    simgrid.info("fix_fingers: Looking for successor of " .. id .. ", checking closest preceding node")
    return remote_find_successor(ask_to, id)
  end

end

-- Asks a remote node the successor of an id.
-- ask_to: id of a remote node to ask to
-- id: the id to find
-- return value: the id of the successor, or nil if the request failed
function remote_find_successor(ask_to, id)

  local task      = simgrid.task.new("", comp_size, comm_size)
  task.type       = "find successor"
  task.request_id = id               -- This is the id we want to find
  task.answer_to  = my_node.id       -- This is where the answer needs to go
                                     -- (back to us)

  simgrid.info("Sending a 'find successor' request to " .. ask_to .. " for id " .. id .. " (timeout=".. timeout .. ")")
  if task:send(ask_to, timeout) then
    -- request successfully sent: wait for an answer

    while true do
      simgrid.info("New iteration in while loop of remote_find_successor(); I'm still waiting for a response!")
      --print(task.request_id)
      simgrid.info("Starting to wait for a message; timeout=" .. timeout)
      task = my_node.comm_recv:wait(timeout)
      simgrid.info("Finished to wait")
      -- TODO Do we need this?
      --for i,v in pairs(task) do
          --print(i, v)
      --end
      --simgrid.info("I will crash!")
      --task.answer = math.tointeger(task.answer)
      --simgrid.info("Ich denke task.type ist leer")
      --simgrid.info("Before irecv: " .. my_node.id)

      -- Even if the recv above failed (timeout occurred) -- we want to be
      -- able to receive a message if it comes in, even without us explicitly
      -- calling the recv() method.
      my_node.comm_recv = simgrid.task.irecv(my_node.id)

      if not task then
        -- failed to receive the answer
        return nil
      else
        -- a task was received: is it the expected answer (i.e., the response to
        -- our query and for the id we're interested in)
        if task.type ~= "find successor answer" or task.request_id ~= id then
          -- this is not our answer, but we still need to handle it.
          simgrid.info("Wrong request of type " .. task.type .. " received, expected 'find successor answer'")
          handle_task(task)

        else
          -- this is our answer
          simgrid.info("Received the answer to my 'find successor' request for id " ..
          id .. ": the successor is " .. task.answer)

          -- TODO: Do we need math.tointeger here?
          return math.tointeger(task.answer)
        end
      end
    end
  else
    simgrid.info("Failed to send the 'find successor' request to " .. ask_to ..
      " for id " .. id)
  end

  -- This can never be reached, because if this host finds the successor, it
  -- will return it right away!
  simgrid.info("Whooops! I should never reach this statement, because I didn't find a successor!")

  -- We need to return the successor here
end

-- Asks a remote node its predecessor.
-- ask_to: id of a remote node to ask to
-- return value: the id of its predecessor, or nil if the request failed
function remote_get_predecessor(ask_to)

  local task = simgrid.task.new("", comp_size, comm_size)
  task.type = "get predecessor"
  task.answer_to = math.tointeger(my_node.id)
  -- TODO c.heinrich: Remove this
  --task.note = "Bla " .. ask_to .. " at time  " .. simgrid.get_clock()

  simgrid.info("Sending request for '" .. task.type .."' to id '" .. ask_to .. "'")
  if task:send(ask_to, timeout) then
    simgrid.info("Done sending the request to " .. ask_to)
    -- request successfully sent: wait for an answer
    -- We need to iterate here because we might receive other
    -- messages too (but not the answer to the request we just sent);
    -- hence, we loop here.
    while true do
      simgrid.info("Starting to wait. My id: " .. my_node.id)
      task = my_node.comm_recv:wait(timeout)
      simgrid.info("Finished to wait. My id: " .. my_node.id .. " ask_to is " .. ask_to)
      my_node.comm_recv = simgrid.task.irecv(my_node.id)

      if not task then
        -- failed to receive the answer
        simgrid.info("Task not received - is null?")
        return nil
      else
        -- a task was received: is it the expected answer?
        if task.type ~= "get predecessor answer" then
          -- this is not our answer
          simgrid.info("Task is NOT 'get predecessor answer'")
          handle_task(task)
        else
          -- this is our answer
          -- FIXME make sure the message answers to this particular request
          --simgrid.info(task.answer)
          for i,v in pairs(task) do
              print(my_node.id, i, v)
          end
          simgrid.info("Task is answer for predecessor! The answer is: ")
          if (task.answer) then print("NIL!\n") else print("Not NIL\n") end
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
    simgrid.info("Getting remote predecessor from ".. successor)
    candidate = remote_get_predecessor(successor)
    simgrid.info("Received ".. candidate .. " as candidate")
  else
    candidate = my_node.predecessor
  end

  simgrid.info("Still stabilizing")
  -- candidate might become my new successor
  if candidate ~= nil and is_in_interval(candidate, my_node.id + 1, successor - 1) then
    simgrid.info("I'm updating my successor to " .. math.tointeger(candidate))
    my_node.fingers[1] = math.tointeger(candidate)

    -- If candidate is not my_node.id, then I should notify candidate that I'm here.
    -- (So this node updates it's predecessor to me)
    --remote_notify(candidate, my_node.id)
  end

  simgrid.info("Successor: " .. successor .. " and my id: " .. my_node.id)
  -- If candidate is nil, this means that our successor has no predecessor.
  -- So we should tell him about us...
  -- TODO: I think a host that receives a message could automatically add
  -- this other node as a predecessor if it doesn't have any... needs to
  -- be implemented somewhere else, not here.
  if candidate == nil and successor ~= my_node.id then
    remote_notify(successor, my_node.id)
  end
end

-- Notifies the current node that its predecessor may have changed
-- - candidate_predecessor: the possible new predecessor
function notify(candidate_predecessor)
  if my_node.predecessor == nil or is_in_interval(candidate_predecessor,
      my_node.predecessor + 1, my_node.id - 1) then
    simgrid.info("Updated my predecessor to " .. candidate_predecessor)
    my_node.predecessor = math.tointeger(candidate_predecessor)
  end
end

-- Notifies a remote node that its predecessor my have changed.
-- - notify_to
-- - candidate the possible new predecessor
function remote_notify(notify_to, candidate_predecessor)

  simgrid.info("Updating someone else's predecessor (id: " .. notify_to .. " predecessor to ".. candidate_predecessor .. ")")
  local task = simgrid.task.new("", comp_size, comm_size)
  task.type = "notify"
  task.request_id = candidate_predecessor
  task:dsend(notify_to)
end

-- Refreshes the finger table of the current node,
-- one finger per call.
function fix_fingers()

  local i = math.tointeger(my_node.next_finger_to_fix)
  local id = find_successor(math.tointeger(my_node.id + 2^i))
  simgrid.info("Called fix_fingers(). Next finger to fix: " .. i .. " and I will check " .. my_node.id + 2^i .. ". Request returned " .. id)

  if id ~= nil then
    if id ~= my_node.fingers[i] then
      my_node.fingers[i] = id
      simgrid.info("fix_fingers: Updated finger " .. i .. " to " .. id)
    else
      simgrid.info("fix_fingers: id is " .. id)
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

