#  Task-related bindings to ruby  */
# 
#  Copyright 2010. The SimGrid Team. All right reserved. */
# 
# This program is free software; you can redistribute it and/or modify it
#  under the terms of the license (GNU LGPL) which comes with this package. */
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
require 'simgrid_ruby'
require 'thread'

#######################################
#  Semaphore
#######################################

class Semaphore
  def initialize(initvalue = 0)
    @counter = initvalue
    @waiting_list = []
  end

  def acquire
    Thread.critical = true
    if (@counter -= 1) < 0
      MSG::debug(Thread.current.to_s+" acquires "+self.to_s+". That's blocking.")
      @waiting_list.push(Thread.current)
      Thread.stop
    else
      MSG::debug(Thread.current.to_s+" acquires "+self.to_s+". It was free.")      
    end
    self
  ensure
    Thread.critical = false
  end

  def release
    Thread.critical = true
    begin
      if (@counter += 1) <= 0
        t = @waiting_list.shift
        t.wakeup if t
        MSG::debug(Thread.current.to_s+" releases "+self.to_s+". Wakeup "+t.to_s)
      else 
        MSG::debug(Thread.current.to_s+" releases "+self.to_s+". Nobody to wakeup")
      end
    rescue ThreadError
      retry
    end
    self
  ensure
    Thread.critical = false
  end
end

########################################################################
# Class Process 
########################################################################
class MSG::Process < Thread 
  @@nextProcessId = 0

# Attributes
  attr_reader :name, :pargs ,:properties	# Read only
  
    def initialize(*args)
      super(){
	
     raise "Bad number of arguments to create a Ruby process. Expected (name,args,prop) " if args.size < 3
     
    @schedBegin = Semaphore.new(0)
    @schedEnd = Semaphore.new(0)    
    @id = @@nextProcessId
    @@nextProcessId +=1
    @name = args[0]
    @pargs = args[1]
    @properties = args[2]
    start()
      }
    end
    
  def main(args) 
    # To be overriden by childs
    raise("You must define a main() function in your process, containing the code of this process")
  end
     
  def start()
     @schedBegin.acquire
    MSG::debug("Let's execute the main() of the Ruby process")
    main(@pargs)
    @schedEnd.release
    MSG::debug("Released my schedEnd, bailing out")
    processExit(self) # Exit the Native Process
    
  end
    
  def getBind()
    return @bind
  end
   
  def setBind(bind)
    @bind = bind
  end
    
  def unschedule()
    @schedEnd.release
    @schedBegin.acquire
  end
  
  def schedule()   
    @schedBegin.release
    @schedEnd.acquire
  end
  
  def pause()
    processSuspend(self)
  end
  
  def restart()
    processResume(self)
  end
  
  def isSuspended()
    processIsSuspended(self)
  end
  
  def getHost()
    processGetHost(self)
  end

# The Rest of Methods !!! To be Continued ... FIXME: what's missing?
end
############################################
# Task Extend from the native Class RbTask
############################################
class MSG::Task < MSG::RbTask

  def initialize(*args)
   super() #no effect
  end
  
  def setData(value)
    super(self,value)
  end
  
  def data()
    super(self)
  end
  
  def name
      super(self)
  end
  
  def compSize
     super(self)
  end
  
  def send(mailbox)
    super(self,mailbox)
  end
  
  def source
    super(self)
  end
  
  def sender
    super(self)
  end
  
  def listen(t_alias)
    super(t_alias)
  end
  
  def execute
    super(self)
  end
    
  def listenFromHost(t_alias,host)
    super(t_alias,host)
  end
  
  def setPriority(priority)
    super(self,priority)
  end
  
  def cancel()
    super(self)
  end
  
  def hasData()
    super(self)
  end
   
end  
####################################################
# Host Extend from the native Class RbHost
####################################################
class MSG::Host < MSG::RbHost
  
  attr_reader :data
  def initialize(*ars)
    @data = 1
    p "Host Initializer"
  end
  
  def data()
    return @data
  end
  
  def setData(value)
    @data = value
  end
  
  def getByName(name)
    super(name)
  end
  
  def name
    super(self)
  end
  
  def speed
    super(self)
  end
  
  def getData
    super(self)
  end
  
  
  def isAvail
    super(self)
  end
  
  def number
    super()
  end
  
  def getHostProcess(process)
    super(process)
  end
    
end
#########################
# Main chunck 
#########################
MSG.init(ARGV)