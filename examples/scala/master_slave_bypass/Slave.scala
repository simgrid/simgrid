/*
 * Copyright (c) 2006-2013. The SimGrid Team.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */
package master_slave_bypass

import org.simgrid.msg.HostFailureException
import org.simgrid.msg.HostNotFoundException
import org.simgrid.msg.Msg
import org.simgrid.msg.TimeoutException
import org.simgrid.msg.TransferFailureException
import org.simgrid.msg.Process

class Slave(hostname:String, name:String) extends Process(hostname, name) {
  def main(args:Array[String]) {
    Msg.info("Slave Hello!")
    val task = new FinalizeTask()
    Msg.info("Send finalize!")
    task.send("alice")
  }
}
