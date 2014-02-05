-- Copyright (c) 2011-2014. The SimGrid Team.
-- All rights reserved.

-- This program is free software; you can redistribute it and/or modify it
-- under the terms of the license (GNU LGPL) which comes with this package.

dofile 'master.lua'
dofile 'slave.lua'
-- Simulation Code ----------------------------------------------------------

require "simgrid"

simgrid.platform(arg[1])
simgrid.application(arg[2])
simgrid.run()
simgrid.info("Simulation's over. See you.")
-- end-of-master-slave
