 dofile 'master.lua'
 dofile 'slave.lua' 
  --Set Application
   simgrid.Host.setFunction{host="Tremblay",fct="Master",args="20,550000000,1000000,4"};
   simgrid.Host.setFunction{host="Bourassa",fct="Slave",args="0"};
   simgrid.Host.setFunction{host="Jupiter",fct="Slave",args="1"};
   simgrid.Host.setFunction{host="Fafard",fct="Slave",args="2"};
   simgrid.Host.setFunction{host="Ginette",fct="Slave",args="3"};
   
  --Save Application 
   simgrid.msg_register_application();
