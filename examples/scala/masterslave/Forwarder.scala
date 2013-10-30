/*
 * Copyright (c) 2006-2013. The SimGrid Team.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

package masterslave

import Stream._
import org.simgrid.msg.Host
import org.simgrid.msg.Msg
import org.simgrid.msg.MsgException
import org.simgrid.msg.Task
import org.simgrid.msg.Process

class Forwarder(host:Host, name:String, args:Array[String]) extends Process(host,name,args) {

   def main(args: Array[String]){
      if (args.length < 3) {	 
	 Msg.info("Forwarder needs 3 arguments (input mailbox, first output mailbox, last one)")
	 Msg.info("Got "+args.length+" instead")
	 System.exit(1)
      }
      val input = args(0).toInt		
      val firstOutput = args(1).toInt		
      val lastOutput = args(2).toInt		
      
      var taskCount = 0
      val slavesCount = lastOutput - firstOutput + 1
      Msg.info("Receiving on 'slave_"+input+"'")
      var cont = true
      
      continually({Task.receive("slave_"+input)})
        .takeWhile(!_.isInstanceOf[FinalizeTask])
        .foreach(task => {
          val dest = firstOutput + (taskCount % slavesCount)
          Msg.info("Sending \"" + task.getName() + "\" to \"slave_" + dest + "\"")
          task.send("slave_"+dest)
          taskCount += 1
        })

      Msg.info("Got a finalize task. Let's forward that we're done.")
      for (cpt <- firstOutput to lastOutput) {
        val tf = new FinalizeTask()
        tf.send("slave_"+cpt)
      }
	 
      Msg.info("I'm done. See you!")
   }
}

