function Sender(...) 

	simgrid.info("Hello From Sender")
	receiver = simgrid.Host.getByName(arg[1])
	task_comp = arg[2]
	task_comm = arg[3]
	rec_alias = arg[4]
	
	size = 4
	m1 = mkmatrix(size, size)
        m2 = mkmatrix(size, size)	

	if (#arg ~= 4) then
	    error("Argc should be 4");
	end
	simgrid.info("Argc="..(#arg).." (should be 4)")

	-- Sending Task
	task = simgrid.Task.new("matrix_task",task_comp,task_comm);
	task['matrix_1'] = m1;
	task['matrix_2'] = m2;
	task['size'] = size;
	simgrid.info("Sending "..simgrid.Task.name(task).." to "..simgrid.Host.name(receiver));
	simgrid.Task.send(task,rec_alias);
	-- Read The Result 
	mm = task['matrix_res']
	simgrid.info("Got the Multiplication result ...Bye");
	--mprint(size,size,mm);

end
