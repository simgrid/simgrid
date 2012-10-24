dofile 'master.lua'
dofile 'slave.lua'
-- Simulation Code ----------------------------------------------------------

require "simgrid"

simgrid.platform(arg[1] or "../../msg/small_platform.xml")
simgrid.application(arg[2] or "../deploy.xml")
simgrid.run()
simgrid.info("Simulation's over. See you.")
-- end-of-master-slave
