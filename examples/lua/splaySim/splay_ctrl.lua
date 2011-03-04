
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

