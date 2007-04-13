Dim oShell
Dim iExitCode1

Set oShell = WScript.CreateObject ("WSCript.shell")
	
iExitCode1 = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\timer_client",1,False)

Set oShell = Nothing

'This function returns the current directory of the script 
Function GetCurrentDirectory()
	
	Dim sScriptFullName 
	
	sScriptFullName = WScript.ScriptFullName
	GetCurrentDirectory = mid(sScriptFullName,1,InStrRev(sScriptFullName,"\"))
	
End Function
