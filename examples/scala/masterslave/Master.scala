/*
 * Master of a basic master/slave example in Java
 *
 * Copyright (c) 2006-2013. The SimGrid Team.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */
package masterslave;

import org.simgrid.msg.Host;
import org.simgrid.msg.Msg;
import org.simgrid.msg.MsgException;
import org.simgrid.msg.Task;
import org.simgrid.msg.Process;;

class Master(host:Host, name:String, args:Array[String]) extends Process(host,name,args) {
  def main(args:Array[String]) {
    if (args.length < 4) {
      Msg.info("Master needs 4 arguments")
      System.exit(1)
    }

    val tasksCount = args(0).toInt		
    val taskComputeSize = args(1).toDouble		
    val taskCommunicateSize = args(2).toDouble
    val slavesCount = args(3).toInt

    Msg.info("Hello! Got " +  slavesCount + " slaves and " + tasksCount + " tasks to process")

    for (i <- 0 until tasksCount) {
      val task = new Task("Task_" + i, taskComputeSize, taskCommunicateSize)
      task.send("slave_"+(i%slavesCount))
    }

    Msg.info("All tasks have been dispatched. Let's tell everybody the computation is over.")

    for (i <- 0 until slavesCount) {
      val task = new FinalizeTask()
      task.send("slave_"+(i%slavesCount))
    }

    Msg.info("Goodbye now!")
  }
}
