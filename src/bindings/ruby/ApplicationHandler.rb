require 'ProcessFactory'

$debug = true

class ApplicationHandler

  @processFactory
  
#   Initialize
  def initialize()
     
  end
  
    
#  onStartDocument
  def onStartDocument()
    
    @processFactory = ProcessFactory.new
    
    if ($debug)
      puts "onStartDocument"
    end
      
    
  end
  
#   onBeginProcess
  def onBeginProcess (hostName ,function)
    
    @processFactory.setProcessIdentity(hostName,function)
    
     if ($debug)
      puts "onBeginProcess"
     end
    
  end

#   onProperty
  
  def onProperty(id,value)
    
    @processFactory.setProperty(id,value)
    
     if ($debug)
      puts "onProperty"
     end
    
  end
  
#   RegisterProcessArg
  
  def onProcessArg(arg)
    
    @processFactory.registerProcessArg(arg)
      
      if ($debug)
      puts "onProcessArg"
      end
    
  end

#   OnEndProcess
  
 def onEndProcess()
   
   @processFactory.createProcess()
   
   if ($debug)
      puts "onEndProcess"
   end
      
 end


 #  onEndDocument
 
 def onEndDocument()  
#    Euh...Actually Nothin' to Do !!
   
   if($debug)
   puts "onEndDocument"
   end
 end
  
 
#  End Class
 
end

