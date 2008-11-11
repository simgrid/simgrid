

import re        
from buildbot.steps.source import SVN
from buildbot.steps.shell import ShellCommand
from buildbot.steps.transfer import FileDownload

from buildbot.status import builder
from buildbot.status.builder import SUCCESS,FAILURE, EXCEPTION,WARNINGS

from buildbot.process.properties import WithProperties


# Define a new builder status
# Configure return the exit code 77 when the target platform don't
# bear ucontext functionnality. In this case the CustomConfigure returns
# INCOMPATIBLE_PLATFORM.

"""
CustomConfigure class for master-side representation of the
custom configure command. This class considers the error (exit code 77)
that occurs when the platform target don't bear ucontext functionnality.
"""
class CustomConfigure(ShellCommand):

    name = "configure"
    description = ["running configure"]
    descriptionDone = [name]
    incompatible = False

    # Override evaluateCommand method to handle the case of platform that 
    # don't support ucontext. The method returns sets self.incompatible
    # in this case.
	
    def evaluateCommand(self, cmd):
        """Decide whether the command was SUCCESS, FAILURE or incompatible platform"""
	
	if cmd.rc == 0:
		return SUCCESS
	elif cmd.rc == 77:
                self.incompatible = True
		return FAILURE
	else:
 	        return FAILURE
           
     # Override getColor method. The method returns the text "blue" when platform is incompatible
	
    def getColor(self, cmd, results):
        assert results in (SUCCESS, WARNINGS, FAILURE)
        if results == SUCCESS:
            return "green"
        elif results == WARNINGS:
            return "orange"
        elif self.incompatible:
            return "blue"
        else:
            return "red"
            
        # Override getText method. The method calls describe method
        # with the text "failed {incompatible platform}" when the platform
        # don't support ucontext functionnality.
	       
    def getText(self, cmd, results):
        if results == SUCCESS:
            return self.describe(True) + ["Configure success"]
        elif results == WARNINGS:
            return self.describe(True) + ["warnings"]
        elif self.incompatible:
            return self.describe(True) + ["failed {incompatible platform}"]
        else:
            return self.describe(True) + ["failed"]
            
    def maybeGetText2(self, cmd, results):
        if results == SUCCESS:
            pass
        elif results == WARNINGS:
            pass
        elif self.incompatible:
            return ["incompatible platform"]
        else:
            return self.describe(True) + ["failed"]
        return []


"""
Cleanup the build dir, and setup a SVN revision in the waterfall afterward
"""
class CleanupCommand(ShellCommand):
   name="cleanup"
   descriptionDone="cleanup"
   command=["bash","-c","rm -rf * .svn"]

   def maybeGetText2(self,cmd,results):
       if self.build.getProperty("revision") == None:
           return ["Missing svn revision"]
       return ["SVN r%s" % self.build.getProperty("revision")]
        
"""
Just like a plain SVN, but displays the current revision in the waterfall afterall
"""
class CustomSVN(SVN):        
   def getText(self,cmd,results):
       lines = cmd.logs['stdio'].getText()
       r = re.search(' (\d+).',lines)
       if results == SUCCESS and r:
            return self.describe(True) + ["updated to %s" % r.group(1)]
       elif results == SUCCESS:
            return self.describe(True) + ["updated to some version"]
       else:
            return self.describe(True) + ["failed"]
        
   def maybeGetText2(self,cmd,results):
       lines = cmd.logs['stdio'].getText()
       r = re.search(' (\d+).',lines)
       if results == SUCCESS and r:
           return ["SVN r%s" % r.group(1)]
       else:
           return []
              
"""
CustomCheck class for master-side representation of the checkall results.
This class stores and displays the amount of errors in the checks.
"""
class CustomCheck(ShellCommand):
    name = "check"
    description = ["running checks"]
    descriptionDone = [name]

            
   # Override per step getText method.	       
    def getText(self, cmd, results):
       lines = cmd.logs['stdio'].getText().split("\n")
       re.compile('^FAIL:')
       fail = len( filter(lambda line: re.search('^FAIL:', line), lines) )
       re.compile('^XFAIL:')
       xfail = len( filter(lambda line: re.search('^XFAIL:', line), lines) )
       re.compile('^SKIP:')
       skip = len( filter(lambda line: re.search('^SKIP:', line), lines) )
       re.compile('^XPASS:')
       xpass = len( filter(lambda line: re.search('^XPASS:', line), lines) )
       re.compile('^PASS:')
       passed = len( filter(lambda line: re.search('^PASS:', line), lines) )
       
       res = ""
       if fail != 0:
           res += ("%d failed, " % fail)
       if skip != 0:
           res += ("%d skipped, " % skip)
       if xpass != 0:
           res += ("%d unexpected success, " % xpass)
       if xfail != 0:
           res += ("%d expected failure, " % xfail)
       res += ("%d total" % (passed + fail + skip + xpass + xfail))
              
       if results == SUCCESS:
            return self.describe(True) + ["Success (%s)" % res]
       elif results == WARNINGS:
            return self.describe(True) + ["warnings"]
       elif fail == 0:
            return self.describe(True) + ["failed strangly"]
       else:
            return self.describe(True) + [res]
                
   # Add some text to the top-column box
    def getText2(self, cmd, results):
       lines = cmd.logs['stdio'].getText().split("\n")
       re.compile('^FAIL:')
       fail = len( filter(lambda line: re.search('^FAIL:', line), lines) )
       re.compile('^XFAIL:')
       xfail = len( filter(lambda line: re.search('^XFAIL:', line), lines) )
       re.compile('^SKIP:')
       skip = len( filter(lambda line: re.search('^SKIP:', line), lines) )
       re.compile('^XPASS:')
       xpass = len( filter(lambda line: re.search('^XPASS:', line), lines) )
       re.compile('^PASS:')
       passed = len( filter(lambda line: re.search('^PASS:', line), lines) )
       
       res = ""
       if fail != 0:
           res += ("%d failed, " % fail)
       if skip != 0:
           res += ("%d skipped, " % skip)
       if xpass != 0:
           res += ("%d unexpected success, " % xpass)
       if xfail != 0:
           res += ("%d expected failure, " % xfail)
       res += ("%d total" % (passed + fail + skip + xpass + xfail))
              
       if results == SUCCESS:
            return ["All tests ok (%s)" % res]
       elif results == WARNINGS:
            return ["Warnings (%s)" % res]
       elif fail == 0:
            return ["Tests failed strangly"]
       else:
            return [res]
