require 'simdag'
include DAG

# Still using the old method to run :
#>> make     ( to generate dag.so)
#>> ruby example.rb   ( to run )
# FIXME if using valgrind , you will get Some 'Invalid Read of Size" When Calling rb_task_schedule  '


class Example

  def initialize()
    DAG.init(ARGV)
    
    if (ARGV.length == 1)
      DAG.init(ARGV[0])
    else 
      DAG.createEnvironment("platform.xml")
    end
   
    taskA = DAG::Task.new("Tache A",10)
    taskB = DAG::Task.new("Tache B",40)
    taskC = DAG::Task.new("Tache C",30)
    taskD = DAG::Task.new("Tache D",60)
    
#     Adding dependency
    DAG::Task::addDependency(taskB,taskA)
    DAG::Task::addDependency(taskC,taskA)
    DAG::Task::addDependency(taskD,taskB)
    DAG::Task::addDependency(taskD,taskC)

#      Execution Params
    
    workstation_number = 2
    workstation_list = DAG::Workstation.getList()
    w1 = workstation_list[0]
    w2 = workstation_list[1]
    
     p "workstation 1 : "+ w1.name
     p "workstation 2 : "+ w2.name
    
    computation_amount = Array.new()
    communication_amount = Array.new()
    
    computation_amount << 2000000 << 1000000
    communication_amount << 200000 << 300000
    
    rate = -1
    
    
#     Scheduling
    
    DAG::Task.schedule(taskA,workstation_number,workstation_list,computation_amount,communication_amount,rate) 
    DAG::Task.schedule(taskB,workstation_number,workstation_list,computation_amount,communication_amount,rate)
    DAG::Task.schedule(taskC,workstation_number,workstation_list,computation_amount,communication_amount,rate)
    DAG::Task.schedule(taskD,workstation_number,workstation_list,computation_amount,communication_amount,rate)
  
	
    changed_tasks = Array.new()
    changed_tasks = DAG::Task.simulate(-1.0)


    for i in 0..changed_tasks.size-1
      puts "Task "+ changed_tasks[i].name + " Started at " + changed_tasks[i].startTime.to_s + " has been done at "+ changed_tasks[i].finishTime.to_s
      
    end
        
    puts "Everything's Done ... Bye!!"

    end
end

expl = Example.new 