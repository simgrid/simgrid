require "simgrid"
dofile 'platform.lua'
--dofile 'deploy.lua'
--Rutform.lua'
dofile 'master.lua'
dofile 'slave.lua'
   --dofile 'deploy.lua'
   simgrid.application("deployb.xml")
   simgrid.run()
   simgrid.info("Simulation's over.See you.")
