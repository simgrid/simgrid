Dim oShell
Dim iExitCode

Set oShell = WScript.CreateObject ("WSCript.shell")
	
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\chrono_multiplier",1,False)

Set oShell = Nothing

'This function returns the current directory of the script 
Function GetCurrentDirectory()
	
	Dim sScriptFullName 
	
	sScriptFullName = WScript.ScriptFullName
	GetCurrentDirectory = mid(sScriptFullName,1,InStrRev(sScriptFullName,"\"))
	
End Function
