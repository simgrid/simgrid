/* Copyright (c) 2007-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/plugins/energy.h"
#include "simgrid/plugins/carbon_footprint.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;


void turn_host_off(simgrid::s4u::Host * host)
{
  host->turn_off();
}
void update_co2_actor(simgrid::s4u::Host* host, std::map<int, double > co2_data)
{  
  for (auto it = co2_data.begin(); it != co2_data.end(); it++)
  {
    int time = it->first;
    double co2_data = it->second;
    simgrid::s4u::this_actor::sleep_until(time);
    sg_host_set_carbon_intensity(host,co2_data);    
  }      
}

void test_execution()
{          
    sg4::Host* host = sg4::this_actor::get_host();
    
    double host_speed_in_flops = host->get_speed();

    // Run one task (total 1 CPU core) for 1 hour    
    double flopAmount_1_hour = host_speed_in_flops * 3600 * 1;    
    
    sg4::ExecPtr activity = sg4::this_actor::exec_init(flopAmount_1_hour);
    activity->start();
    activity->wait();

    // Run two tasks (total of 2 CPU cores) for 2 hours
    sg4::ExecPtr activity_2_1 = sg4::this_actor::exec_init(flopAmount_1_hour*2);        
    sg4::ExecPtr activity_2_2 = sg4::this_actor::exec_init(flopAmount_1_hour*2);
    
    activity_2_1->start();
    activity_2_2->start();

    activity_2_1->wait();
    activity_2_2->wait();
  
    // Run three tasks (total of 3 CPU cores) for 3 hours
    sg4::ExecPtr activity_3_1 = sg4::this_actor::exec_init(flopAmount_1_hour*3);        
    sg4::ExecPtr activity_3_2 = sg4::this_actor::exec_init(flopAmount_1_hour*3);
    sg4::ExecPtr activity_3_3 = sg4::this_actor::exec_init(flopAmount_1_hour*3);
    
    activity_3_1->start();
    activity_3_2->start();
    activity_3_3->start();

    activity_3_1->wait();
    activity_3_2->wait();
    activity_3_3->wait();

    // Run four tasks (total of 4 CPU cores) for 4 hours
    sg4::ExecPtr activity_4_1 = sg4::this_actor::exec_init(flopAmount_1_hour*4);        
    sg4::ExecPtr activity_4_2 = sg4::this_actor::exec_init(flopAmount_1_hour*4);
    sg4::ExecPtr activity_4_3 = sg4::this_actor::exec_init(flopAmount_1_hour*4);        
    sg4::ExecPtr activity_4_4 = sg4::this_actor::exec_init(flopAmount_1_hour*4);

    activity_4_1->start();
    activity_4_2->start();
    activity_4_3->start();
    activity_4_4->start();

    activity_4_1->wait();
    activity_4_2->wait();
    activity_4_3->wait();
    activity_4_4->wait();

    
 
}

int main(int argc, char* argv[])
{


  // Dict values for grid CO2 of each scenario, key is the time, and value is the carbon emissions (in g CO2/kWh)
  std::map<int, double > brazil_grid_co2;
  std::map<int, double> france_grid_co2;
  std::map<int, double > usa_grid_co2;

  int seconds_in_hour = 3600;

  // Grid electricity emissions values

  // For Brazil    
  brazil_grid_co2[0] = 100.07;  
  brazil_grid_co2[seconds_in_hour*1] = 93.6;
  brazil_grid_co2[seconds_in_hour*2] = 93.89;
  brazil_grid_co2[seconds_in_hour*3] = 96.04;
  brazil_grid_co2[seconds_in_hour*4] = 95.0;
  brazil_grid_co2[seconds_in_hour*5] = 94.4;
  brazil_grid_co2[seconds_in_hour*6] = 94.11;
  brazil_grid_co2[seconds_in_hour*7] = 94.99;
  brazil_grid_co2[seconds_in_hour*8] = 96.44;
  brazil_grid_co2[seconds_in_hour*9] = 99.76;


  // For France
  france_grid_co2[0] = 29.09;
  france_grid_co2[seconds_in_hour*1] = 30.08;
  france_grid_co2[seconds_in_hour*2] = 32.33;
  france_grid_co2[seconds_in_hour*3] = 32.96;
  france_grid_co2[seconds_in_hour*4] = 33.0;
  france_grid_co2[seconds_in_hour*5] = 33.41;
  france_grid_co2[seconds_in_hour*6] = 34.52;
  france_grid_co2[seconds_in_hour*7] = 33.63;
  france_grid_co2[seconds_in_hour*8] = 32.17;
  france_grid_co2[seconds_in_hour*9] = 31.95;

  // For USA
  usa_grid_co2[0] = 453.54;
  usa_grid_co2[seconds_in_hour*1] = 441.48;
  usa_grid_co2[seconds_in_hour*2] = 437.93;
  usa_grid_co2[seconds_in_hour*3] = 437.61;
  usa_grid_co2[seconds_in_hour*4] = 442.29;
  usa_grid_co2[seconds_in_hour*5] = 447.18;
  usa_grid_co2[seconds_in_hour*6] = 452.04;
  usa_grid_co2[seconds_in_hour*7] = 453.96;
  usa_grid_co2[seconds_in_hour*8] = 455.13;
  usa_grid_co2[seconds_in_hour*9] = 457.54;

  sg_host_energy_plugin_init();              
  sg_host_carbon_footprint_plugin_init();

  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  
  sg4::Host* host_br_static_co2 = sg4::Host::by_name("host_br_static_co2");
  sg4::Host* host_fr_static_co2 = sg4::Host::by_name("host_fr_static_co2");
  sg4::Host* host_usa_static_co2 = sg4::Host::by_name("host_usa_static_co2"); 

  sg4::Host* host_br_dynamic_co2 = sg4::Host::by_name("host_br_dynamic_co2");
  sg4::Host* host_fr_dynamic_co2 = sg4::Host::by_name("host_fr_dynamic_co2");
  sg4::Host* host_usa_dynamic_co2 = sg4::Host::by_name("host_usa_dynamic_co2");


  sg4::Actor::create("turn_off test", host_br_static_co2, turn_host_off,host_usa_static_co2);
  sg4::Actor::create("turn_off test", host_br_static_co2, turn_host_off,host_usa_dynamic_co2);
       
  sg4::Actor::create("execution", host_br_static_co2, test_execution);
  sg4::Actor::create("execution", host_br_dynamic_co2, test_execution);

  sg4::Actor::create("update_co2", host_br_static_co2, update_co2_actor,host_usa_dynamic_co2,usa_grid_co2);
  sg4::Actor::create("update_co2", host_br_static_co2, update_co2_actor,host_fr_dynamic_co2,france_grid_co2);
  sg4::Actor::create("update_co2", host_br_static_co2, update_co2_actor,host_br_dynamic_co2,brazil_grid_co2);

  e.run();


  return 0;

}
