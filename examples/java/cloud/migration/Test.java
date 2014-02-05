/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package cloud.migration;

import org.simgrid.msg.*;
import org.simgrid.msg.Process;

public class Test extends Process{

    Test(Host host, String name, String[] args) throws HostNotFoundException, NativeException  {
        super(host, name, args);
    }

    public void main(String[] strings) throws MsgException {

       double startTime = 0;
       double endTime = 0;

       /* get hosts 1 and 2*/
        Host host0 = null;
        Host host1 = null;

        try {
            host0 = Host.getByName("host0");
            host1 = Host.getByName("host1");
        }catch (HostNotFoundException e) {
            e.printStackTrace();  //To change body of catch statement use File | Settings | File Templates.
        }

      /* Create VM1 */
        int dpRate = 70;
        int load1 = 90;
        int load2 = 80;

        XVM vm1 = null;
        vm1 = new XVM(
                host0,
                "vm0",
                1, // Nb of vcpu
                2048, // Ramsize,
                125, // Net Bandwidth
                null, //VM disk image
                -1,   //size of disk image,
                125, // Net bandwidth,
                dpRate // Memory intensity
        );


        vm1.start();

        /* Collocated VMs */
        int collocatedSrc = 6;
        int vmSrcLoad[] = {
                80,
                0,
                90,
                40,
                30,
                90,
        };

        XVM tmp = null;
        for (int i=1 ; i<= collocatedSrc ; i++){
            tmp = new XVM(
                    host0,
                    "vm"+i,
                    1, // Nb of vcpu
                    2048, // Ramsize,
                    125, // Net Bandwidth
                    null, //VM disk image
                    -1,   //size of disk image,
                    125, // Net bandwidth,
                    dpRate // Memory intensity
            );
            tmp.start();
            tmp.setLoad(vmSrcLoad[i-1]);
        }

        int collocatedDst = 6;
        int vmDstLoad[] = {
                0,
                40,
                90,
                100,
                0,
                80,
        };

        for (int i=1 ; i <= collocatedDst ; i++){
            tmp = new XVM(
                    host1,
                    "vm"+i+collocatedSrc,
                    1, // Nb of vcpu
                    2048, // Ramsize,
                    125, // Net Bandwidth
                    null, //VM disk image
                    -1,   //size of disk image,
                    125, // Net bandwidth,
                    dpRate // Memory intensity
            );
            tmp.start();
            tmp.setLoad(vmDstLoad[i-1]);
        }





        Msg.info("Round trip of VM1 (load "+load1+"%)");
        vm1.setLoad(load1);
        Msg.info("     - Launch migration from host 0 to host 1");
        startTime = Msg.getClock();
        vm1.migrate(host1);
        endTime = Msg.getClock();
        Msg.info("     - End of Migration from host 0 to host 1 (duration:"+(endTime-startTime)+")");
        Msg.info("     - Launch migration from host 1 to host 0");
        startTime = Msg.getClock();
        vm1.migrate(host0);
        endTime = Msg.getClock();
        Msg.info("     - End of Migration from host 1 to host 0 (duration:"+(endTime-startTime)+")");

        Msg.info("\n \n \nRound trip of VM1 (load "+load2+"%)");
        vm1.setLoad(load2);
        Msg.info("     - Launch migration from host 0 to host 1");
        startTime = Msg.getClock();
        vm1.migrate(host1);
        endTime = Msg.getClock();
        Msg.info("     - End of Migration from host 0 to host 1 (duration:"+(endTime-startTime)+")");
        Msg.info("     - Launch migration from host 1 to host 0");
        startTime = Msg.getClock();
        vm1.migrate(host0);
        endTime = Msg.getClock();
        Msg.info("     - End of Migration from host 1 to host 0 (duration:"+(endTime-startTime)+")");

        waitFor(100000);
        Main.setEndOfTest();
    }
}
