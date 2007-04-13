Dim oShell
Dim sInstallDirectory	' The install directory of the simulator Simgrid
Dim sCurrentDirectory	' The current directory of the script
Dim sCommandLine		' The command line of the application to launch
Dim iExitCode			' The exit code of the application to launch

Set oShell = WScript.CreateObject ("WSCript.shell")
Set oUseEnv = oShell.Environment("USER")

' Get the install directory of Simgrid from the environment variable SG_INSTALL_DIR 
sInstallDirectory = oUseEnv("SG_INSTALL_DIR")
sCurrentDirectory = GetCurrentDirectory()
' Construct the command line of the application to launch
sCommandLine = "cmd /K " & sCurrentDirectory & "bin\simple_token_simulator " & sInstallDirectory & "\simgrid\examples\msg\small_platform.xml " & sCurrentDirectory & "simple_token.xml"

iRetVal = oShell.run(sCommandLine,1,True)

Set oShell = Nothing
Set oWshShell = Nothing

' This function returns the directory of the script
Function GetCurrentDirectory()
	Dim sScriptFullName 
	
	sScriptFullName = WScript.ScriptFullName
	GetCurrentDirectory = mid(sScriptFullName,1,InStrRev(sScriptFullName,"\"))
	
End Function

