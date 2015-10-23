
hostFactory = function(host_args)
    if type(host_args.power) ~= "number" then
        error("OOPS")
    end
    return function(more_args)
    end
end

