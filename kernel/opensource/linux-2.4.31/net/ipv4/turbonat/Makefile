O_TARGET := turbo_nat.o
obj-m    := $(O_TARGET)

#CONFIG_FULL_PACKAGE=y
ifeq ($(CONFIG_FULL_PACKAGE),y)
obj-y    := ip_turbo_nat.o
include $(TOPDIR)/Rules.make
else
include $(TOPDIR)/Rules.make
turbo_nat.o: turbo_nat.oo
	cp turbo_nat.oo turbo_nat.o
endif
