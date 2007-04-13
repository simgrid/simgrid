Dim oShell
Dim iExitCode

Set oShell = WScript.CreateObject ("WSCript.shell")

'run the slaves
	
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\pmm_slave 127.0.0.1:4242",1,False)
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\pmm_slave 127.0.0.1:4242",1,False)
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\pmm_slave 127.0.0.1:4242",1,False)
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\pmm_slave 127.0.0.1:4242",1,False)
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\pmm_slave 127.0.0.1:4242",1,False)
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\pmm_slave 127.0.0.1:4242",1,False)
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\pmm_slave 127.0.0.1:4242",1,False)
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\pmm_slave 127.0.0.1:4242",1,False)
'iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\pmm_slave 127.0.0.1:4242",1,False)


'run the master
iExitCode = oShell.run("cmd /K " & GetCurrentDirectory() & "bin\pmm_master 4242",1,False)

Set oShell = Nothing

'This function returns the current directory of the script 
Function GetCurrentDirectory()
	
	Dim sScriptFullName 
	
	sScriptFullName = WScript.ScriptFullName
	GetCurrentDirectory = mid(sScriptFullName,1,InStrRev(sScriptFullName,"\"))
	
End Function
