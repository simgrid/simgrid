require 'msg'
require 'RubyProcess'
require 'Master'
require 'Slave'

# include MSG

# raise "Bad Number Of Arguments" if ARGV.length != 2 

# info("Bye")

MSG.init(ARGV)
raise "Bad Number Of Arguments" if (ARGV.length < 2)
#  p  Host.number()
MSG.createEnvironment(ARGV[0])
#  p  Host.number()
MSG.deployApplication(ARGV[1])
# p  Host.number()
 MSG.run()
# exit()
