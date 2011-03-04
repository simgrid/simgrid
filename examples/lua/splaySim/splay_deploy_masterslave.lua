
   dofile "master.lua"
   dofile "slave.lua"
   --Set Application
   simgrid.Host.setFunction{host="Splayd_1",fct="Master",args="20,550000000,1000000,4"};
   simgrid.Host.setFunction{host="Splayd_5",fct="Slave",args="0"};
   simgrid.Host.setFunction{host="Splayd_2",fct="Slave",args="1"};
   simgrid.Host.setFunction{host="Splayd_3",fct="Slave",args="2"};
   simgrid.Host.setFunction{host="Splayd_4",fct="Slave",args="3"};
   
   simgrid.info("Start Saving Platform...");
  --Save Application 
   simgrid.msg_register_application(); 
   simgrid.info("Platform Saved...");
