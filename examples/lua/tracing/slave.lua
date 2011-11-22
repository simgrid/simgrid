
-- Slave Function ---------------------------------------------------------
function Slave(...)

local my_mailbox="slave "..arg[1]
simgrid.info("Hello from lua, I'm a poor slave with mbox: "..my_mailbox)

while true do

  local tk = simgrid.task.recv(my_mailbox);
  if (simgrid.task.get_name(tk) == "finalize") then
    simgrid.info("Slave '" ..my_mailbox.."' got finalize msg");
    break
  end
  --local tk_name = simgrid.task.get_name(tk) 
  simgrid.info("Slave '" ..my_mailbox.."' processing "..simgrid.task.get_name(tk))
  simgrid.task.execute(tk)
  simgrid.info("Slave '" ..my_mailbox.."': task "..simgrid.task.get_name(tk) .. " done")
end -- while

simgrid.info("Slave '" ..my_mailbox.."': I'm Done . See You !!");

end

