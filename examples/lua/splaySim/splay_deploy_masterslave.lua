
   dofile "master.lua"
   dofile "slave.lua"
   --Set Application
   simgrid.host.set_function{host="Splayd_1",fct="Master",args="20,550000000,1000000,4"};
   simgrid.host.set_function{host="Splayd_5",fct="Slave",args="0"};
   simgrid.host.set_function{host="Splayd_2",fct="Slave",args="1"};
   simgrid.host.set_function{host="Splayd_3",fct="Slave",args="2"};
   simgrid.host.set_function{host="Splayd_4",fct="Slave",args="3"};

  --Save Application 
   simgrid.msg_register_application(); 
