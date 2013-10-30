/*
 * Copyright (c) 2006-2013. The SimGrid Team.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */
package masterslave;

import Stream._
import org.simgrid.msg.Host
import org.simgrid.msg.HostFailureException
import org.simgrid.msg.Msg
import org.simgrid.msg.Task
import org.simgrid.msg.TaskCancelledException
import org.simgrid.msg.TimeoutException
import org.simgrid.msg.TransferFailureException
import org.simgrid.msg.Process

class Slave(host:Host, name:String, args:Array[String]) extends Process(host,name,args) {
  def main(args:Array[String]){
    if (args.length < 1) {
      Msg.info("Slave needs 1 argument (its number)")
      System.exit(1)
    }

    val num = args(0).toInt
    
    continually({Task.receive("slave_"+num)})
      .takeWhile(!_.isInstanceOf[FinalizeTask])
      .foreach(task => {
        Msg.info("Received \"" + task.getName() +  "\". Processing it.");
        try {
          task.execute();
        } catch {
          case e:TaskCancelledException => {}
        }
      })

    Msg.info("Received Finalize. I'm done. See you!")
  }
}
