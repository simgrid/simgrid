// SimGrid configuration file for Windows.
// Register all environement variables needed for the compilation of SimGrid library
// and all the tesh suite, examples and test suites.


// FIXME declare the include directory in the environment of the compilation.

// Test if the string s is in the array a.
function ArrayContains(a, s)
{
	var i = 0;
	
	for(i = 0; i < a.length; i++)
		if(a[i] == s)
		 	return true;
			 
	return false;		
}

// SimGrid configuration file for Windows.
// Register all environement variables needed for the compilation of SimGrid library
// and all the tesh suite, examples and test suites.

var Shell;
var UserEnv;
var FileSystem;


var DebugLibraryPath;		// The path of the SimGrid debug library
var ReleaseLibraryPath;		// The path of the SimGrid release library

var Lib;					// The value of the environment variable LIB
var Include;				// The value of the environment variable INCLUDE
var Libs;					// SimGrid library name (the value of the environment variable LIBS).


var VCInstallDir;			// Visual C install directory
var SGBuildDir;				// This build directory
var JNIIncludeDir;			// The path of the jni.h header

var Args = WScript.Arguments;

// check the arguments to be sure it's right
if (Args.Count() < 3)
{
   WScript.Echo("SimGrid Configuration.");
   WScript.Echo("Configure the environment of the compilation of SimGrid library");
   WScript.Echo("\n\tUsage: CScript SimGridConfig.js <SGBuildDir> <VCInsallDir> <JNIIncludeDir>");
   WScript.Quit(1);
}

SGBuildDir = Args.Item(0);
VCInstallDir = Args.Item(1);
JNIIncludeDir = Args.Item(2); 

var FileSystem = new ActiveXObject("Scripting.FileSystemObject");

// Build the 2 paths of SimGrid library paths (debug and release)
ReleaseLibraryPath = SGBuildDir + "\\build\\vc7\\simgrid\\Release";
DebugLibraryPath = SGBuildDir + "\\build\\vc7\\simgrid\\Debug";

// Check the directories specified as parameters of the script.

if(!FileSystem.FolderExists(SGBuildDir))
{
	WScript.Echo("Not a directory `(" + SGBuildDir + ")'");	
	WScript.Quit(2);
}
else if(!FileSystem.FolderExists(VCInstallDir))
{
	WScript.Echo("Not a directory `(" + VCInstallDir + ")'");	
	WScript.Quit(3);
}
else if(!FileSystem.FolderExists(JNIIncludeDir))
{
	WScript.Echo("Not a directory `(" + JNIIncludeDir + ")'");	
	WScript.Quit(5);
}

Shell = WScript.CreateObject("Wscript.Shell");
UserEnv = Shell.Environment("USER");

WScript.Echo("Configuration of SimGrig Library Compilation in progress...");
WScript.Echo("Build directory : " + SGBuildDir);	
WScript.Echo("Visual C install directory : " + VCInstallDir);
WScript.Echo("Java Native Interface include directory : " + JNIIncludeDir);


// Build the content of the environment variable LIB.
Lib = UserEnv("LIB");

if(typeof(UserEnv("LIB")) != "undefined" && Lib.length > 0)
{
	var a = Lib.split(";");
	
	if(!ArrayContains(a,DebugLibraryPath))
		Lib = Lib + DebugLibraryPath + ";";;
		
	 if(!ArrayContains(a,ReleaseLibraryPath))
		Lib = Lib +  ReleaseLibraryPath + ";";
		
	if(!ArrayContains(a, VCInstallDir + "lib"))
		Lib = Lib +  VCInstallDir + "lib" + ";";
		
	if(!ArrayContains(a, VCInstallDir + "PlatformSDK\\Lib"))
		Lib = Lib +  VCInstallDir + "PlatformSDK\\Lib" + ";";	
}
else
{
	// SimGrid librairy paths
	 Lib = DebugLibraryPath + ";";
	 Lib = Lib + ReleaseLibraryPath + ";";
	 // Visual C library paths.
	 Lib = Lib + VCInstallDir + "lib" + ";";
	 Lib = Lib + VCInstallDir + "PlatformSDK\\Lib" + ";";
	 	
}

// Build the content of the environment variable INCLUDE.
Include = UserEnv("INCLUDE");

if(typeof(UserEnv("INCLUDE")) != "undefined" && Include.length > 0)
{
	
	var a = Include.split(";");
	
	// Add the Visual C include directories
	if(!ArrayContains(a, VCInstallDir + "include"))
		Include = Include + VCInstallDir + "include" + ";";
		
	if(!ArrayContains(a,VCInstallDir + "PlatformSDK\\include"))
		Include = Include +  VCInstallDir + "PlatformSDK\\include" + ";";
	
	
	// Add the SimGrid include directories
	if(!ArrayContains(a,SGBuildDir + "src"))
		Include = Include + SGBuildDir + "src" + ";";
		
	if(!ArrayContains(a,SGBuildDir + "include"))
		Include = Include + SGBuildDir + "include" + ";";
		
	if(!ArrayContains(a,SGBuildDir + "src\\include"))
		Include = Include + SGBuildDir + "src\\include" + ";";
		
	// Add the JNI include directories.
	if(!ArrayContains(a,JNIIncludeDir))
		Include = Include +  JNIIncludeDir + ";";
		
	if(!ArrayContains(a,JNIIncludeDir + "awin32"))
		Include = Include + JNIIncludeDir + "win32" + ";";
	
}
else
{
	// Visual C include directories
	Include = VCInstallDir + "include" + ";";
	Include = Include + VCInstallDir + "PlatformSDK\\include" + ";";
	// SimGrid include directories.
	Include = Include + SGBuildDir + "src" + ";";
	Include = Include + SGBuildDir + "include" + ";" ;
	Include = Include + SGBuildDir + "src\\include" + ";";
	// JNI include directories.
	Include = Include + JNIIncludeDir + ";";
	Include = Include + JNIIncludeDir + "win32" + ";";
	 
}

// Build the content of the environment variable LIBS.
Libs = UserEnv("LIBS");

if(typeof(UserEnv("LIBS")) != "undefined" && Libs.length > 0)
{
	var a = Libs.split(";");
	
	if(!ArrayContains(a, "simgrid.lib"))
		Libs = Libs + "simgrid.lib" + ";";
}
else
{
	Libs = "simgrid.lib" + ";";
}

// Update the environment variables.
UserEnv("LIB") = Lib;
UserEnv("INCLUDE") = Include;
UserEnv("LIBS") = Libs;



WScript.Echo("Configuration successeded");
