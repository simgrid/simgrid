require "simgrid"

  --Set Application
   simgrid.gras_set_process_function("Fafard","client",{"Tremblay","4000"});
   simgrid.gras_set_process_function("Tremblay","server",{"4000"});
  --Save Application 
   simgrid.gras_generate("ping"); 


