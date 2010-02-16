require 'msg'
require 'RubyProcess'
require 'RubyTask'
include MSG

class Slave < RbProcess
  
  def initialize()
    super()
  end
  
 # msg_main : that function that will be executed when Running Simulation
  def msg_main(args)
    puts "Hello From Slave"
    s_mailbox = "slave>>" + args[0]
    while true
     
      p "Hellow...................here3 "+s_mailbox
      task = RbTask.receive(s_mailbox)
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