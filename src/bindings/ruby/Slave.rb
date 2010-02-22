require 'msg'
require 'RubyProcess'
require 'RubyTask'
include MSG

class Slave < RbProcess
  
  def initialize()
    super()
  end
  
  #for Testing
  def msg_main2(args)
    info("Hello From Slave")
  end
  
  
  
  
  
 # msg_main : that function that will be executed when Running Simulation
  def msg_main(args)
    info("Hello From Slave")
    s_mailbox = "slave>>" + args[0]

    while true
        
       info("Ready to Receive Task")
       task = RbTask.receive(s_mailbox)
       task_name = RbTask.name(task)
       info ("Task Received : " + task_name)
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
