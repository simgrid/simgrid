# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


include_HEADERS = gras.h xbt.h
nobase_include_HEADERS = \
	xbt/misc.h \
	xbt/sysdep.h \
	xbt/virtu.h \
	xbt/str.h xbt/strbuff.h xbt/hash.h \
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
	xbt/time.h \
	xbt/synchro.h  xbt/synchro_core.h\
	xbt/queue.h \
	\
	msg/msg.h \
	msg/datatypes.h \
	\
	simdag/simdag.h \
	simdag/datatypes.h \
	\
	smpi/smpi.h \
	\
	surf/surfxml_parse.h \
	surf/simgrid_dtd.h \
	\
	gras/datadesc.h gras/transport.h \
	gras/virtu.h gras/emul.h gras/process.h gras/module.h \
	\
	gras/messages.h gras/timer.h \
	\
	amok/peermanagement.h \
	amok/bandwidth.h

surf/simgrid_dtd.h:
	$(MAKE) -C ../src ../include/surf/simgrid_dtd.h

xbt/graphxml.h:
	$(MAKE) -C ../src ../include/xbt/graphxml.h


include $(top_srcdir)/acmacro/dist-files.mk
