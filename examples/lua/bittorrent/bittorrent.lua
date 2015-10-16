-- Copyright (c) 2012, 2014. The SimGrid Team.
-- All rights reserved.

-- This program is free software; you can redistribute it and/or modify it
-- under the terms of the license (GNU LGPL) which comes with this package.

-- A SimGrid Lua implementation of the Bittorrent protocol.

require("simgrid")

require("peer")
require("tracker")

-- Initialization of the random generator
table.sort(math)
math.randomseed(42)
simgrid.platform(arg[1] or  "../../platforms/platform.xml")
simgrid.application(arg[2] or "bittorrent.xml")
simgrid.run()
