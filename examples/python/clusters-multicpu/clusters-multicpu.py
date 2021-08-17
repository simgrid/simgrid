# Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# This example shows how to build a torus cluster with multi-core hosts.
#
# However, each leaf in the torus is a StarZone, composed of several CPUs
#
# Each actor runs in a specific CPU. One sender broadcasts a message to all receivers.

import simgrid
import sys
import typing


class Sender:
    """
    Send a msg for each host in its host list
    """

    def __init__(self, hosts, msg_size=int(1e6)):
        self.hosts = hosts
        self.msg_size = msg_size

    # Actors that are created as object will execute their __call__ method.
    # So, the following constitutes the main function of the Sender actor.
    def __call__(self):
        pending_comms = []
        mboxes = []

        for host in self.hosts:
            msg = "Hello, I'm alive and running on " + simgrid.this_actor.get_host().name
            mbox = simgrid.Mailbox.by_name(host.name)
            mboxes.append(mbox)
            pending_comms.append(mbox.put_async(msg, self.msg_size))

        simgrid.this_actor.info("Done dispatching all messages")

        # Now that all message exchanges were initiated, wait for their completion in one single call
        simgrid.Comm.wait_all(pending_comms)

        simgrid.this_actor.info("Goodbye now!")


class Receiver:
    """
    Receiver actor: wait for 1 message on the mailbox identified by the hostname
    """

    def __call__(self):
        mbox = simgrid.Mailbox.by_name(simgrid.this_actor.get_host().name)
        received = mbox.get()
        simgrid.this_actor.info("I got a '%s'." % received)

#####################################################################################################


def create_hostzone(zone: simgrid.NetZone, coord: typing.List[int], ident: int) -> typing.Tuple[simgrid.NetPoint, simgrid.NetPoint]:
    """
    Callback to set a cluster leaf/element

    In our example, each leaf if a StarZone, composed of 8 CPUs.
    Each CPU is modeled as a host, connected to the outer world through a high-speed PCI link.
    Obs.: CPU0 is the gateway for this zone

       (outer world)
            CPU0 (gateway)
       up ->|   |
            |   |<-down
            +star+
         /   / \   \
        /   /   \   \<-- 100Gbs, 10us link (1 link UP and 1 link DOWN for full-duplex)
       /   /     \   \
      /   /       \   \
      CPU1   ...   CPU8

    :param zone: Cluster netzone being created (usefull to create the hosts/links inside it)
    :param coord: Coordinates in the cluster
    :param ident: Internal identifier in the torus (for information)
    :return netpoint, gateway: the netpoint to the StarZone and CPU0 as gateway
    """
    num_cpus = 8     # Number of CPUs in the zone
    speed = "1Gf"    # Speed of each CPU
    link_bw = "100GBps"  # Link bw connecting the CPU
    link_lat = "1ns"  # Link latency

    hostname = "host" + str(ident)
    # create the StarZone
    host_zone = simgrid.NetZone.create_star_zone(hostname)
    # setting my Torus parent zone
    host_zone.set_parent(zone)

    gateway = None
    # create CPUs
    for i in range(num_cpus):
        cpu_name = hostname + "-cpu" + str(i)
        host = host_zone.create_host(cpu_name, speed).seal()
        # the first CPU is the gateway
        if i == 0:
            gateway = host
        # create split-duplex link
        link = host_zone.create_split_duplex_link("link-" + cpu_name, link_bw)
        link.set_latency(link_lat).seal()
        # connecting CPU to outer world
        host_zone.add_route(host.get_netpoint(), None, None, None, [
                            simgrid.LinkInRoute(link, simgrid.LinkInRoute.Direction.UP)], True)

    # seal newly created netzone
    host_zone.seal()
    return host_zone.get_netpoint(), gateway.get_netpoint()

#####################################################################################################


