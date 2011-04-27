dofile 'sim_splay.lua'

function SPLAYschool()
    log:print("My ip is :" ..job.me.ip())
    events.sleep(5)
    rpc.call(job.nodes[3],{"call_me","Arg_test"})
    events.sleep(5)
    os.exit()
end

function call_me(position)
    log:print("I received an RPC from node "..position);
end

events.thread("SPLAYschool")
start.loop()



