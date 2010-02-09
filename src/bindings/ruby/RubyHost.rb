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