function Receiver(...)

  simgrid.info("Hello From Receiver")
  local sender = simgrid.host.get_by_name(arg[1])
  local send_alias = arg[2]
  local recv_alias = "Receiver"
  simgrid.info("Receiving Task from " .. simgrid.host.name(sender))
  local task = simgrid.task.recv(recv_alias)
  local mm = mmult(task['size'], task['size'], task['matrix_1'], task['matrix_2'])
  --mprint(task['size'], task['size'], mm)
  task['matrix_res'] = mm
  simgrid.info("Calcul is done ... Bye")
end

local n = tonumber((arg and arg[1]) or 1)
function mkmatrix(rows, cols)
  local count = 1
  local mx = {}
  for i = 0, (rows - 1) do
    local row = {}
    for j = 0, (cols - 1) do
      row[j] = count
      count = count + 1
    end
    mx[i] = row
  end
  return mx
end

function mmult(rows, cols, m1, m2)
  local m3 = {}
  for i = 0, (rows - 1) do
    m3[i] = {}
    for j = 0, (cols - 1) do
      local rowj = 0
      for k = 0, (cols - 1) do
	rowj = rowj + m1[i][k] * m2[k][j]
      end
      m3[i][j] = rowj
    end
  end
  return m3
end

function mprint(rows, cols, m)
  for i = 0, (cols - 1) do
    for j = 0, (rows - 1 )do
      print(m[i][j])
    end
  end
end

