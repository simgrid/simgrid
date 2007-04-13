Dim oShell
Dim iExitCode

Set oShell = WScript.CreateObject ("WSCript.shell")
	

oShell.run "cmd /K C:\dev\cvs\simgrid\examples\gras\all2all\bin\_all2all_receiver 4000 5,1,false"
oShell.run "cmd /K C:\dev\cvs\simgrid\examples\gras\all2all\bin\_all2all_receiver 4001 5,1,false"
oShell.run "cmd /K C:\dev\cvs\simgrid\examples\gras\all2all\bin\_all2all_receiver 4002 5,1,false"
oShell.run "cmd /K C:\dev\cvs\simgrid\examples\gras\all2all\bin\_all2all_receiver 4003 5,1,false"
oShell.run "cmd /K C:\dev\cvs\simgrid\examples\gras\all2all\bin\_all2all_receiver 4004 5,1,false"

WScript.Sleep 1000

cmd = "cmd /K C:\dev\cvs\simgrid\examples\gras\all2all\bin\_all2all_sender spinnaker.loria.fr:4000 spinnaker.loria.fr:4001 spinnaker.loria.fr:4002 spinnaker.loria.fr:4003 spinnaker.loria.fr:4004 512"

iExitCode = oShell.run(cmd,1,False) 
iExitCode = oShell.run(cmd,1,False)
iExitCode = oShell.run(cmd,1,False)
iExitCode = oShell.run(cmd,1,False)
iExitCode = oShell.run(cmd,1,False)


Set oShell = Nothing
