-- Copyright (c) 2011-2012, 2014. The SimGrid Team.
-- All rights reserved.

-- This program is free software; you can redistribute it and/or modify it
-- under the terms of the license (GNU LGPL) which comes with this package.

function Master(...)

  if select("#", ...) ~= 4 then
    error("Wrong number of arguments (got " .. #arg ..
        ", expected 4: nb_tasks comp_size comm_size slave_count)")
  end

  local nb_task, comp_size, comm_size, slave_count = select(1, ...)
  simgrid.info("Hello from lua, I'm the master")


  -- Dispatch the tasks

  for i = 1, nb_task do
    local task = simgrid.task.new("Task " .. i, comp_size, comm_size)
    local task_name = task:get_name()
    local alias = "slave " .. string.format("%d", i % slave_count)
    simgrid.info("Sending  '" .. task_name .. "' to '" .. alias .."'")
    task:send(alias) -- C user data set to NULL
    simgrid.info("Done sending '".. task_name .. "' to '" .. alias .."'")
  end

  -- Sending Finalize Message To Others

  simgrid.info("All tasks have been dispatched. Let's tell everybody the computation is over.")
  for i = 0, slave_count - 1 do
    local alias = "slave " .. i
    simgrid.info("Sending finalize to '" .. alias .. "'")
    local finalize = simgrid.task.new("finalize", comp_size, comm_size)
    finalize:send(alias)
  end
  simgrid.info("Everything's done.")
end -- end_of_master

