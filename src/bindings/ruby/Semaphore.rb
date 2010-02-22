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
require 'thread'
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