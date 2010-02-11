require 'msg'
require 'Semaphore'
include MSG

$DEBUG = true  # This is a Global Variable Useful for Debugging


class RbProcess < Thread 
  @@nextProcessId = 0
# Attributes
  attr_accessor :bind, :id, :proprieties, :name,
      :pargs, :schedBegin, :schedEnd
  
# Initialize : USED in ApplicationHandler to Initialize it
  def initialize(*args)
    
    argc = args.size
#      Default Init***************************** No Args
    if argc == 0    #>>> new()
    super() {
    @id = 0
    @bind = 0
    @name = ""
    @pargs = Array.new()
    init_var()
     if $DEBUG
	puts "Init Default Initialzer..."
     end  
#     Thread.pass  
#     sleep   # Sleep Forever ... To Keep Thread Alive ?!!
    
    }
    end
    
    
    # Init with 1 Argument ***********************>>Name( Not Used )
    if argc == 1
      super(){
	@name = args[0]
	@pargs = Array.new()
	init_var()
	if $DEBUG
	  puts "Init with Name..."
	end
      }
    end
    
  
    
    # Init with 2 arguments **********************>>>(HostName,Name) Or (Host , Name)
    
    if argc == 2   
      super(){
      type = args[0].type()
      if ( type.to_s == "String")
        host = Host.getByName(args[0])
      end
      if ( type.to_s == "MSG::Host")
        host = args[0]  
      end
      if $DEBUG
        puts host
      end
      raise "Process Name Cannot Be Null"   if args[1].empty?
      @name = args[1] # First Arg
      if $DEBUG
        puts @name
      end
      @pargs = Array.new()    # No Args[] Passed in Arguments
      @@nextProcessId += 1
      @id = @@nextProcessId
      init_var()
      createProcess(self,host)
      if $DEBUG
      puts "Initilize with 2 args"
      end
      
#       sleep  
      }
    
    
    end
    
    
    
       
    # Init with 3 arguments **********************(hostName,Name,args[]) or # (Host,Name,args[])
    
    if argc == 3  
      super(){
	type = args[0].type()
        if( type.to_s == "String")
	  host =Host.getByName(args[0])
        end
        if ( type.to_s == "MSG::Host" )
        host = args[0]
        end
        if $DEBUG
        puts host
        end
      
        raise "Process Name Cannot Be Null" if args[0].empty? 
        @name = args[1]
        type = args[2].type()
        raise "Third Argument Should be an Array" if type != "Array"
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
    end

    end

    
    # Init_var Called By Initialize  
    
    
  def init_var()
    
    @proprieties = Hash.new()
    # Process Synchronization Tools
     @schedBegin = Semaphore.new(0)
     @schedEnd = Semaphore.new(0)
      
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
  
    #get Own Name
  
  def getName()
   
    return @name
  
  end
  
  #get a Process Name
  
  def processName(process)
  
    return process.name
  
  end
  
  #get Bind
  def getBind()
    
    return @bind
    
  end
  
  #set Binds
  def setBind(bind)
    
    @bind = bind
    
  end
    
      # Stop
  
  def unschedule() 
    
    Thread.pass
  
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
  
#    The Rest of Methods !!! To be Continued ...

end