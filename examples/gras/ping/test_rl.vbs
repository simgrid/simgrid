Dim oShell
Dim iExitCode1
Dim iExitCode2

Set oShell = WScript.CreateObject ("WSCript.shell")
	
iExitCode1 = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\ping_server 4002",1,False)
'WScript.Sleep 1000
iExitCode2 = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\ping_client 127.0.0.1 4002",1,False)

 
Set oShell = Nothing

'This function returns the current directory of the script 
Function GetCurrentDirectory()
	
	Dim sScriptFullName 
	
	sScriptFullName = WScript.ScriptFullName
	GetCurrentDirectory = mid(sScriptFullName,1,InStrRev(sScriptFullName,"\"))
	
End Function
