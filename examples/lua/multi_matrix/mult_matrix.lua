dofile 'sender.lua'
dofile 'receiver.lua'
require "simgrid"

simgrid.platform("quicksort_platform.xml")
simgrid.application("quicksort_deployment.xml")
simgrid.run()
simgrid.info("Simulation's over.See you.")

