require 'simgrid_ruby'
include MSG
require 'thread'

$DEBUG = false  # This is a Global Variable Useful for Debugging

###########################################################################
# Class Semaphore 
###########################################################################

class Semaphore
  def initialize(initvalue = 0)
    @counter = initvalue
    @waiting_list = []
  end

  def acquire
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
    Thread.critical = true
    begin
      if (@counter += 1) <= 0
  t = @waiting_list.shift
  t.wakeup if t
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
class MsgProcess < Thread 
  @@nextProcessId = 0

# Attributes
  attr_reader :bind, :id, :pargs    # Read only
  attr_accessor :name, :properties  # R/W
  
# Initialize : Used from ApplicationHandler to fill it in
  def initialize(*args)
    @schedBegin = Semaphore.new(0)
    @schedEnd = Semaphore.new(0)    
    @properties = Hash.new()
    @id = @@nextProcessId++
    
    argc = args.size

    if argc == 0 # Default initializer
      super() {
        @id = 0
        @bind = 0
        @name = ""
        @pargs = Array.new()
        start()
        if $DEBUG
        	puts "Init Default Initializer...Nothing to do...Bye"
        end  
      }
       
    # 2 arguments: (HostName,Name) Or (Host , Name)
    elsif argc == 2   
      super(){
        type = args[0].type()
        if ( type.to_s == "String")
          host = Host.getByName(args[0])
        elsif ( type.to_s == "MSG::Host")
          host = args[0]
        else 
          raise "first argument of type "+args[0].type().to_s+", but expecting either String or MSG::Host"
        end
        if $DEBUG
          puts host
        end
        raise "Process name cannot be null" if args[1].empty?
        @name = args[1] 
        if $DEBUG
          puts @name
        end
        @pargs = Array.new()    # No Args[] Passed in Arguments
        start()
        createProcess(self,host)
	      if $DEBUG
	        puts "Initilize with 2 args"
        end
      }
       
    # 3 arguments: (hostName,Name,args[]) or (Host,Name,args[])
    elsif argc == 3  
      super(){
        type = args[0].type()
        if ( type.to_s == "String")
          host = Host.getByName(args[0])
        elsif ( type.to_s == "MSG::Host")
          host = args[0]
        else 
          raise "first argument of type "+args[0].type().to_s+", but expecting either String or MSG::Host"
        end
        if $DEBUG
          puts host
        end
      
        raise "Process name cannot be null" if args[1].empty?
        @name = args[1]
        type = args[2].type()
        raise "Third argument should be an Array" if type != "Array"
        @pargs = args[3]
        createProcess(self,host)  
        
      if $DEBUG
      	puts "Initilize with 3 args"
      end      
	   }
  else 
    raise "Bad number of argument: Expecting either 1, 2 or 3, but got "+argc
  end
    end
  
  # main
  def msg_main(args) 
    # To be overriden by childs
    raise("You must define a msg_main() function in your process, containing the code of this process")
  end
     
  # Start : To keep the process alive and waiting via semaphore
  def start()
    @schedBegin.acquire
    # execute the main code of the process     
    debug("Begin execution")
    msg_main(@pargs)
    processExit(self) # Exit the Native Process
    @schedEnd.release
  end
    
  def processList()
    Thread.list.each {|t| p t}
  end
  
  #Get Own ID
  def getID()
    return @id
  end
  
  #Get a Process ID
  def processID(process)
    return process.id
  end
  
  #Get Own Name
  def getName()
    return @name
  end
  
  #Get a Process Name
  def processName(process)
    return process.name
  end
  
  #Get Bind
  def getBind()
    return @bind
  end
  
  #Get Binds
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

########################################################################
# Class ProcessFactory
########################################################################

class ProcessFactory 

#     Attributes
   attr_accessor :args, :properties, :hostName, :function
#    Initialize
    def initialize()
    
    @args = Array.new
    @properties = Hash.new
    @hostName = nil
    @function = nil
    
    end
    
#     setProcessIdentity
    def setProcessIdentity(hostName,function)
      @hostName = hostName
      @function = function
      
      if !args.empty?
	       args.clear
      end
      
      if !properties.empty?
	properties.clear   
      end
    
    end

    def registerProcessArg(arg)
      @args.push(arg)
    end

#     CreateProcess
    def createProcess()
      process = rubyNewInstance(@function) 
      size = @args.size
      for i in 0..size-1
      	process.pargs.push(@args[i]) 
      end
      process.name = @function
      host = Host.getByName(@hostName)
      processCreate(process,host)
      process.properties = @properties
      @properties = Hash.new
      
    end
    
#     SetProperty
    def setProperty(id,value)
      @properties[id] = value
    end
end

#########################################################################
# Class ApplicationHandler
#########################################################################
class ApplicationHandler
  @processFactory  
  
  def initialize()
    @processFactory = ProcessFactory.new
  end
  
  def onBeginProcess(hostName,function)
    @processFactory.setProcessIdentity(hostName,function)
    debug("onBeginProcess("+hostName+","+function+")")
  end

  def onProperty(id,value)
    @processFactory.setProperty(id,value)
  end
  
  def onProcessArg(arg)
    @processFactory.registerProcessArg(arg)
  end

  def onEndProcess()
   @processFactory.createProcess()
   debug("onEndProcess")
  end
end
 
#########################
# Main chunck 
#########################
MSG.init(ARGV)
