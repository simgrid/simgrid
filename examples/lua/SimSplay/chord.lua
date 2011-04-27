dofile "sim_splay.lua"
between, call, thread, ping = misc.between_c, rpc.call, events.thread, rpc.ping
n, predecessor, finger, timeout, m = {}, nil, {}, 5, 24
function join(n0) -- n0: some node in the ring
	simgrid.info("Euh...")
	predecessor = nil
	finger[1] = call(n0, {'find_successor', n.id})
	simgrid.info("8Here..")
	call(finger[1], {'notify', n})
end

function closest_preceding_node(id)
	for i = m, 1, -1 do
		if finger[i] and between(finger[i].id, n.id, id) then 
			return finger[i]
		end
	end
	return n
end

function find_successor(id)
	if finger[1].id == n.id or between(id, n.id, (finger[1].id + 1) % 2^m) then
		return finger[1]
	else
		local n0 = closest_preceding_node(id)
		return call(n0, {'find_successor', id})
	end
end
function stabilize()
	local x = call(finger[1], 'predecessor')
	if x and between(x.id, n.id, finger[1].id) then
		finger[1] = x -- new successor
		call(finger[1], {'notify', n})
	end
end
function notify(n0)
	if n0.id ~= n.id and
			(not predecessor or between(n0.id, predecessor.id, n.id)) then
		predecessor = n0
	end
end
function fix_fingers()
	refresh = (refresh and (refresh % m) + 1) or 1 -- 1 <= next <= m
	finger[refresh] = find_successor((n.id + 2^(refresh - 1)) % 2^m)
end
function check_predecessor()
	if predecessor and not rpc.ping(predecessor) then
		predecessor = nil
	end
end

n.id = math.random(1, 2^m)
finger[1] = n
if job then
	n.ip, n.port = job.me.ip, job.me.port
	join({ip = "192.42.43.42", port = 20000})
else
	simgrid.info("bizzaaaaaar...")
	n.ip, n.port = "127.0.0.1", 20000
	if arg[1] then n.ip = arg[1] end
	if arg[2] then n.port = tonumber(arg[2]) end
	if not arg[3] then
		print("RDV")
	else
		print("JOIN")
		thread(function() join({ip = arg[3], port = tonumber(arg[4])}) end)
	end
end
rpc.server(n.port)
events.periodic(stabilize, timeout)
events.periodic(check_predecessor, timeout)
events.periodic(fix_fingers, timeout)
events.loop()
