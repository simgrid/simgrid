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

