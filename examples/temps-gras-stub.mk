## Generate the temp source files on need
##

$(foreach proc, $(PROCESSES), _$(NAME)_$(proc).c) _$(NAME)_simulator.c: $(srcdir)/$(NAME)_deployment.xml ../../../tools/gras/gras_stub_generator@EXEEXT@
	[ x@EXEEXT@ = x ] || exenv=wine; $$exenv ../../../tools/gras/gras_stub_generator@EXEEXT@ $(NAME) $(srcdir)/$(NAME)_deployment.xml >/dev/null

../../../tools/gras/gras_stub_generator@EXEEXT@:
	make -C   ../../../tools/gras/ gras_stub_generator@EXEEXT@

##
## Cleanups
CLEANFILES = _$(NAME)_simulator.c  $(NAME).Makefile.local $(NAME).Makefile.remote $(NAME).deploy.sh \
             $(foreach proc, $(PROCESSES), _$(NAME)_$(proc).c)

MOSTLYCLEANFILES = $(NAME).trace
