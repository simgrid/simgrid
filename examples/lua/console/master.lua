--Master Function
function Master(...) 

  if #arg ~= 4 then
    error("Wrong number of arguments (got " .. #arg ..
          ", expected 4: nb_tasks comp_size comm_size slave_count)")
  end

  simgrid.info("Hello from lua, I'm the master")
  for i,v in ipairs(arg) do
      simgrid.info("Got " .. v)
  end

  local nb_task, comp_size, comm_size, slave_count = unpack(arg)

  simgrid.info("Argc=" .. (#arg) .. " (should be 4)")

  -- Dispatch the tasks

  for i = 1, nb_task do
    task = simgrid.task.new("Task " .. i, comp_size, comm_size);
    local task_name = simgrid.task.get_name(task)
    alias = "slave " .. (i%slave_count);
    simgrid.info("Master sending  '" .. task_name .. "' To '" .. alias .. "'");
    simgrid.task.send(task, alias); -- C user data set to NULL
    simgrid.info("Master done sending '" .. task_name .. "' To '" .. alias .. "'");
  end

  -- Sending Finalize Message To Others

  simgrid.info("Master: All tasks have been dispatched. Let's tell everybody the computation is over.");
  for i = 0, slave_count-1 do
    alias = "slave " .. i;
    simgrid.info("Master: sending finalize to " .. alias);
    finalize = simgrid.task.new("finalize", comp_size, comm_size);
    simgrid.task.send(finalize, alias)
  end
  simgrid.info("Master: Everything's done.");
end
--end_of_master
