###############################################################################
#
# common.mk
#
# Import variables:
#   PROJECT_ROOT = The project root directory.
#   INSTALL_DIR = The binary code installation directory.
#
###############################################################################

#SHELL = /bin/sh

#CC      = gcc
#CXX     = g++
#CPP     = gcc
#LD      = $(CPP)
#AR      = ar
#RANLIB  = ranlib
LINT    = lint

MAKE    = make
INSTALL = /bin/install -c
INSTALL_PROGRAM = ${INSTALL}
RM      = rm

CFLAGS   = $(IFX_CFLAGS) -fno-builtin 

WARNING = -pedantic
ifeq ($(rough), )
 WARNING +=  -Wall  
endif

OPTIMI  = -O2

CPPFLAGS +=
DEFS    =

ARFLAGS = crv

#PROFILE = -pg
PROFILE = 

ifeq ($(OS), )
  OS := $(shell uname)
  ifeq ($(OS), SunOS)
	ifeq ($(word 1, $(subst ., , $(shell uname -r))), 5)
		OS := Solaris
	endif
        CFLAGS += -DSOLARIS 
  endif
  ifeq ($(OS), Linux)
        CFLAGS += -DLINUX
  endif
endif

os       = $(OS)
root_dir = ${PROJECT_ROOT}
srcdir   = .
#srcdir   = ${root_dir}/src
bindir   = ${root_dir}/${os}/bin
libdir   = ${root_dir}/${os}/lib
objdir   = ${root_dir}/${os}/obj
incdir   = ${root_dir}/include 
mkdir    = ${root_dir}/make
#mkdir    = ${root_dir}/src/make
install_libdir   = ${INSTALL_DIR}/lib
install_incdir   = ${INSTALL_DIR}/include

ifneq ($(release), )
  CFLAGS  += $(OPTIMI) $(WARNING)
  bindir   = ${root_dir}/${os}/release.bin
  libdir   = ${root_dir}/${os}/release.lib
  objdir   = ${root_dir}/${os}/release.obj
else
  CFLAGS  += -g $(WARNING) $(PROFILE)
endif

ifneq ($(mts), )
 CFLAGS += -D_REENTRANT
endif

