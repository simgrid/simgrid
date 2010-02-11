require 'msg'
require 'RubyProcess'
require 'Master'
require 'Slave'

include MSG

# raise "Bad Number Of Arguments" if ARGV.length != 2 

# info("Bye")

init(ARGV)
raise "Bad Number Of Arguments " if (ARGV.length < 2)
# p  Host.number()
createEnvironment(ARGV[0])
# p  Host.number()
deployApplication(ARGV[1])
# p  Host.number()

 
# run()
# exit()
