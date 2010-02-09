require 'msg'
require 'RubyProcess'

def Master(*args)
  
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


def Slave(*args)
  
   puts "Hello From Slave"
    s_mailbox = "slave" + args[0]
    
    
    while true
      
      task = RbTask.recieve(s_mailbox)
      
      task_name = RbTask.name(task)
      
      if ( task_name == "finalize" )
	puts "Slave" + s_mailbox + "got finalize msg"
	break
      end
      
      puts "Slave " + s_mailbox + "Processing" + RbTask.name(task)
      RbTask.execute(task)
      
    end
	
    puts "Slave " + s_mailbox + "I'm Done , See You !!"
  
  
end