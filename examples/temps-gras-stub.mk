## Generate the temp source files on need
##

$(foreach proc, $(PROCESSES), _$(NAME)_$(proc).c) _$(NAME)_simulator.c: $(srcdir)/$(NAME).xml $(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@
	[ x@EXEEXT@ = x ] || exenv=wine; $$exenv $(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@ $(NAME) $(srcdir)/$(NAME).xml >/dev/null

$(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@:
	make -C $(top_builddir)/tools/gras/ gras_stub_generator@EXEEXT@

##
## Cleanups
CLEANFILES = _$(NAME)_simulator.c $(NAME).mk $(NAME).Makefile.local $(NAME).Makefile.remote $(NAME).deploy.sh \
             $(foreach proc, $(PROCESSES), _$(NAME)_$(proc).c)

MOSTLYCLEANFILES = $(NAME).trace
