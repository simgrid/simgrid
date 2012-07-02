-- Routing table data
routing_table = {
buckets = {},
}
-- Initialize the routing table
function routing_table_init(id) 
	routing_table.id = id
	-- Bucket initialization
	for i = 0,common.IDENTIFIER_SIZE do
		routing_table.buckets[i] = {id = i, set = {}, list = {}}
	end
end
-- Returns an identifier which is in a specific bucket of a routing table
function get_id_in_prefix(id, prefix)
	if id == 0 then
		return 0
	end
	local identifier = 1
	identifier = math.pow(identifier,prefix - 1)
	identifier = bxor(identifier,id)
	return identifier
end
-- Returns the corresponding node prefix for a given id
function get_node_prefix(id)
	for i = 0,32 do
		if is_integer(id / math.pow(2,i)) and (id / math.pow(2,i)) % 2 == 1 then
			return 31 - i
		end
	end 
	return 0
end
-- Finds the corresponding bucket in a routing table for a given identifier
function find_bucket(id)
	local xor_number = bxor(id,routing_table.id)
	local prefix = get_node_prefix(xor_number)
	--simgrid.info("Prefix:" .. prefix .. " number:" .. xor_number)
	return routing_table.buckets[prefix]
end
-- Updates the routing table with a new value.
function routing_table_update(id)
	if id == routing_table.id then
		return
	end
	local bucket = find_bucket(id)
	if bucket.set[id] ~= nil then
		simgrid.debug("Updating " .. id .. " in my routing table")
		-- If the element is already in the bucket, we update it.
		table.remove(bucket.list,index_of(bucket.list,id))
		table.insert(bucket.list,0,id)
	else	
		simgrid.debug("Insert " .. id .. " in my routing table in bucket " .. bucket.id)
		table.insert(bucket.list,id)
		bucket.set[id] = true
	end
end
-- Returns the closest notes we know to a given id 
function find_closest(destination_id)
	local answer = {destination = destination_id, nodes = {}}
	
	local bucket = find_bucket(destination_id)
	for i,v in pairs(bucket.list) do
		table.insert(answer.nodes,{id = v, distance = bxor(v,destination_id)})
	end
	
	local i = 1
	
	while #answer.nodes < common.BUCKET_SIZE and ((bucket.id - i) >= 0 or (bucket.id + i) <= common.IDENTIFIER_SIZE) do
		-- Check the previous buckets
		if bucket.id - i >= 0 then
			local bucket_p = routing_table.buckets[bucket.id - i]
			for i,v in pairs(bucket_p.list) do
				table.insert(answer.nodes,{id = v, distance = bxor(v,destination_id)})
			end
		end			
		-- Check the next buckets
		if bucket.id + i <= common.IDENTIFIER_SIZE then
			local bucket_n = routing_table.buckets[bucket.id + i]
			for i,v in pairs(bucket_n.list) do
				table.insert(answer.nodes,{id = v, distance = bxor(v,destination_id)})
			end			
		end
		i = i + 1
	end
	-- Sort the list
	table.sort(answer.nodes, function(a,b) return a.distance < b.distance end)
	-- Trim the list
	while #answer.nodes > common.BUCKET_SIZE do
		table.remove(answer.nodes)
	end
	
	return answer
end