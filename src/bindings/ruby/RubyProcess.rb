# 
#  * $Id$
#  *
#  * Copyright 2010 Martin Quinson, Mehdi Fekari           
#  * All right reserved. 
#  *
#  * This program is free software; you can redistribute 
#  * it and/or modify it under the terms of the license 
#  *(GNU LGPL) which comes with this package. 

require 'msg'
require 'Semaphore'
include MSG
$DEBUG = false  # This is a Global Variable Useful for Debugging

class RbProcess < Thread 
  @@nextProcessId = 0
# Attributes
  attr_accessor :bind, :id, :proprieties, :name,
      :pargs, :schedBegin, :schedEnd
  
# Initialize : USED in ApplicationHandler to Initialize it
  def initialize(*args)
    
    argc = args.size
# Default initializer
    if argc == 0    #>>> new()  
    super() {
    @id = 0
    @bind = 0
    @name = ""
    @pargs = Array.new()
    init_var()
    start()
     if $DEBUG
	puts "Init Default Initialzer...Nothing to do...Bye"
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
      start()
      createProcess(self,host)
	if $DEBUG
	  puts "Initilize with 2 args"
	end
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
  
  #main
  def msg_main(args)
    # To Be Implemented within The Process...
    # The Main Code of The Process to be Executed ...
  end
     
  
  # Start : To keep the Process Alive and waitin' via semaphore
  def start()
    
    @schedBegin.acquire()
    #execute The Main Code of The Process ( Example Master ; Slave ...)     
    msg_main(@pargs)
    processExit(self) #Exite the Native Process
    @schedEnd.release()
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
#     Thread.pass
    @schedEnd.release()
    @schedBegin.acquire()
  end
  
  def schedule()
    @schedBegin.release()
    @schedEnd.release()
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