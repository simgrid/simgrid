-- Copyright (c) 2011, 2013-2014. The SimGrid Team.
-- All rights reserved.

-- This program is free software; you can redistribute it and/or modify it
-- under the terms of the license (GNU LGPL) which comes with this package.

require "simgrid"
dofile 'platform.lua'
dofile 'deploy.lua'
--Rutform.lua'
dofile 'master.lua'
dofile 'slave.lua'
   simgrid.run()
   simgrid.info("Simulation's over.See you.")
