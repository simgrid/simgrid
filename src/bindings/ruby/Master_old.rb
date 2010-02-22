require 'msg'
require 'RubyProcess'

include MSG

class Master < RbProcess  
  def initialize()
    super()
  end

  def msg_main2(args)
  info("Hello From Master")    
  end
  
  # msg_main : that function that will be executed when Running Simulation
  def msg_main(args) # args is an array Containin' arguments for function master
    info("Hello From Master") 
    size = args.size
    puts "Number of Args for Master = " + size.to_s
   for i in 0..size-1
      puts  args[i]
   end
   
   raise "Master needs 3 arguments" if size < 3 
   channel = 0
   numberOfTask = Integer(args[0]) 
   taskComputeSize = Float(args[1])
   taskCommunicationSize = Float(args[2])
   slaveCount = args.size - 3
   
   slaves = Array.new
   todo = Array.new
   
   
   #Creating & Sending Task
   for i in 0..numberOfTask-1
  
     todo[i] = RbTask.new("Task_"+ i.to_s, taskComputeSize , taskCommunicationSize );
     
   end
   
   for i in 3..args.size-1
     
     slaves[i-3] = RbHost.getByName(args[i])
   end
     
   
   for i in 0..numberOfTask-1
     p "Sending ( Put )  Task .." + i.to_s+ " to Slave " + RbHost.name(slaves[i%slaveCount])
     RbTask.put(todo[i],slaves[i%slaveCount])
   end
  
   # Sending Finalize Tasks
   p "Sending Finalize Task Later !!"
   puts "Master : Everything's Done"
  end  
  
end