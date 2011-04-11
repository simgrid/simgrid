require "simgrid"
-- Splay global modules
rpc = {}
log = {}
job = {}
event = {}
os = {}
start = {}
-- Splay global variables
job.me ={}
job.nodes = {}
job.list_type = "random"

--Init nodes tables
function init_nodes()
	for i= 1,simgrid.Host.number() do		
		job.nodes[i] = simgrid.Host.getPropValue(simgrid.Host.at(i),"ip")..":"..simgrid.Host.getPropValue(simgrid.Host.at(i),"port");
	end 	
end

function init_jobs()
   init_nodes()
end

-- Job methods
function job.me.ip()
     return simgrid.Host.getPropValue(simgrid.Host.self(),"ip");
end

function job.me.port()
 return simgrid.Host.getPropValue(simgrid.Host.self(),"port");
end

function job.position()
  return simgrid.Host.getPropValue(simgrid.Host.self(),"position");
end

-- log Methods
function log:print(msg)
  simgrid.info(msg);
end

-- rpc Methods
function rpc.call(node,call)
 --init_nodes();
 func = "empty"
 arg = "empty"
 mailbox = node

 if type(call) == "table" then
 	func = call[1]
	arg = call[2]
 end
 task_call = simgrid.Task.new("splay_task",10000,10000);
 task_call['func_call_name'] = func;
 task_call['func_call_arg'] = arg;
 --log:print("Sending Task to mailbox "..mailbox.." to call "..func.." with arg "..arg);
 simgrid.Task.iSend(task_call,mailbox);
 call_function(func,arg)
end 

-- event Methods
function event.sleep(time)
  my_mailbox = job.me.ip()..":"..job.me.port()
  task = simgrid.Task.splay_recv(my_mailbox, time)
end

-- main func for each process, this is equivalent to the Deploiment file 
function event.thread(main_func)
  dofile("platform_script.lua");
 init_jobs()
end

-- OS methods
function os.exit()
 simgrid.Host.destroy(simgrid.Host.self());
end

-- Start Methods
function start.loop()
 simgrid.run()
 simgrid.clean()
end
-- useful functions
function call_function(fct,arg)
    _G[fct](arg)
end

function SPLAYschool()
 simgrid.info("Calling me...")
end
