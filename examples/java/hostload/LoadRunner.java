/* Copyright (c) 2016-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package hostload;

import org.simgrid.msg.*;
import org.simgrid.msg.Process;


public class LoadRunner extends Process {

    public LoadRunner(Host host, String s) {
    super(host, s);
}

public void display(){
    Msg.info("Computed Flops "+ 		getHost().getComputedFlops());
    Msg.info("Current Load "+ 		getHost().getCurrentLoad());
    Msg.info("GetLoad "+ 		getHost().getLoad());
    Msg.info("AvgLoad "+ 		getHost().getAvgLoad());
}
@Override
public void main(String[] strings) throws MsgException {
    double workload = 100E6;
    Host host = getHost();
    display();
    // Run a task
    Task task1 = new Task("t1", workload, 0);
    task1.execute();
    display();
    double taskTime = Msg.getClock();
    Msg.info("Task1 simulation time: "+ taskTime);

    // Run a second task
    new Task("t1", workload, 0).execute();

    taskTime = Msg.getClock() - taskTime;
    Msg.info("Task2 simulation time: "+ taskTime);
    display();

}


}

