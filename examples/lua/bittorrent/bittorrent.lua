-- A SimGrid Lua implementation of the Bittorrent protocol.

require("simgrid")
	
require("peer")
require("tracker")

simgrid.platform(arg[1] or  "../../msg/msg_platform.xml")
simgrid.application(arg[2] or "bittorrent.xml")
simgrid.run()