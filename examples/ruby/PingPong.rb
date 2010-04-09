require 'simgrid.so'

include MSG

#####################
#PingPongTask Class
#####################

class PingPongTask < MSG::Task
  
  # The initialize method has no effect 
  @time 
  def setTime(t)
    @time = t
  end
  
  def getTime()
    return @time
  end
end

####################
# Sender Class
####################

class Sender < MSG::Process
  
 def main(args)
   MSG::info("Hello from Sender")
   hostCount = args.size
   MSG::info("Host count :" + hostCount.to_s)
   mailboxes = Array.new
      
   for i in 0..hostCount-1
     mailboxes<< MSG::Host.getByName(args[i]).name
   end
   
   for i in 0..hostCount-1
     time = MSG.getClock
     MSG::info("sender time :"+time.to_s)
     task = PingPongTask.new("PingTask",10000,2000)
     MSG::info("task created :" + task.name);
     #task.join(time) -- MSG::task.join(data) is a Native method you can use to attach any data you want to the task
     task.setTime(time)
     task.send(mailboxes[i])
   end
   MSG::info("Bye!!")     
   end
 end
  
####################
# Receiver Class
####################

class Receiver < MSG::Process
  
  def main(args)
    MSG::info("Hello from Receiver")
    time = MSG.getClock
    host = MSG::Host.getHostProcess(self)
    task = PingPongTask.receive(host.name)
    timeGot = MSG.getClock
    MSG::info("Got at time: "+timeGot.to_s)
    #timeSent = task.data -- 
    timeSent = task.getTime
    MSG::info("Was sent at time "+timeSent.to_s)
    communicationTime = timeGot - time
    MSG::info("Communication Time: "+communicationTime.to_s)
    MSG::info("--- bw "+(100000000/communicationTime).to_s+" ----")
    MSG::info("Bye!!")
  end
  
end

#################################################
# main chunck
#################################################

if (ARGV.length == 2) 
	MSG.createEnvironment(ARGV[0])
	MSG.deployApplication(ARGV[1])
else 
	MSG.createEnvironment("ping_pong_platform.xml")
	MSG.deployApplication("ping_pong_deployment.xml")
end

MSG.run
MSG.exit
