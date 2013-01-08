dofile '../console/master.lua'
dofile '../console/slave.lua'
-- Simulation Code ----------------------------------------------------------

require "simgrid"

simgrid.platform(arg[1])
simgrid.application(arg[2])
simgrid.run()
simgrid.info("Simulation's over. See you.")
-- end-of-master-slave
