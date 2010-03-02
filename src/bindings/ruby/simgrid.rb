require 'simgrid_ruby'
include MSG
require 'thread'

$DEBUG = true  # This is a Global Variable Useful for Debugging

###########################################################################
# Class Semaphore 
###########################################################################
class Semaphore 
  Thread.abort_on_exception = true
  attr_accessor :permits
 
  def initialize ( permits )
       @permits = permits
  end
   

  def acquire(mutex,cv)
    raise "Interrupted Thread " if (!Thread.current.alive?)
    mutex.synchronize {
      while @permits <= 0 
        cv.wait(mutex)       
      end
    
      @permits = @permits - 1
      cv.signal    
    }    
  end
    
  def release(mutex,cv)
    mutex.synchronize{
      @permits += 1
      cv.signal
    }
  end
end

########################################################################
# Class RbProcess 
########################################################################
class RbProcess < Thread 
  @@nextProcessId = 0
# Attributes
  attr_accessor :bind, :id, :properties, :name,
      :pargs, :schedBegin, :schedEnd, :mutex, :cv
  
# Initialize : Used from ApplicationHandler to fill it in
  def initialize(*args)
    
    argc = args.size

    if argc == 0 # Default initializer
      super() {
        @id = 0
        @bind = 0
        @name = ""
        @pargs = Array.new()
        init_var()
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
        @@nextProcessId += 1
        @id = @@nextProcessId
        init_var()
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
        @@nextProcessId +=1
        @id = @@nextProcessId
        init_var()
        createProcess(self,host)  
        
      if $DEBUG
      	puts "Initilize with 3 args"
      end
      
#       sleep #keep the thread running
	   }
  else 
    raise "Bad number of argument: Expecting either 1, 2 or 3, but got "+argc
  end
    end

  # Init_var Called By Initialize  
  def init_var()  
    @proprieties = Hash.new()
    @mutex = Mutex.new
    @cv = ConditionVariable.new
    # Process Synchronization Tools
    @schedBegin = Semaphore.new(0)
    @schedEnd = Semaphore.new(0)    
  end
  
  #main
  def msg_main(args)
    # To Be Implemented within The Process...
    # The Main Code of The Process to be Executed ...
  end
     
  # Start : To keep the Process Alive and waitin' via semaphore
  def start()
    @schedBegin.acquire(@mutex,@cv)
    #execute The Main Code of The Process ( Example Master ; Slave ...)     
    msg_main(@pargs)
    processExit(self) #Exite the Native Process
    @schedEnd.release(@mutex,@cv)
  end
    
#   NetxId
  def nextId ()
    @@nextProcessId +=1
    return @@nextProcessId
  end

  if $DEBUG
    #Process List
    def processList()
      Thread.list.each {|t| p t}
    end
  end
  
  #Get Own ID
  def getID()
    return @id
  end
  
  # set Id
  def setID(id)
    @id = id
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
    
    @schedEnd.release(@mutex,@cv)
#     info("@schedEnd.release(@mutex,@cv)")
    @schedBegin.acquire(@mutex,@cv)
#     info("@schedBegin.acquire(@mutex,@cv)")
     
  end
  
  def schedule()
    @schedBegin.release(@mutex,@cv)
    @schedEnd.acquire(@mutex,@cv)
  end
  
   #C Simualateur Process Equivalent  Management
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
   attr_accessor :args, :proprieties, :hostName, :function
#    Initlialize
    def initialize()
    
    @args = Array.new
    @proprieties = Hash.new
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
      
      if !proprieties.empty?
	proprieties.clear   
      end
    
    end

#     RegisterProcess  
    def registerProcessArg(arg)
      
      @args.push(arg)
      
    end

#     CreateProcess
    def createProcess()
      
      process = rubyNewInstance(@function) # process = rubyNewInstanceArgs(@function,@args) #
      size = @args.size
      for i in 0..size-1
      	process.pargs.push(@args[i]) 
      end
      process.name = @function
      process.id = process.nextId() # This increment Automaticaly  The Static ProcessNextId for The Class RbProces
      host = RbHost.getByName(@hostName)
      processCreate(process,host)
      process.properties = @properties
      @proprieties = Hash.new
      
    end
    
#     SetProperty
    def setProperty(id,value)
      @proprieties[id] = value
    end
end

#########################################################################
# Class ApplicationHandler
#########################################################################
class ApplicationHandler
  @processFactory  
#   Initialize
  def initialize()
     #Nothing todo
  end
  
    #  onStartDocument
  def onStartDocument()
    
    @processFactory = ProcessFactory.new
    if ($DEBUG)
      puts "onStartDocument"
    end
      
  end
  
#   onBeginProcess
  def onBeginProcess(hostName,function)
    
    @processFactory.setProcessIdentity(hostName,function)
    
     if ($DEBUG)
      puts "onBeginProcess"
     end
    
  end

#   onProperty
    def onProperty(id,value)
    
    @processFactory.setProperty(id,value)
    
     if ($DEBUG)
      puts "onProperty"
     end
    
  end
  
#   RegisterProcessArg
    def onProcessArg(arg)
    
    @processFactory.registerProcessArg(arg)
      
      if ($DEBUG)
      puts "onProcessArg"
      end
    
  end

#   OnEndProcess
   def onEndProcess()
   
   @processFactory.createProcess()
   
   if ($DEBUG)
      puts "onEndProcess"
   end
      
 end

 #  onEndDocument
  def onEndDocument()  
#    Erm... Actually nothing to do !!
   
   if($DEBUG)
   puts "onEndDocument"
   end
 end
 
 #  End Class
 end
 
#########################
# Class RbHost 
#########################

class RbHost < Host
# Attributes
  attr_accessor :bind, :data 
  
# Initialize
  def initialize()
    super()
    @bind = 0
    @data = nil
  end
  
end

#########################
# Class RbTask 
#########################
class RbTask < Task  
  attr_accessor :bind 
  
  def initialize(name,comp_size,comm_size)
    super(name,comp_size,comm_size)
  end

end
 
#########################
# Main chunck 
#########################
MSG.init(ARGV)
