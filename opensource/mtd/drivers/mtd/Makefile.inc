#
# Out of tree drivers/mtd/Makefile.inc
#
# $Id: Makefile.inc,v 1.2 2003/07/02 10:44:07 dwmw2 Exp $ 

ifndef CONFIG_MTD
include $(mtd)/defconfig

# This must be first in the include path, so it goes in $(CC) rather
# then $(EXTRA_CFLAGS)

CC += -I$(mtd)/../../include
EXTRA_CFLAGS := -g -DMTD_OUT_OF_TREE -Werror

ifdef SIMPLEMAP
EXTRA_CFLAGS += -DSIMPLEMAP
endif

endif
include $(obj)/Makefile.common
