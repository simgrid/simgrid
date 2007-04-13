Dim oShell

Set oShell = WScript.CreateObject ("WSCript.shell")
	
oShell.run "cmd /K " & "bin\rpc_server.exe 4002,1,false" 
WScript.Sleep 1000
oShell.run "cmd /K " & "bin\rpc_forwarder.exe 4003 spinnaker.loria.fr 4002,1,false"
WScript.Sleep 1000
oShell.run "cmd /K " & "bin\rpc_client.exe spinnaker.loria.fr 4002 spinnaker.loria.fr 4003,1,false"

Set oShell = Nothing



Function GetCurrentDirectory()

	Dim sScriptFullName 
	
	sScriptFullName = WScript.ScriptFullName
	
	GetCurrentDirectory = mid(sScriptFullName,1,InStrRev(sScriptFullName,"\"))

End Function
