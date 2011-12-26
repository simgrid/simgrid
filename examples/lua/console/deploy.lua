 dofile 'master.lua'
 dofile 'slave.lua' 
  --Set Application
   simgrid.host.set_function{host="Tremblay",fct="Master",args="20,550000000,1000000,4"};
   simgrid.host.set_function{host="Bourassa",fct="Slave",args="0"};
   simgrid.host.set_function{host="Jupiter",fct="Slave",args="1"};
   simgrid.host.set_function{host="Fafard",fct="Slave",args="2"};
   simgrid.host.set_function{host="Ginette",fct="Slave",args="3"};
   
  --Save Application 
   simgrid.msg_register_application();
