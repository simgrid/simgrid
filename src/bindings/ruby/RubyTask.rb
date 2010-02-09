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