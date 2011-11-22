function Sender(...) 

  simgrid.info("Hello From Sender")
  local receiver = simgrid.host.get_by_name(arg[1])
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
  local task = simgrid.task.new("matrix_task", task_comp, task_comm)
  task['matrix_1'] = m1
  task['matrix_2'] = m2
  task['size'] = size
  simgrid.info("Sending " .. simgrid.task.get_name(task) .. " to " .. simgrid.host.name(receiver))
  simgrid.task.send(task, rec_alias)
  simgrid.info("Got the Multiplication result ...Bye")
end

