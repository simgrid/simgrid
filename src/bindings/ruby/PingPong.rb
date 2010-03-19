require 'simgrid'

include MSG

#####################
#PingPongTask Class
#####################

class PingPongTask < MSG::Task
  

  def initialize(*args)
    #Has No Role, Since Its The Task Class ( Created from C ) tht will be called , 
    # and any instruction here will be ignored
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
     
#      FIXME Handel Exceptions 
   end
   
   for i in 0..hostCount-1
     time = MSG.getClock.to_s # to send as a data >> must be a string
     MSG::info("sender time :"+time.to_s)
     task = PingPongTask.new("PingTask",10000,2000)
     task.setData(time)
     p "mailboxe >>> "+ mailboxes[i]
     task.send(mailboxes[i])
   end
   
   MSG::info("GoodBye !!!")
     
   end
   
 end
  
####################
# Receiver Class
####################

class Receiver < MSG::Process
  
  def main(args)
    
    MSG::info("Hello from Receiver")
    time = MSG.getClock
    MSG::info("Try to get a task")
    host = MSG::Host.getHostProcess(self)
    task = PingPongTask.receive(host.name)
    p "task name recevied : "+ task.name
    p "data in the task :" + task.data
    timeGot = MSG.getClock
    timeSent= task.data
    MSG::info("Got at time: "+timeGot.to_s)
    MSG::info("Was sent at time "+timeSent.to_s)
    communicationTime = timeGot - time
    MSG::info("Communication Time: "+communicationTime.to_s)
    MSG::info("--- bw "+(100000000/communicationTime).to_s+" ----")
    MSG::info("GoodBye !!!")
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
             