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
include MSG

class RbTask < Task
  
  
# Attributes
  attr_accessor :bind 
  
  
#   Initialize
  def initialize(name,comp_size,comm_size)
#      @bind = 10
    super(name,comp_size,comm_size)
    end
  

end