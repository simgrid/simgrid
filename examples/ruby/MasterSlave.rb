require 'simgrid'
include MSG
#################################################
# Class Master
#################################################


class Master < MSG::Process  
  # main : that function that will be executed when running simulation

  def main(args) # args is an array containing arguments for function master
   size = args.size
   for i in 0..size-1
     MSG::info("args["+String(i)+"]="+args[i])
   end
  
   raise "Master needs 3 arguments" if size < 3 
   numberOfTask = Integer(args[0]) 
   taskComputeSize = Float(args[1])
   taskCommunicationSize = Float(args[2])
   slaveCount = Integer(args[3]) 
   
   # Creates and sends the tasks
    for i in 0..numberOfTask-1
     task = Task.new("Task_"+ i.to_s, taskComputeSize , taskCommunicationSize);
     mailbox = "slave " + (i%slaveCount).to_s
     MSG::info("Master Sending "+ task.name + " to " + mailbox + " with Comput Size " + 
           task.compSize.to_s)
     task.send(mailbox)
     MSG::info("Master Done Sending " + task.name + " to " + mailbox)
    end
  
   # Sending Finalize MSG::Tasks
   #MSG::info("Master: All "+numberOfTask+" tasks have been dispatched. Let's tell everybody the computation is over.")
   MSG::info("Master: All tasks have been dispatched. Let's tell everybody the computation is over.")
   for i in 0..slaveCount-1
     mailbox = "slave " + i.to_s
     finalize_task = Task.new("finalize",0,0)
     finalize_task.send(mailbox)
   end
   MSG::info("Master : Everything's Done")
  end    
end
#################################################
# Class Slave
#################################################
class Slave < MSG::Process

  def main(args)
    mailbox = "slave " + args[0]
    for i in 0..args.size-1
      MSG::debug("args["+String(i)+"]="+args[i])
    end

    while true
       task = Task.receive(mailbox)
       if (task.name == "finalize")
	       break
       end
       task.execute
       MSG::debug("Slave '" + mailbox + "' done executing task "+ task.name + ".")
    end
    MSG::info("I'm done, see you")
  end
end
#################################################
# main chunck
#################################################
if (ARGV.length == 2) 
	MSG.createEnvironment(ARGV[0])
	MSG.deployApplication(ARGV[1])
else 
	MSG.createEnvironment("platform.xml")
	MSG.deployApplication("deploy.xml")
end
MSG.run
puts "Simulation time : " + MSG.getClock .to_s
MSG.exit
