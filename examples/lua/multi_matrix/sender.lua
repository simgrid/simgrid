-- Copyright (c) 2011, 2014. The SimGrid Team.
-- All rights reserved.

-- This program is free software; you can redistribute it and/or modify it
-- under the terms of the license (GNU LGPL) which comes with this package.

function Sender(...)

  simgrid.info("Hello From Sender")
  local receiver = simgrid.host.get_by_name(select(1, ...))
  local task_comp = select(2, ...)
  local task_comm = select(3, ...)
  local rec_alias = select(4, ...)

  local size = 4
  local m1 = mkmatrix(size, size)
  local m2 = mkmatrix(size, size)

  if select("#", ...) ~= 4 then
    error("Argc should be 4")
  end
  simgrid.info("Argc=" .. select("#", ...) .. " (should be 4)")

  -- Sending Task
  local task = simgrid.task.new("matrix_task", task_comp, task_comm)
  task['matrix_1'] = m1
  task['matrix_2'] = m2
  task['size'] = size
  simgrid.info("Sending " .. simgrid.task.get_name(task) .. " to " .. simgrid.host.name(receiver))
  simgrid.task.send(task, rec_alias)
  simgrid.info("Got the Multiplication result ...Bye")
end

