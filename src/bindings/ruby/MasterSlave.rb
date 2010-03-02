require 'simgrid'
include MSG

#################################################
# Class Master
#################################################

class Master < RbProcess  
  def initialize2()
    super()
  end
 
  
  # msg_main : that function that will be executed when Running Simulation
  def msg_main(args) # args is an array containing arguments for function master
    info("Hello From Master")
    size = args.size
    info ("Number of Args for Master = " + size.to_s)
   for i in 0..size-1
      info("args["+String(i)+"]="+args[i])
   end
   
   raise "Master needs 3 arguments" if size < 3 
   numberOfTask = Integer(args[0]) 
   taskComputeSize = Float(args[1])
   taskCommunicationSize = Float(args[2])
   slaveCount = Integer(args[3]) 
   
   #Creating & Sending Task
   for i in 0..numberOfTask-1
  
     
     task = RbTask.new("Task_"+ i.to_s, taskComputeSize , taskCommunicationSize );
     s_alias = "slave>>" + (i%slaveCount).to_s
     info("Master Sending "+ RbTask.name(task) + " to " + s_alias + " with Comput Size " + RbTask.compSize(task).to_s)
     RbTask.send(task,s_alias)
     info("Master Done Sending " +RbTask.name(task) + " to " + s_alias)
#       sameTask = RbTask.receive(s_alias)
#      puts "Master Receiving its Own Task"
   end
  
   # Sending Finalize Tasks
   info ("Master: All tasks have been dispatched. Let's tell everybody the computation is over.")
   for i in 0..slaveCount-1
     s_alias = "slave " + i.to_s
     info ("Master Sending Finalize to " + s_alias)
     RbTask.send(RbTask.new("finalize",0,0),s_alias)
   end
   info("Master : Everything's Done")
  end    
end

#################################################
# Class Slave
#################################################
class Slave < RbProcess
  
  def initialize()
    super()
  end
  
 # msg_main : that function that will be executed when Running Simulation
  def msg_main(args)
    info("Hello From Slave")
    s_mailbox = "slave>>" + args[0]

    while true
        
       info("Ready to Receive Task")
       task = RbTask.receive(s_mailbox)
       task_name = RbTask.name(task)
       info ("Task Received : " + task.name)
      if (task_name == "finalize")
	info("Slave" + s_mailbox + "got finalize msg")
	break
      end
      info("Slave " + s_mailbox + " ...Processing" + RbTask.name(task))
       RbTask.execute(task)
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
  Thread.list.each {|t| p t}
end

# Thread.list.each {|t| p t}
MSG.run()
MSG.getClock()
# exit()
