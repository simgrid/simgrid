// Tesh configuration file for Windows.
// Register all environement variables needed by Tesh
// during its compilation and during the running of Tesh files.


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

var SGBuildDir;				// This build directory
var TeshDir;
var TeshExampleDir;
var TeshSuiteDir;
var TestSuiteDir;
var ExampleDir;
var SrcDir;

var Path;
var Include;

var Args = WScript.Arguments;

// check the arguments to be sure it's right
if (Args.Count() < 1)
{
   WScript.Echo("Tesh Configuration.");
   WScript.Echo("Configure the environment of Tesh (mini test shell)");
   WScript.Echo("\n\tUsage: CScript TeshConfig.js <SGBuildDir> <TeshVersion>");
   WScript.Quit(1);
}

SGBuildDir = Args.Item(0);
TeshVersion = Args.Item(1);

var FileSystem = new ActiveXObject("Scripting.FileSystemObject");

// Check the directories specified as parameters of the script.

if(!FileSystem.FolderExists(SGBuildDir))
{
	WScript.Echo("Not a directory `(" + SGBuildDir + ")'");	
	WScript.Quit(2);
}



// Build the directories

TeshDir = SGBuildDir + "tools\\tesh" + TeshVersion;
TeshExampleDir = TeshDir + "\\examples";
TeshSuiteDir = SGBuildDir + "teshsuite";
TestSuiteDir = SGBuildDir + "testsuite";
ExampleDir =  SGBuildDir + "examples";
SrcDir = ExampleDir + "\\msg";


Shell = WScript.CreateObject("Wscript.Shell");
UserEnv = Shell.Environment("USER");

WScript.Echo("Configuration of Tesh Library Compilation in progress...");
WScript.Echo("Build directory : " + SGBuildDir);


// Include Tesh in the Path
Path = UserEnv("PATH");

if(typeof(UserEnv("PATH")) != "undefined" && Path.length > 0)
{
	var a = Path.split(";");
	
	// Add the Visual C include directories
	if(!ArrayContains(a, TeshDir))
		Path = Path + TeshDir + ";";
}
else
{
	Path = TeshDir + ";";
}

Include = UserEnv("INCLUDE");

if(typeof(UserEnv("INCLUDE")) != "undefined" && Include.length > 0)
{
	var a = Include.split(";");
	
	// Add the Visual C include directories
	if(!ArrayContains(a, TeshDir + "\\include"))
		Include = Include + TeshDir + "\\include" + ";";
		
	if(!ArrayContains(a, TeshDir + "\\w32\\include"))
		Include = Include + TeshDir + "\\w32\\include" + ";";
}
else
{
	Include = TeshDir + "\\include";
	Include = Include + ";" + TeshDir + "\\w32\\include" + ";";
}

// Build environement of Tesh.
UserEnv("TESH") = "tesh";
UserEnv("TESH_DIR") = TeshDir;
UserEnv("TESHEXAMPLE_DIR") = TeshExampleDir;
UserEnv("TESHSUITE_DIR") = TeshSuiteDir;
UserEnv("TESTSUITE_DIR") = TestSuiteDir;
UserEnv("EXAMPLE_DIR") = ExampleDir;
UserEnv("SRCDIR") = SrcDir;
UserEnv("INCLUDE") = Include;


UserEnv("PATH") = Path;


WScript.Echo("Configuration successeded");
