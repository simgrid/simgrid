/* Copyright (c) 2016-2022. The SimGrid Team. All rights reserved.          */

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
        Msg.info("Speed="+getHost().getSpeed()+" flop/s");
        Msg.info("Computed Flops "+ 		getHost().getComputedFlops());
        Msg.info("AvgLoad "+ 		getHost().getAvgLoad());
    }
    @Override
    public void main(String[] strings) throws MsgException {
        display();
        Msg.info("Sleep for 10 seconds");
        waitFor(10);
        display();

        // Run a task
        Task task1 = new Task("t1", 200E6, 0);
        task1.execute();
        display();
        double taskTime = Msg.getClock();
        Msg.info("Task1 simulation time: "+ taskTime);

        // Run a second task
        new Task("t1", 200E6, 0).execute();

        taskTime = Msg.getClock() - taskTime;
        Msg.info("Task2 simulation time: "+ taskTime);
        display();

    }


}