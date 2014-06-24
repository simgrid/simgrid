-- Copyright (c) 2011, 2014. The SimGrid Team.
-- All rights reserved.

-- This program is free software; you can redistribute it and/or modify it
-- under the terms of the license (GNU LGPL) which comes with this package.

dofile 'sender.lua'
dofile 'receiver.lua'
require "simgrid"

simgrid.platform("../../platforms/platform.xml")
simgrid.application("quicksort_deployment.xml")
simgrid.run()
simgrid.info("Simulation's over.See you.")