def create_limiter(zone: simgrid.NetZone, coord: typing.List[int], ident: int) -> simgrid.Link:
    """
    Callback to create limiter link (1Gbs) for each netpoint

    The coord parameter depends on the cluster being created:
    - Torus: Direct translation of the Torus' dimensions, e.g. (0, 0, 0) for a 3-D Torus
    - Fat-Tree: A pair (level in the tree, ident), e.g. (0, 0) for first leaf in the tree and (1,0) for the first switch at
    level 1.
    - Dragonfly: a tuple (group, chassis, blades/routers, nodes), e.g. (0, 0, 0, 0) for first node in the cluster. To
    identify the router inside a (group, chassis, blade), we use MAX_UINT in the last parameter (e.g. 0, 0, 0,
    4294967295).

    :param zone: Torus netzone being created (usefull to create the hosts/links inside it)
    :param coord: Coordinates in the cluster
    :param ident: Internal identifier in the torus (for information)
    :return: Limiter link
    """
    return zone.create_link("limiter-" + str(ident), [1e9]).seal()


def create_torus_cluster():
    """
    Creates a TORUS cluster

    Creates a TORUS cluster with dimensions 2x2x2

    The cluster has 8 elements/leaves in total. Each element is a StarZone containing 8 Hosts.
    Each pair in the torus is connected through 2 links:
    1) limiter: a 1Gbs limiter link (set by user through the set_limiter callback)
    2) link: 10Gbs link connecting the components (created automatically)

    (Y-axis=2)
    A
    |
    |   D (Z-axis=2)
    +  / 10 Gbs
    | +
    |/ limiter=1Gps
    B-----+----C (X-axis=2)

    For example, a communication from A to C goes through:
    <tt> A->limiter(A)->link(A-B)->limiter(B)->link(B-C)->limiter(C)->C </tt>

    More precisely, considering that A and C are StarZones, a
    communication from A-CPU-3 to C-CPU-7 goes through:
    1) StarZone A: A-CPU-3 -> link-up-A-CPU-3 -> A-CPU-0
    2) A-CPU-0->limiter(A)->link(A-B)->limiter(B)->link(B-C)->limiter(C)->C-CPU-0
    3) StarZone C: C-CPU-0-> link-down-C-CPU-7 -> C-CPU-7

    Note that we don't have limiter links inside the StarZones(A, B, C),
    but we have limiters in the Torus that are added to the links in the path (as we can see in "2)")

    More details in: <a href="https://simgrid.org/doc/latest/Platform_examples.html?highlight=torus#torus-cluster">Torus
    Cluster</a>
    """
    # create the torus cluster, 10Gbs link between elements in the cluster
    simgrid.NetZone.create_torus_zone("cluster", None, [2, 2, 2], simgrid.ClusterCallbacks(create_hostzone, None, create_limiter), 10e9, 10e-6,
                                      simgrid.Link.SharingPolicy.SPLITDUPLEX).seal()

#####################################################################################################


def create_fat_tree_cluster():
    """
    Creates a Fat-Tree cluster

    Creates a Fat-Tree cluster with 2 levels and 6 nodes
    The following parameters are used to create this cluster:
    - Levels: 2 - two-level of switches in the cluster
    - Down links: 2, 3 - L2 routers is connected to 2 elements, L1 routers to 3 elements
    - Up links: 1, 2 - Each node (A-F) is connected to 1 L1 router, L1 routers are connected to 2 L2
    - Link count: 1, 1 - Use 1 link in each level

    The first parameter describes how many levels we have.
    The following ones describe the connection between the elements and must have exactly n_levels components.


                            S3     S4                <-- Level 2 routers
       link:limiter -      /   \  /  \
                          +     ++    +
       link: 10GBps -->  |     /  \    |
        (full-duplex)    |    /    \   |
                         +   +      +  +
                         |  /        \ |
                         S1           S2             <-- Level 1 routers
      link:limiter ->    |             |
                         +             +
     link:10GBps  -->   /|\           /|\
                       / | \         / | \
                      +  +  +       +  +  +
     link:limiter -> /   |   \     /   |   \
                    A    B    C   D    E    F        <-- level 0 Nodes

    Each element (A to F) is a StarZone containing 8 Hosts.
    The connection uses 2 links:
    1) limiter: a 1Gbs limiter link (set by user through the set_limiter callback)
    2) link: 10Gbs link connecting the components (created automatically)

    For example, a communication from A to C goes through:
    <tt> A->limiter(A)->link(A-S1)->limiter(S1)->link(S1-C)->->limiter(C)->C</tt>

    More precisely, considering that A and C are StarZones, a
    communication from A-CPU-3 to C-CPU-7 goes through:
    1) StarZone A: A-CPU-3 -> link-up-A-CPU-3 -> A-CPU-0
    2) A-CPU-0->limiter(A)->link(A-S1)->limiter(S1)->link(S1-C)->limiter(C)->C-CPU-0
    3) StarZone C: C-CPU-0-> link-down-C-CPU-7 -> C-CPU-7

    More details in: <a href="https://simgrid.org/doc/latest/Platform_examples.html#fat-tree-cluster">Fat-Tree
    Cluster</a>
    """
    # create the fat tree cluster, 10Gbs link between elements in the cluster
    simgrid.NetZone.create_fatTree_zone("cluster", None, simgrid.FatTreeParams(2, [2, 3], [1, 2], [1, 1]), simgrid.ClusterCallbacks(create_hostzone, None, create_limiter), 10e9,
                                        10e-6, simgrid.Link.SharingPolicy.SPLITDUPLEX).seal()

