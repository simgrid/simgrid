require 'msg'
require 'RubyProcess'

include MSG

class Master < RbProcess
  
  

 
  
  def initialize(*args)
    
    super()
    channel = 0
    puts "Hey ..from Master"#info("Hey")
    
   slaves = Array.new()
   
   size = args.size
   
   puts "Args = " + size
   
   for i in 0..size-1
     puts "argv :" + args[1]
   end
   
   raise "Master needs 3 arguments" if size < 3
   
   numberOfTask = args[0] #convert to int
   taskComputeSize = args[1] #convert to double
   taskCommunicationSize = args[2] #convert to double
   slaveCount = args[3] #convert to int
   
   
#    todo = Array.new(numberOfTask)
   
   #Creating & Sending Task
   for i in 0..numberOfTask 
     
     task = RbTask.new("Task_" + i.to_s, taskComputeSize , taskCommunicationSize );
     s_alias = "slave " + (i%slaveCount).to_s
     puts "Master Sending "+ RbTask.name(task) + " to " + s_alias
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


