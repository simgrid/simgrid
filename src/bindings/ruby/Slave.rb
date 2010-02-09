require 'msg'
require 'RubyProcess'
require 'RubyTask'
include MSG

class Slave < RbProcess
  
  def initialize(*args)
    
    
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
    
    
  end
  
  

# slave = Slave.new