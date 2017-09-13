#
# defaults
#
ifndef ATH_CAP_SUPERG_FF
export ATH_CAP_SUPERG_FF=1
endif

ifndef ATH_CAP_DYNTURBO
export ATH_CAP_DYNTURBO=0
endif

ifndef ATH_CAP_SUPERG_COMP
export ATH_CAP_SUPERG_COMP=0
endif

ifndef ATH_CAP_XR
export ATH_CAP_XR=1
endif

ifndef ATH_CAP_TPC
export ATH_CAP_TPC=0
endif

ifndef ATH_CAP_CWM
export ATH_CAP_CWM=1
endif

ifndef ATH_CHAINMASK_SELECT
export ATH_CHAINMASK_SELECT=1
endif

ifndef ATH_FORCE_PPM
export ATH_FORCE_PPM=1
endif

ifndef ATH_FORCE_BIAS
export ATH_FORCE_BIAS=1
endif

ifndef AR5416_EMULATION
export AR5416_EMULATION=1
endif

#
# directives
#
ifneq ($(strip ${ATH_CAP_SUPERG_FF}),0)
COPTS+= -DATH_SUPERG_FF=1
endif

ifneq ($(strip ${ATH_CAP_DYNTURBO}),0)
COPTS+= -DATH_SUPERG_DYNTURBO=1 -DATH_TURBO_SCAN=1
endif

ifneq ($(strip ${ATH_CAP_SUPERG_COMP}),0)
COPTS+= -DATH_SUPERG_COMP=1
endif

ifneq ($(strip ${ATH_CAP_XR}),0)
COPTS+= -DATH_SUPERG_XR=1
endif

ifneq ($(strip ${ATH_CAP_TPC}),0)
COPTS+= -DATH_CAP_TPC=1
endif

ifneq ($(strip ${ATH_CAP_CWM}),0)
COPTS+= -DATH_CWM=1
endif

ifneq ($(strip ${ATH_CHAINMASK_SELECT}),0)
COPTS+= -DATH_CHAINMASK_SELECT=1
endif

ifneq ($(strip ${ATH_FORCE_PPM}),0)
COPTS+= -DATH_FORCE_PPM=1
endif

ifneq ($(strip ${ATH_FORCE_BIAS}),0)
COPTS+= -DATH_FORCE_BIAS=1
endif

ifneq ($(strip ${AR5416_EMULATION}),0)
COPTS+= -DAR5416_EMULATION=1
endif


COPTS+= -DDEBUG_PKTLOG
