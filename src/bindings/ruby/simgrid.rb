require 'simgrid_ruby'
require 'thread'

$DEBUG = false  # This is a Global Variable Useful for MSG::debugging

###########################################################################
# Class Semaphore 
###########################################################################

class Semaphore
  def initialize(initvalue = 0)
    @counter = initvalue
    @waiting_list = []
  end

  def acquire
    MSG::debug("Acquire "+self.to_s)
    Thread.critical = true
    if (@counter -= 1) < 0
      @waiting_list.push(Thread.current)
      Thread.stop
    end
    self
  ensure
    Thread.critical = false
  end

  def release
    MSG::debug("Release "+self.to_s)
    Thread.critical = true
    begin
      if (@counter += 1) <= 0
        t = @waiting_list.shift
        t.wakeup if t
        MSG::debug("Wakeup "+t.to_s)
      else 
        MSG::debug("Nobody to wakeup")
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
  attr_reader :bind, :id ,:name           # Read only
  attr_accessor :properties, :pargs  # R/W
  

    def initialize(*args)
      super(){
	
     raise "Bad Number Of arguments to create a a Ruby Process (name,args,prop) " if args.size < 3
     
    @schedBegin = Semaphore.new(0)
    @schedEnd = Semaphore.new(0)    
    #@properties = Hash.new()
    @id = @@nextProcessId
    @@nextProcessId += 1
    @name = args[0]
    @pargs = args[1]
    @properties = args[2]
    
     start()
      
      }
      
    end
    
    
  # main
  def main(args) 
    # To be overriden by childs
    raise("You must define a main() function in your process, containing the code of this process")
  end
     
  # Start : To keep the process alive and waiting via semaphore
  def start()
    @schedBegin.acquire
    # execute the main code of the process     
    MSG::debug("Begin execution")
    main(@pargs)
    processExit(self) # Exit the Native Process
    @schedEnd.release
  end
    

  
  #Get Bind ( Used > Native to Ruby)
  def getBind()
    return @bind
  end
  
  #Get Binds (Used > Ruby to Native)
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
  
   #C Simualator Process Equivalent  Management
  # After Binding Ruby Process to C Process
  
#   pause
  def pause()
    processSuspend(self)
  end
  
#   restart
  def restart()
    processResume(self)
  end
  
#   isSuspended
  def isSuspended()
    processIsSuspended(self)
  end
  
#   getHost
  def getHost()
    processGetHost(self)
  end
  
# The Rest of Methods !!! To be Continued ...
end

#########################
# Main chunck 
#########################
MSG.init(ARGV)
