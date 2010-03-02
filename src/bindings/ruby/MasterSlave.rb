# Debug it with this command:
# make -C ../.. && valgrind ruby MasterSlave.rb --log=ruby.thres:debug 2>&1 | less

require 'simgrid'
include MSG

#################################################
# Class Master
#################################################

class Master < MsgProcess  
  # main : that function that will be executed when Running Simulation
  def main(args) # args is an array containing arguments for function master
   size = args.size
   for i in 0..size-1
     info("args["+String(i)+"]="+args[i])
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
     info("Master Sending "+ Task.name(task) + " to " + mailbox + " with Comput Size " + 
          Task.compSize(task).to_s)
#          task.compSize.to_s) # FIXME: This version creates a deadlock. Interesting
     Task.send(task,mailbox)
     info("Master Done Sending " +Task.name(task) + " to " + mailbox)
   end
  
   # Sending Finalize Tasks
   info ("Master: All tasks have been dispatched. Let's tell everybody the computation is over.")
   for i in 0..slaveCount-1
     mailbox = "slave " + i.to_s
     info ("Master Sending Finalize to " + mailbox)
     Task.send(Task.new("finalize",0,0),mailbox)
   end
   info("Master : Everything's Done")
  end    
end

#################################################
# Class Slave
#################################################
class Slave < MsgProcess
  def main(args)
    mailbox = "slave " + args[0]

    while true
       info("Ready to Receive Task")
       task = Task.receive(mailbox)
       task_name = Task.name(task)
       info ("Task Received : " + task.name)
       if (task_name == "finalize")
 	       info("Slave" + s_mailbox + "got finalize msg")
	       break
       end
       info("Slave " + s_mailbox + " ...Processing" + Task.name(task))
       Task.execute(task)
    end
    info("Slave " + s_mailbox +  "I'm Done , See You !!")
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
  #Thread.list.each {|t| p t}
end

# Thread.list.each {|t| p t}
MSG.run()
MSG.getClock()
# exit()
