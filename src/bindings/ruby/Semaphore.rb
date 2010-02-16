# 
#  * $Id$
#  *
#  * Copyright 2010 Martin Quinson, Mehdi Fekari           
#  * All right reserved. 
#  *
#  * This program is free software; you can redistribute 
#  * it and/or modify it under the terms of the license 
#  *(GNU LGPL) which comes with this package. 
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
       @cv.wait(@mutex)
    end
    @permits = @permits - 1
    }
  end
    
  def release()
    @mutex.synchronize{
      
      @permits += 1
      @cv.signal
       
      }
  end
  
end