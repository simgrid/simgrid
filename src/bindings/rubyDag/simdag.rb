require 'dag'

#===========================
# Task Class
# ==========================
class DAG::Task < DAG::SdTask
  
  def initialize(*args)
    super(args)
  end
  
  def name()
    super(self)
  end
  
  def startTime()
    super(self)
  end
  
  def finishTime()
    super(self)
  end
  
end

#=============================
# WorkStation Class
#=============================
class DAG::Workstation < DAG::SdWorkstation
  
  def initialize()
    super()
  end
  
  def name()
    super(self)
  end
  
end