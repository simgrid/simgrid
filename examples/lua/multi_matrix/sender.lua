function Sender(...) 

  simgrid.info("Hello From Sender")
  local receiver = simgrid.Host.getByName(arg[1])
  local task_comp = arg[2]
  local task_comm = arg[3]
  local rec_alias = arg[4]

  local size = 4
  local m1 = mkmatrix(size, size)
  local m2 = mkmatrix(size, size)

  if #arg ~= 4 then
    error("Argc should be 4")
  end
  simgrid.info("Argc=" .. (#arg) .. " (should be 4)")

  -- Sending Task
  local task = simgrid.Task.new("matrix_task", task_comp, task_comm)
  task['matrix_1'] = m1
  task['matrix_2'] = m2
  task['size'] = size
  simgrid.info("Sending " .. simgrid.Task.name(task) .. " to " .. simgrid.Host.name(receiver))
  simgrid.Task.send(task, rec_alias)
  simgrid.info("Got the Multiplication result ...Bye")
end

