require 'msg'
require 'RubyProcess'

require 'Master'
require 'Slave'

Msg.init()
Msg.createEnvironment("../../../examples/msg/msg_platform.xml")
Msg.deployApplication("MasterSlave_deploy.xml")
Msg.run()
Msg.exit()
