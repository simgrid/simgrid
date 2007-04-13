Dim oShell

Set oShell = WScript.CreateObject ("WSCript.shell")

oShell.run "cmd /K C:\dev\cvs\simgrid\examples\gras\p2p\can\bin\_can_node 4002,1,false" 

WScript.Sleep 1000

oShell.run "cmd /K C:\dev\cvs\simgrid\examples\gras\p2p\can\bin\_can_node 127.0.0.1 4002,1,false" 

Set oShell = Nothing