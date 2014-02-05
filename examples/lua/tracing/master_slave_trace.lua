-- Copyright (c) 2011, 2014. The SimGrid Team.
-- All rights reserved.

-- This program is free software; you can redistribute it and/or modify it
-- under the terms of the license (GNU LGPL) which comes with this package.

dofile 'master.lua'
dofile 'slave.lua'

require "simgrid"
-- Declaring the simulation tracing
simgrid.Trace.start();
-- Declaring tracing categories
simgrid.Trace.category("compute");
--:
simgrid.Trace.category("finalize");

if (#arg == 2) then
simgrid.platform(arg[1])
simgrid.application(arg[2])
else
simgrid.platform("../../msg/small_platform.xml")
simgrid.application("../deploy.xml")
end

simgrid.run()
simgrid.info("Simulation's over.See you.")
simgrid.Trace.finish()

