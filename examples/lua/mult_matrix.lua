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
------------------------------------------------------
function Receiver(...)
	
	simgrid.info("Hello From Receiver")
	sender = simgrid.Host.getByName(arg[1])
	send_alias = arg[2]
	recv_alias = "Receiver";
	simgrid.info("Receiving Task from "..simgrid.Host.name(sender));
	task = simgrid.Task.recv(recv_alias);
	mm = mmult(task['size'],task['size'],task['matrix_1'],task['matrix_2']);
	--mprint(task['size'],task['size'],mm)
	task['matrix_res'] = mm;
	simgrid.info("Calcul is done ... Bye");


end
-----------------------------------------------------

local n = tonumber((arg and arg[1]) or 1)



function mkmatrix(rows, cols)
    local count = 1
    local mx = {}
    for i=0,(rows - 1) do
    local row = {}
    for j=0,(cols - 1) do
        row[j] = count
        count = count + 1
    end
    mx[i] = row
    end
    return(mx)
end

function mmult(rows, cols, m1, m2)
    local m3 = {}
    for i=0,(rows-1) do
        m3[i] = {}
        for j=0,(cols-1) do
            local rowj = 0
            for k=0,(cols-1) do
                rowj = rowj + m1[i][k] * m2[k][j]
            end
            m3[i][j] = rowj
        end
    end
    return(m3)
end

function mprint(rows,cols,m)
 for i=0,(cols-1)do
	for j=0,(rows-1)do
	 	print (m[i][j])
	end
 end
end


--end
require "simgrid"
simgrid.platform("../ruby/quicksort_platform.xml")
simgrid.application("../ruby/quicksort_deployment.xml")
simgrid.run()
simgrid.info("Simulation's over.See you.")
simgrid.clean()


