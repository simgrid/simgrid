-- Copyright (c) 2011, 2014. The SimGrid Team.
-- All rights reserved.

-- This program is free software; you can redistribute it and/or modify it
-- under the terms of the license (GNU LGPL) which comes with this package.

-- Simulation Code ----------------------------------------------------------

require "simgrid"
if (#arg == 2) then
	dofile (arg[1])
	dofile (arg[2])
else
	dofile "splay_platform.lua"
	dofile "splay_deploy_masterslave.lua"
end

simgrid.run()
simgrid.info("Simulation's over.See you.")
simgrid.clean()

