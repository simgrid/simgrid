-- Copyright (c) 2011-2021. The SimGrid Team.
-- All rights reserved.

-- This program is free software; you can redistribute it and/or modify it
-- under the terms of the license (GNU LGPL) which comes with this package.

  require("simgrid")

  function seq(min,max)
    L={}
    for i=min,max,1 do
      table.insert(L,i)
    end
    return L
  end

  function my_cluster(args)
    -- args is a table with the following keys
    --   -
    local required_args = {"id", "prefix", "suffix", "radical", "speed", "bw", "lat" }
    for _,val in pairs(required_args) do
      if args[val] == nil then simgrid.critical("Must specify '" .. val .. "' attribute. See docs for details.") end
    end
    if args.sharing_sharing_policy == nil then
        args.sharing_sharing_policy = "SHARED"
    end
    if args.topology ~= "TORUS" and args.topology ~= "FAT_TREE" then
        args.topology = "Cluster"
    end

    -- Check the mode = Cluster here
    return function()

        simgrid.engine.AS_open{id=args.id,mode=args.topology};

        if args.bb_bw ~= nil and args.bb_lat ~= nil then
          simgrid.engine.backbone_new{id=args.id .. "-bb",bandwidth=args.bb_bw,latency=args.bb_lat,sharing_policy=args.bb_sharing_sharing_policy}
        end
        for _,i in pairs(args.radical) do
            local hostname = args.prefix .. i .. args.suffix
            local linkname = args.id .."_link_" .. i
            simgrid.engine.host_new{id=hostname, speed=args.speed,core=args.core,power_trace=args.availability_file,state_trace=args.state_file};
            simgrid.engine.link_new{id=linkname, bandwidth=args.bw,latency=args.lat, sharing_policy=args.sharing_sharing_policy};
            simgrid.engine.host_link_new{id=hostname,up=linkname.."_UP",down=linkname.."_DOWN"};

            if args.loopback_bw ~= nil and args.loopback_lat ~= nil then
              simgrid.engine.link_new{id=linkname .. "_loopback",bandwidth=args.loopback_bw,latency=args.loopback_lat,sharing_policy="FATPIPE"}
            end
        end
        simgrid.engine.AS_seal()
      end
  end

  simgrid.engine.open();
  cluster_factory = my_cluster{prefix="node-", suffix=".simgrid.org", radical=seq(0,262144), host_factory = function(hostno)
      if hostno % 2 == 0 then return "blabla" end
      if hostno % 2 == 1 then return "blublub" end
    end,
    speed="1Gf",
    id="AS0",
    bw="125MBps",
    lat="50us",
    sharing_sharing_policy="SPLITDUPLEX"
  }()
  --my_cluster{prefix="node2-", suffix=".simgrid.org", radical=seq(0,44) }

  simgrid.engine.close();
