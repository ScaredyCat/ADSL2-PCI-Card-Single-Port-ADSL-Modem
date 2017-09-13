#ifndef AMAZON_ATM_MIB_H
#define AMAZON_ATM_MIB_H

/******************************************************************************
**
** FILE NAME    : atm_mib.h
** PROJECT      : Danube
** MODULES      : ATM
**
** DATE         : 31 DEC 2004
** AUTHOR       : Liu Peng
** DESCRIPTION  : ATM API Structures (MIB)
** COPYRIGHT    :       Copyright (c) 2006
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author         $Comment
** 31 DEC 2004  Liu Peng        Initiate Version
** 23 OCT 2006  Xu Liang        Add GPL header.
*******************************************************************************/

typedef struct {
	__u32 ifHCInOctets_h;
	__u32 ifHCInOctets_l;
	__u32 ifHCOutOctets_h;
	__u32 ifHCOutOctets_l;
	__u32 ifInErrors;
	__u32 ifInUnknownProtos;
	__u32 ifOutErrors;
} atm_cell_ifEntry_t;

typedef struct {
	__u32 ifHCInOctets_h;
	__u32 ifHCInOctets_l;
	__u32 ifHCOutOctets_h;
	__u32 ifHCOutOctets_l;
	__u32 ifInUcastPkts;
	__u32 ifOutUcastPkts;
	__u32 ifInErrors;
	__u32 ifInDiscards;
	__u32 ifOutErros;
	__u32 ifOutDiscards;
} atm_aal5_ifEntry_t;

typedef struct {
	__u32 aal5VccCrcErrors;
	__u32 aal5VccSarTimeOuts;	//no timer support yet
	__u32 aal5VccOverSizedSDUs;
} atm_aal5_vcc_t;

#endif //AMAZON_ATM_MIB_H
