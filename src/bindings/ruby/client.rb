require 'msg'
include MSG


array = Array.new()

# puts array.empty?
array << "Peace!!" << "Hey" << "Euh..."<< "Hey2" << "Euh...2"


# array.clear
# puts array.empty?

mehdi = "Hey...this is my name"

hash = Hash.new


var = "name"

hash[var] = mehdi


# puts hash["name"]

array.push(mehdi)


#  info("Hey...")
# puts array[0]

array2 = array

# puts array2[0]

size = array2.size






task = Task.new('ninou',12,23)
puts "Created task :"+task.to_s+" ; name: "+Task.name(task)
#task2 =RbTask.new('task2')
# task = RbTask.new()
# host = Host.new("Brad")

 
 #host2 = Host.new()
#  task_2 = Task.new('task2',12,23)
#   name1 = task_2.name()
#  name2 = Task.name(task)
#  size = Task.compSize(task)
 name = Task.name(task)
# task.bind
number = 56

process = RbProcess.new("mon nom")
puts "Created process :"+process.to_s+" ; name: "+RbProcess.name(task)
# name = process.name
puts name
# puts process.alive?
# Thread.stop
# process2 = RbProcess.new()
# process2.name = "Hope"

# reader = RubyXML.new();
# reader.parseApplication("application.xml")
# reader.printAll()



# name2 = Task.test()
# puts name
# process2 = RbProcess.new()



 #  puts (name)
# init(array)
# createEnvironment(name);
# Task.goodbye


