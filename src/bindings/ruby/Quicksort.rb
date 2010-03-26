# Debug it with this command:
# make -C ../.. && valgrind ruby MasterSlave.rb --log=ruby.thres:debug 2>&1 | less

require 'simgrid'

include MSG

#################################################
# Class Asker
#################################################

class Sender < MSG::Process  
  # main : that function that will be executed when running simulation
  def main(args) # args is an array containing arguments for function master
  
   hardworking_friend = MSG::Host.getByName(args[0]).name
   taskComputeSize = Float(args[1])
   taskCommunicationSize = Float(args[2])
   send_mailbox =args[3]
   
   myTable = Array.new
   myTable <<1<<-2<<45<<67<<87<<76<<89<<56<<78<<3<<-4<<99
   MSG::info("Hello " + hardworking_friend + " !!, Please !! I need you to help me to sort my table , Here it is :")
   p myTable
   # Creates and send Task With the Table inside
   task = MSG::Task.new("quicksort_task",taskComputeSize, taskCommunicationSize);
   task.setData(myTable);
   MSG::debug("Sending "+ task.name + " to " + send_mailbox + " with Comput Size " + 
           task.compSize.to_s)
   task.send(send_mailbox)
   MSG::debug("Done Sending " + task.name + " to " + send_mailbox)
    
   #waiting for results
   
   recv_mailbox = self.class
   res_task = MSG::Task.receive(recv_mailbox.to_s)
   result = res_task.data
   MSG::info("Greate !! Thx Dude , you're my Best Friend !!")
   MSG::info("Here is my table after a quicksort :)")
   p result
   MSG::info("Bye !! I finished My HomeWork !! Time to Sleep :)")
   end

end


#################################################
# Class Clever
#################################################
class Receiver < MSG::Process

  def main(args)
    
    lazy_friend = MSG::Host.getByName(args[0]).name
    send_mailbox = args[1]
    recv_mailbox = self.class
    
    MSG::info("Oh Not Again !! Grrr")
    task = MSG::Task.receive(recv_mailbox.to_s)
    table = task.data
    quicksort(table,0,table.size-1)
    task.setData(table)
    MSG::info("Ok "+lazy_friend+ "I did it, next time try to do it on your own :)")
    task.send(send_mailbox)
    MSG::info("Bye lazy Friend !!")
    
  end    
  
  def quicksort(list, p, r)
    if p < r then
        q = partition(list, p, r)
        quicksort(list, p, q-1)
        quicksort(list, q+1, r)
    end
end

def partition(list, p, r)
    pivot = list[r]
    i = p - 1
    p.upto(r-1) do |j|
        if list[j] <= pivot
            i = i+1
            list[i], list[j] = list[j],list[i]
        end
    end
    list[i+1],list[r] = list[r],list[i+1]
    return i + 1
end
  
 
end

#################################################
# main chunck
#################################################

if (ARGV.length == 2) 
	MSG.createEnvironment(ARGV[0])
	MSG.deployApplication(ARGV[1])
else 
	MSG.createEnvironment("quicksort_platform.xml")
	MSG.deployApplication("quicksort_deployment.xml")
end

MSG.run
puts "Simulation time : " + MSG.getClock .to_s
MSG.exit
