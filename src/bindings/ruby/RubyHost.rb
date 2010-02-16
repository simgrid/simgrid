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

class RbHost < Host
# Attributes
  attr_accessor :bind, :data 
  
# Initialize
  def initialize()
    @bind = 0
    @data = nil
  end
  
end