require "simgrid"

-- Splay global modules
rpc = {}
log = {}
job = {}
events = {}
os = {}
start = {}
misc = {}

-- Splay global variables
job.me = {}
job.nodes = {}
job.list_type = "random"

-- Init nodes tables
function init_nodes()
  for i = 1, simgrid.host.number() do		
    job.nodes[i] = { ip = simgrid.host.get_prop_value(simgrid.host.at(i), "ip"),
                     port = simgrid.host.get_prop_value(simgrid.host.at(i), "port") }
  end 	
end

function init_jobs()
  init_nodes()
end

-- Job methods
function job.me.ip()
  return simgrid.host.get_prop_value(simgrid.host.self(), "ip")
end

function job.me.port()
  return simgrid.host.get_prop_value(simgrid.host.self(), "port")
end


function job.position()
  return simgrid.host.get_prop_value(simgrid.host.self(), "position")
end

-- log Methods
function log:print(msg)
  simgrid.info(msg);
end

-- rpc Methods
function rpc.call(node, call)
  --init_nodes();
  func = "empty"
  arg = "empty"
  mailbox = node

  if type(node) == "table" then
    mailbox = node.ip..":"..node.port
  end

  if type(call) == "table" then
    func = call[1]
    arg = call[2]
  end
  task_call = simgrid.task.new("splay_task", 10000, 10000)
  task_call['func_call_name'] = func
  task_call['func_call_arg'] = arg
  log:print("Sending Task to mailbox "..mailbox.." to call '"..func.."' with arg '"..arg.."'")
  simgrid.task.send(task_call, mailbox)

end

function rpc.server(port)
  -- nothing really to do : no need to open Socket since it's a Simulation
end

-- event Methods
function events.sleep(time)
  my_mailbox = job.me.ip()..":"..job.me.port()
  task = simgrid.task.recv(my_mailbox, time)

  if task ~= nil then
    -- an RPC call just woke me up
    call_function(task['func_call_name'], task['func_call_arg'])
  end
end

-- main function for each process, this is equivalent to the deployment file 
function events.thread(main_func)
  dofile("platform_script.lua")
  init_jobs()
end

-- OS methods
function os.exit()
  simgrid.host.destroy(simgrid.host.self())
end

-- Start Methods
function start.loop()
  simgrid.run()
  --simgrid.clean()
end

-- Misc Methods
function misc.between(a, b)
  return a
end

-- useful functions
function call_function(fct, arg)
  _G[fct](arg)
end

function SPLAYschool(arg)
  simgrid.info("Calling me..."..arg)
end

