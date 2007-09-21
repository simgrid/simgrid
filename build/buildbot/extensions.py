

from buildbot.process import step
from buildbot.process.step import ShellCommand
from buildbot.status import builder
from buildbot.status.builder import SUCCESS,FAILURE, EXCEPTION,WARNINGS

# Define a new builder status
# Configure return the exit code 77 when the target platform don't
# bear ucontext functionnality. In this case the CustomConfigure returns
# INCOMPATIBLE_PLATFORM.

INCOMPATIBLE_PLATFORM = EXCEPTION +1

"""
CustomConfigure class for master-side representation of the
custom configure command. This class considers the error (exit code 77)
that occurs when the platform target don't bear ucontext functionnality.
"""
class CustomConfigure(ShellCommand):

    name = "configure"
    description = ["running configure"]
    descriptionDone = [name]

    # Override evaluateCommand method to handle the case of platform that 
    # don't support ucontext. The method returns INCOMPATIBLE_PLATFORM
    # in this case.
	
    def evaluateCommand(self, cmd):
        """Decide whether the command was SUCCESS, INCOMPATIBLE_PLATFORM,
        or FAILURE."""
	
	if cmd.rc == 0:
		return SUCCESS
	elif cmd.rc == 77:
		builder.Results.append('incompatible_platform')
		return INCOMPATIBLE_PLATFORM
	else:
		 return FAILURE
           
     # Override getColor method. The method returns the text "yellow" 
     # when the results parameter is INCOMPATIBLE_PLATFORM.
	
    def getColor(self, cmd, results):
        assert results in (SUCCESS, WARNINGS, FAILURE,INCOMPATIBLE_PLATFORM)
        if results == SUCCESS:
            return "green"
        elif results == WARNINGS:
            return "orange"
        elif results == INCOMPATIBLE_PLATFORM:
            return "bleu"
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
        elif results == INCOMPATIBLE_PLATFORM:
            return self.describe(True) + ["failed {incompatible platform}"]
        else:
            return self.describe(True) + ["failed"]
            


    
