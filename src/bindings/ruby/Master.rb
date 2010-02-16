require 'msg'
require 'RubyProcess'

include MSG

class Master < RbProcess  
  def initialize()
    super()
  end

 
  # msg_main : that function that will be executed when Running Simulation
  def msg_main(args) # args is an array Containin' arguments for function master
    puts "Hey From Ruby...I'm The Master" 
    size = args.size
    puts "Number of Args for Master = " + size.to_s
   for i in 0..size-1
      puts  args[i]
   end
   
   raise "Master needs 3 arguments" if size < 3 
   numberOfTask = Integer(args[0]) 
   taskComputeSize = Float(args[1])
   taskCommunicationSize = Float(args[2])
   slaveCount = Integer(args[3]) 
   
   #Creating & Sending Task
   for i in 0..numberOfTask
  
     task = RbTask.new("Task_" + i.to_s, taskComputeSize , taskCommunicationSize );
     s_alias = "slave>>" + (i%slaveCount).to_s
     puts "Master Sending "+ RbTask.name(task) + " to " + s_alias + " with Comput Size " + RbTask.compSize(task).to_s 
     RbTask.send(task,s_alias)
     puts "Master Done Sending " +RbTask.name(task) + " to " + s_alias
  
   end
  
   # Sending Finalize Tasks
   puts "Master: All tasks have been dispatched. Let's tell everybody the computation is over."
   for i in 0..slaveCount-1
     s_alias = "slave " + i.to_s
     puts "Master Sending Finalize to " + s_alias
     RbTask.send(RbTask.new("finalize",0,0),s_alias)
   end
   puts "Master : Everything's Done"
  end  
  
end