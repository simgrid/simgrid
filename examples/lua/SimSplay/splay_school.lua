require("sim_splay")

function SPLAYschool()
  log:print("My ip is: "..job.me.ip())
  for i = 1000,10000 do
    log:print(i)
  end
  
--[[
  events.sleep(5)

  if job.me.ip() == job.nodes[1].ip then
    log:print("calling")
    rpc.call(job.nodes[2], {"call_me", job.me.ip()})
  end
  log:print("second sleep")
  events.sleep(5)
  os.exit()
]]
end

function call_me(from)
  log:print("I received an RPC from "..from)
end

events.thread("SPLAYschool")
start.loop()

