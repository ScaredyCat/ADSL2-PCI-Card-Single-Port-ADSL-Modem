###############################################################################
#
# module.mk
#
# before including this file:
#   include ../common.mk
#
# Import variables:
#   LIBTARGET =
#   OBJECTS =
#   PROGFLAGS =
#   PROGDEFS =
#   PROGLIBS =
#
###############################################################################

CFLAGS +=  $(PROGFLAGS) $(IFX_CFLAGS)
DEFS   +=  $(PROGDEFS)
#INC = -I. -I.. -I$(incdir) -I$(srcdir)
INC = -I.

#LDFLAGS = -g $(PROFILE)
LDFLAGS += -g $(IFX_LDFLAGS)

ifeq ($(os), Solaris)
  PROGLIBS += -lsocket -lnsl -lpthread -lrt
endif

ifeq ($(os), Linux)
  PROGLIBS += -lnsl -lpthread
endif

link_command = $(LD) $(LDFLAGS) -o $@

.PHONY: clean
.SUFFIXES:

OBJ_OUT = $(foreach file, $(OBJECTS), $(objdir)/$(file))
SRC_OUT = $(OBJECTS:.o=.c)
DEP_OUT = $(OBJ_OUT:.o=.d)

-include $(DEP_OUT)

$(objdir)/%.d: %.c $(objdir)/nul
	$(SHELL) -ec "$(CC) -M $(INC) $(DEFS) $(CFLAGS) $(CPPFLAGS) $< | sed \
	's/$*\.o/$(subst /,\/,$(objdir)/$*.o) $(subst /,\/,$(objdir)/$*.d)/g' \
	> $@"

$(objdir)/%.d: %.cpp
	$(SHELL) -ec "$(CC) -M $(INC) $(DEFS) $(CFLAGS) $(CPPFLAGS) $< | sed \
	's/$*\.o/$(subst /,\/,$(objdir)/$*.o) $(subst /,\/,$(objdir)/$*.d)/g' \
	> $@"

$(objdir)/%.o: %.cpp
	$(CPP) $(CFLAGS) -c $< $(CPPFLAGS) $(DEFS) $(INC) -o $@

$(objdir)/%.o: %.c
	$(CC) -c -o $@ $(INC) $(CFLAGS) $(CPPFLAGS) $(DEFS) $<

$(LIBTARGET): $(libdir)/nul $(OBJ_OUT) $(DEP_OUT)
	$(RM) -f $@
	$(AR) $(ARFLAGS) $@ $(OBJ_OUT)
	-$(RANLIB) $@

$(APPTARGET): $(OBJ_OUT) $(DEP_OUT) $(bindir)/nul
	$(link_command) $(OBJ_OUT) -L$(libdir) $(PROGLIBS)
	
$(objdir)/nul:
	if ( test ! -f $(objdir)/nul) ; then \
		mkdir -p $(objdir); \
		echo "" > $(objdir)/nul; \
	fi

$(libdir)/nul:
	if ( test ! -f $(libdir)/nul) ; then \
		mkdir -p $(libdir); \
		echo "" > $(libdir)/nul; \
	fi

$(bindir)/nul:
	if ( test ! -f $(bindir)/nul) ; then \
		mkdir -p $(bindir); \
		echo "" > $(bindir)/nul; \
	fi

clean:
	$(RM) -f $(LIBTARGET) $(APPTARGET) \
	*.o core $(bindir)/*.ini $(OBJ_OUT) $(DEP_OUT)

#check:
#install:
#dist:
