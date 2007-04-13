Dim oShell
Dim iExitCode

Set oShell = WScript.CreateObject ("WSCript.shell")
	
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\simple_token_node  4000 127.0.0.1 4010 --create-token",1,False)
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\simple_token_node 4010 127.0.0.1 4020",1,False)
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\simple_token_node 4020 127.0.0.1 4030",1,False)
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\simple_token_node 4030 127.0.0.1 4040",1,False)
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\simple_token_node 4040 127.0.0.1 4050",1,False)
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\simple_token_node 4050 127.0.0.1 4000",1,False)

Set oShell = Nothing

'This function returns the current directory of the script 
Function GetCurrentDirectory()
	
	Dim sScriptFullName 
	
	sScriptFullName = WScript.ScriptFullName
	GetCurrentDirectory = mid(sScriptFullName,1,InStrRev(sScriptFullName,"\"))
	
End Function
