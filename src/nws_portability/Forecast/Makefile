#
# Makefile for Forecaster

# include common variables
include ../Makedefs

# these are the targets
LIBRARY = $(buildlibdir)/libNwsForecast.a

# objects we are creating here
OBJS	= 	$(buildobjdir)/exp_smooth.o \
		$(buildobjdir)/fbuff.o \
		$(buildobjdir)/forc.o \
		$(buildobjdir)/forecast_api.o \
		$(buildobjdir)/last_value.o \
		$(buildobjdir)/median.o \
		$(buildobjdir)/mse_forc.o \
		$(buildobjdir)/run_mean.o

.PHONY: all clean dist

all: $(LIBRARY) 

clean:
	@$(RM) -f $(OBJS) 

dist:
	@$(INSTALL) -d $(DIST_ROOT)/Forecast
	@$(CP) Makefile *.c $(DIST_ROOT)/Forecast

$(LIBRARY): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)
	$(RANLIB) $@

#pure:
#	$(MAKE) LD="$(PURIFY) $(LD)"


# Compilation rules for C.
$(buildobjdir)/%.o : %.c ../Include/config_portability.h ../Include/%.h ../Include/forc.h
	$(CC) $(INCLUDES) $(CFLAGS) -g -c $< -o $@

