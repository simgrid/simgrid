require 'msg'
require 'RubyProcess'
require 'Master'
require 'Slave'

# include MSG

raise "Bad Number Of Arguments" if ARGV.length != 2 

MSG.init(ARGV)
# Thread.list.each {|t| p t}
raise "Bad Number Of Arguments" if (ARGV.length < 2)
MSG.createEnvironment(ARGV[0])
# Thread.list.each {|t| p t}
MSG.deployApplication(ARGV[1])
# Thread.list.each {|t| p t}
MSG.run()
# Thread.list.each {|t| p t}
MSG.getClock()
# exit()
