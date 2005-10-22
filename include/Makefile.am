include_HEADERS = gras.h xbt.h
nobase_include_HEADERS = \
	xbt/misc.h \
	xbt/sysdep.h \
	xbt/asserts.h xbt/ex.h \
	xbt/log.h \
	xbt/module.h \
	xbt/dynar.h xbt/dict.h xbt/set.h \
	xbt/heap.h \
	xbt/fifo.h \
	xbt/swag.h \
	xbt/config.h \
	xbt/cunit.h \
	\
	msg/msg.h \
	msg/datatypes.h \
	\
	surf/surf_parse.h \
	surf/surfxml.h \
	\
	gras/datadesc.h gras/transport.h \
	gras/virtu.h gras/emul.h gras/process.h \
	\
	gras/messages.h gras/timer.h \
	\
	amok/base.h \
	amok/bandwidth.h

surf/surfxml.h:
	$(MAKE) -C ../src ../include/surf/surfxml.h

include $(top_srcdir)/acmacro/dist-files.mk
