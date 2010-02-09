require 'thread'
class Semaphore
  
  
  Thread.abort_on_exception = true

  attr_accessor :permits, :mutex, :cv
  
  
  
  def initialize ( permits )
    
    
      @permits = permits
      @mutex = Mutex.new
      @cv = ConditionVariable.new
    
  end
  
  
  def acquire()

    raise "Interrupted Thread " if (!Thread.current.alive?)
    @mutex.synchronize {
    while @permits < 1
      @cv.wait(mutex)
    end
    @permits = @permits - 1
    }
  end
    
  
  def release()
    @mutex.synchronize{
      @value = @value + 1
      @cv.signal
         
      }
    
  end
  
  

  
end