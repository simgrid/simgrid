class Sem
  
  
  attr_accessor :permits, :mutex, :cv
  
  Thread.abort_on_exception = true
  

  
#   Initialize
  def initialize ( permits )
    
    @permits = permits
    @mutex = Mutex.new
    @cv = ConditionVariable.new
    
  end
  
  
  
#   Aquire
  def acquire()
    
    if(!Thread.current.alive?)    #Thread.interrupted in Java
      raise "Exception : Thread Interrupted"      
    end
  
    mutex.synchronize {
      
      while @permits <= 0 
    
        @cv.wait(mutex)   #or Thread.stop ???!!
       
    end
      
    @permits -=1
      }
         
  end
  
  
  def release()
    
    mutex.synchronize {
      
      @permits +=1
      @cv.signal    #Notify ??!!
      
      }
    
  
end