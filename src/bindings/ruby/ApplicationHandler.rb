require 'ProcessFactory'

$DEBUG = true

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
#    Euh...Actually Nothin' to Do !!
   
   if($DEBUG)
   puts "onEndDocument"
   end
 end
 
 #  End Class
 end

