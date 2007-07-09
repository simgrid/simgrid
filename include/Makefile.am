include_HEADERS = gras.h xbt.h
nobase_include_HEADERS = \
	xbt/misc.h \
	xbt/sysdep.h \
	xbt/virtu.h \
	xbt/str.h \
	xbt/function_types.h \
	xbt/asserts.h xbt/ex.h \
	xbt/log.h \
	xbt/module.h \
	xbt/mallocator.h \
	xbt/dynar.h xbt/dict.h xbt/set.h \
	xbt/heap.h \
	xbt/graph.h \
	xbt/fifo.h \
	xbt/swag.h \
	xbt/matrix.h \
	xbt/peer.h \
	xbt/config.h \
	xbt/cunit.h \
	xbt/graphxml_parse.h \
	xbt/graphxml.h \
	\
	xbt/synchro.h \
	\
	msg/msg.h \
	msg/datatypes.h \
	\
	simdag/simdag.h \
	simdag/datatypes.h \
	\
	surf/surfxml_parse.h \
	surf/surfxml.h \
	\
	gras/datadesc.h gras/transport.h \
	gras/virtu.h gras/emul.h gras/process.h gras/module.h \
	\
	gras/messages.h gras/timer.h \
	\
	amok/peermanagement.h \
	amok/bandwidth.h

surf/surfxml.h:
	$(MAKE) -C ../src ../include/surf/surfxml.h

xbt/graphxml.h:
	$(MAKE) -C ../src ../include/xbt/graphxml.h

include $(top_srcdir)/acmacro/dist-files.mk
