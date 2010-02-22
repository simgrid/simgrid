require 'msg'
require 'RubyProcess'
require 'RubyTask'
include MSG

class Slave < RbProcess
  
  def initialize()
    super()
  end
  
  def msg_main(args)
    info("Hello From Slave")
  end
  
 # msg_main : that function that will be executed when Running Simulation
  def msg_main(args)
    info("Hello From Slave")
    while true
        
       task = RbTask.get(); #by default Port set to 0
       task_name = RbTask.name(task)
       puts "my name ........" + task_name
      if ( task_name == "finalize" )
	puts "Received finalize msg"
	break
      end
       puts "Processing..." + RbTask.name(task)
       RbTask.execute(task)
    end
    puts "I'm Done ... See You !!"
    break
    end
    
    
  end
  