#####################################################################################################


def create_dragonfly_cluster():
    """
    Creates a Dragonfly cluster

    Creates a Dragonfly cluster with 2 groups and 16 nodes
    The following parameters are used to create this cluster:
    - Groups: 2 groups, connected with 2 links (blue links)
    - Chassis: 2 chassis, connected with a single link (black links)
    - Routers: 2 routers, connected with 2 links (green links)
    - Nodes: 2 leaves per router, single link

    The diagram below illustrates a group in the dragonfly cluster

    +------------------------------------------------+
    |        black link(1)                           |
    |     +------------------------+                 |
    | +---|--------------+     +---|--------------+  |
    | |   |  green       |     |   |  green       |  |
    | |   |  links (2)   |     |   |  links (2)   |  |   blue links(2)
    | |   R1 ====== R2   |     |   R3 -----  R4 ======================> "Group 2"
    | |  /  \      /  \  |     |  /  \      /  \  |  |
    | | A    B    C    D |     | E    F    G    H |  |
    | +------------------+     +------------------+  |
    |      Chassis 1                Chassis 2        |
    +------------------------------------------------+
     Group 1

    Each element (A, B, C, etc) is a StarZone containing 8 Hosts.
    The connection between elements (e.g. A->R1) uses 2 links:
    1) limiter: a 1Gbs limiter link (set by user through the set_limiter callback)
    2) link: 10Gbs link connecting the components (created automatically)

    For example, a communication from A to C goes through:
    <tt> A->limiter(A)->link(A-R1)->limiter(R1)->link(R1-R2)->limiter(R2)->link(R2-C)limiter(C)->C</tt>

    More details in: <a href="https://simgrid.org/doc/latest/Platform_examples.html#dragonfly-cluster">Dragonfly
    Cluster</a>
    """
    # create the dragonfly cluster, 10Gbs link between elements in the cluster
    simgrid.NetZone.create_dragonfly_zone("cluster", None, simgrid.DragonflyParams([2, 2], [2, 1], [2, 2], 2), simgrid.ClusterCallbacks(
        create_hostzone, None, create_limiter), 10e9, 10e-6, simgrid.Link.SharingPolicy.SPLITDUPLEX).seal()

###################################################################################################


if __name__ == '__main__':
    e = simgrid.Engine(sys.argv)
    platform = sys.argv[1]

    # create platform
    if platform == "torus":
        create_torus_cluster()
    elif platform == "fatTree":
        create_fat_tree_cluster()
    elif platform == "dragonfly":
        create_dragonfly_cluster()
    else:
        sys.exit("invalid param")

    host_list = e.get_all_hosts()
    # create the sender actor running on first host
    simgrid.Actor.create("sender", host_list[0], Sender(host_list))
    # create receiver in every host
    for host in host_list:
        simgrid.Actor.create("receiver-" + host.name, host, Receiver())

    # runs the simulation
    e.run()
