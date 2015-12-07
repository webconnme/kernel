/*******************************************************************************
	Copyright (C) 2009 NEXELL Co., All Rights Reserved
	NEXELL Co. Proprietary & Confidential

	project		: LF2000, DTV
	file name	: nx_spdif.c
	description	: spdif prototype
	coded by	: Charles Park
................................................................................
	modification history
	2010.10.18	: Hans Han			change to c code
	2009.12.28	: Charles Park		1st version
*******************************************************************************/

//------------------------------------------------------------------------------
//	Include
//------------------------------------------------------------------------------
#include <nx_spdif.h>

static struct NX_SPDIF_RegisterSet *__g_pRegister = CNULL;
//==============================================================================
// Module Implementation
//==============================================================================
CBOOL NX_SPDIF_Initialize( void )
{
	static CBOOL bInit = CFALSE;

	if( CFALSE == bInit )
	{
		__g_pRegister = CNULL;

		bInit = CTRUE;
	}

	return CTRUE;
}

U32 NX_SPDIF_GetNumberOfModule( void )
{
	return NUMBER_OF_SPDIF_MODULE;
}

U32		NX_SPDIF_GetPhysicalAddress( void )
{
	return (U32)PHY_BASEADDR_SPDIF_MODULE;
}

U32		NX_SPDIF_GetSizeOfRegisterSet( void )
{
	return (U32)sizeof(struct NX_SPDIF_RegisterSet );
}

void	NX_SPDIF_SetBaseAddress( U32 BaseAddress )
{
	NX_ASSERT( CNULL != BaseAddress );

	__g_pRegister = (struct NX_SPDIF_RegisterSet *)BaseAddress;
}

U32	NX_SPDIF_GetBaseAddress( void )
{
	return (U32)__g_pRegister;
}
//------------------------------------------------------------------------------
// @brief Open Module
//------------------------------------------------------------------------------
CBOOL NX_SPDIF_OpenModule( void )
{
	NX_ASSERT(	! NX_SPDIF_CheckBusy() );
	return CTRUE;
}

//------------------------------------------------------------------------------
// @brief Close Module
//------------------------------------------------------------------------------
CBOOL NX_SPDIF_CloseModule( void )
{
	NX_ASSERT(	! NX_SPDIF_CheckBusy() );
	return CTRUE;
}

//------------------------------------------------------------------------------
// @brief Check Busy
//------------------------------------------------------------------------------
CBOOL NX_SPDIF_CheckBusy( void )
{
	NX_ASSERT( CNULL != __g_pRegister );

	return CFALSE;
}

void NX_SPDIF_SetInterruptEnable ( S32 IntNum, CBOOL Enable )
{
	U32 regvalue;
	NX_ASSERT( CNULL != __g_pRegister );
	NX_ASSERT( !(IntNum & ~0xF));

	regvalue = __g_pRegister->ENBIRQ;
	if(Enable)
		regvalue |= (1<< IntNum );
	else
		regvalue &= ~(1<<IntNum);

	WriteIODW(&__g_pRegister->ENBIRQ, (U32) regvalue);
}

void NX_SPDIF_SetInterruptEnable32 ( U32 EnableFlag )
{
	U32 regvalue;
	NX_ASSERT( CNULL != __g_pRegister );

	regvalue = __g_pRegister->ENBIRQ;
	regvalue &= ~0xF;
	regvalue |= (EnableFlag & 0xf);

	WriteIODW(&__g_pRegister->ENBIRQ, (U32) regvalue);
}

U32 NX_SPDIF_GetInterruptEnable32 ( void )
{
	NX_ASSERT( CNULL != __g_pRegister );

	return (__g_pRegister->ENBIRQ & 0xf);
}

CBOOL	NX_SPDIF_GetInterruptPending( S32 IntNum )
{
	NX_ASSERT( CNULL != __g_pRegister );
	return ((__g_pRegister->ENBIRQ>>4) & 1<<(IntNum))? CTRUE : CFALSE;
}

CBOOL	NX_SPDIF_GetInterruptPendingAll( void )
{
	NX_ASSERT( CNULL != __g_pRegister );
	return (__g_pRegister->ENBIRQ & 0xF0) ? CTRUE : CFALSE;
}

U32 NX_SPDIF_GetInterruptPending32 ( void )
{
	NX_ASSERT( CNULL != __g_pRegister );

	return ((__g_pRegister->ENBIRQ>>4) & 0xf);
}

void	NX_SPDIF_ClearInterruptPending( S32 IntNum )
{
	register U32 regvalue;
	NX_ASSERT( CNULL != __g_pRegister );

	regvalue = __g_pRegister->ENBIRQ;
	regvalue |= (1<<(IntNum+4));
	WriteIODW(&__g_pRegister->ENBIRQ, (U32) regvalue);
}

void NX_SPDIF_ClearInterruptPending32( U32 PendingFlag )
{
	register U32 regvalue;
	NX_ASSERT( CNULL != __g_pRegister );

	regvalue = __g_pRegister->ENBIRQ;
	regvalue |= ((PendingFlag<<4) & 0xf0);

	WriteIODW(&__g_pRegister->ENBIRQ, (U32) regvalue);
}

void NX_SPDIF_ClearInterruptPendingAll( void )
{
	register U32 regvalue;
	NX_ASSERT( CNULL != __g_pRegister );

	regvalue = __g_pRegister->ENBIRQ;
	regvalue |= 0xF0;

	WriteIODW(&__g_pRegister->ENBIRQ, (U32) regvalue);
}

S32 NX_SPDIF_GetInterruptNumber(void)
{
	return (S32)INTNUM_OF_SPDIF_MODULE;
}
//--------------------------------------------------------------------------
/// @name   MES_IDMAable implementation
//--------------------------------------------------------------------------
U32 NX_SPDIF_GetDMABusWidth ( void )
{
    return 32;
}

U32		NX_SPDIF_GetDMABaseAddress ( void )
{
	return (U32)PHY_BASEADDR_SPDIF_MODULE;
}

U32		NX_SPDIF_GetDMAIndex ( void )
{
	return (U32)DMAINDEX_OF_SPDIF_MODULE_RX;
}

//==============================================================================
// SPDIF Control
//==============================================================================
void NX_SPDIF_SetCtrl ( U32 Ctrl )
{
	NX_ASSERT( CNULL != __g_pRegister );

//	__g_pRegister->CTRL = (U32) (Ctrl & 0x7ff);

//	__g_pRegister->CTRL = (U32) (Ctrl & 0x3ff);
	WriteIODW(&__g_pRegister->CTRL, (U32)(Ctrl & 0x3ff));
}

void NX_SPDIF_SetEnable( CBOOL bEnable )
{
	register U32 SetValue;
	register U32 ENABLE_POS = 8;
	NX_ASSERT( CNULL != __g_pRegister );
	NX_ASSERT( CTRUE != bEnable || CFALSE != bEnable );

	SetValue = __g_pRegister->CTRL;
	if( bEnable == CTRUE )	SetValue |= (1<< ENABLE_POS);
	else					SetValue &= ~(1<< ENABLE_POS);
	WriteIODW(&__g_pRegister->CTRL, SetValue);
}

void NX_SPDIF_AssertResetFIFO( void )
{
	register U32 SetValue;
	register const U32 RESETFIFO_POS = 9;
	NX_ASSERT( CNULL != __g_pRegister );

	SetValue = __g_pRegister->CTRL;
	SetValue |= 1<< RESETFIFO_POS;
	WriteIODW(&__g_pRegister->CTRL, SetValue);
}

void NX_SPDIF_NigateResetFIFO( void )
{
	register U32 SetValue;
	register const U32 RESETFIFO_POS = 9;
	NX_ASSERT( CNULL != __g_pRegister );

	SetValue = __g_pRegister->CTRL;
	SetValue &= ~(1<< RESETFIFO_POS);
	WriteIODW(&__g_pRegister->CTRL, SetValue);
}

void NX_SPDIF_SetDataSampling( CBOOL bEnable )
{
	register U32 SetValue;
	register const U32 DATSMP_POS = 0;
	NX_ASSERT( CNULL != __g_pRegister );
	NX_ASSERT( CTRUE != bEnable || CFALSE != bEnable );

	SetValue = __g_pRegister->CTRL;
	if(bEnable == CTRUE)	SetValue |= (1<< DATSMP_POS);
	else					SetValue &= ~(1<< DATSMP_POS);
	WriteIODW(&__g_pRegister->CTRL, SetValue);
}

CBOOL NX_SPDIF_GetDataSampling( void )
{
	register const U32 DATSMP_POS = 0;
	NX_ASSERT( CNULL != __g_pRegister );

	return (__g_pRegister->CTRL & (1<<DATSMP_POS)) ? CTRUE : CFALSE;
}


void NX_SPDIF_SetBITSWAP( CBOOL bEnable )
{
	register U32 SetValue;
	register const U32 BITSWAP_POS = 1;
	NX_ASSERT( CNULL != __g_pRegister );
	NX_ASSERT( CTRUE != bEnable || CFALSE != bEnable );

	SetValue = __g_pRegister->CTRL;
	if(bEnable == CTRUE)	SetValue |= (1<< BITSWAP_POS);
	else					SetValue &= ~(1<< BITSWAP_POS);
	WriteIODW(&__g_pRegister->CTRL, SetValue);
}

CBOOL NX_SPDIF_GetBITSWAP( void )
{
	register const U32 BITSWAP_POS = 1;
	NX_ASSERT( CNULL != __g_pRegister );

	return (__g_pRegister->CTRL & (1<<BITSWAP_POS)) ? CTRUE : CFALSE;
}


void NX_SPDIF_SetDataOnly( CBOOL bEnable )
{
	register U32 SetValue;
	register const U32 DATAONLY_POS = 2;
	NX_ASSERT( CNULL != __g_pRegister );
	NX_ASSERT( CTRUE != bEnable || CFALSE != bEnable );

	SetValue = __g_pRegister->CTRL;
	if(bEnable == CTRUE)	SetValue |= (1<< DATAONLY_POS);
	else					SetValue &= ~(1<< DATAONLY_POS);
	WriteIODW(&__g_pRegister->CTRL, SetValue);
}

CBOOL NX_SPDIF_GetDataOnly( void )
{
	register const U32 DATAONLY_POS = 2;
	NX_ASSERT( CNULL != __g_pRegister );

	return (__g_pRegister->CTRL & (1<<DATAONLY_POS)) ? CTRUE : CFALSE;
}

void NX_SPDIF_CaptureUserState( CBOOL bEnable )
{
	register U32 SetValue;
	register const U32 CAPUSERSTAT_POS = 3;
	NX_ASSERT( CNULL != __g_pRegister );
	NX_ASSERT( CTRUE != bEnable || CFALSE != bEnable );

	SetValue = __g_pRegister->CTRL;
	if(bEnable == CTRUE)	SetValue |= (1<< CAPUSERSTAT_POS);
	else					SetValue &= ~(1<< CAPUSERSTAT_POS);
	WriteIODW(&__g_pRegister->CTRL, SetValue);
}

CBOOL NX_SPDIF_GetCaptureUserState( void )
{
	register const U32 CAPUSERSTAT_POS = 3;
	NX_ASSERT( CNULL != __g_pRegister );

	return (__g_pRegister->CTRL & (1<<CAPUSERSTAT_POS)) ? CTRUE : CFALSE;
}

void NX_SPDIF_SetPhaseDetect( CBOOL bEnable )
{
	register U32 SetValue;
	register const U32 PHASEDET_POS = 8;
	NX_ASSERT( CNULL != __g_pRegister );
	NX_ASSERT( CTRUE != bEnable || CFALSE != bEnable );

	SetValue = __g_pRegister->CTRL;
	if(bEnable == CTRUE)	SetValue |= (1<< PHASEDET_POS);
	else					SetValue &= ~(1<< PHASEDET_POS);
	WriteIODW(&__g_pRegister->CTRL, SetValue);
}


CBOOL NX_SPDIF_GetPhaseDetect( void )
{
	register const U32 PHASEDET_POS = 8;
	NX_ASSERT( CNULL != __g_pRegister );

	return (__g_pRegister->CTRL & (1<<PHASEDET_POS)) ? CTRUE : CFALSE;
}


CBOOL NX_SPDIF_GetLock ( void )
{
	NX_ASSERT( CNULL != __g_pRegister );

	return ((__g_pRegister->CTRL & (0x1<<10)) != 0) ? CTRUE : CFALSE;
}

CBOOL NX_SPDIF_GetClrFIFO ( void )
{
	NX_ASSERT( CNULL != __g_pRegister );

	return ((__g_pRegister->CTRL & 0x1<<9) != 0) ? CTRUE : CFALSE;
}

CBOOL NX_SPDIF_GetEnbPhaseDet ( void )
{
	NX_ASSERT( CNULL != __g_pRegister );

	return ((__g_pRegister->CTRL & 0x1<<8) != 0) ? CTRUE : CFALSE;
}

U8 NX_SPDIF_GetConf_Mode ( void )
{
	NX_ASSERT( CNULL != __g_pRegister );

	return (U8)(__g_pRegister->CTRL & (0xf<<4));
}

CBOOL NX_SPDIF_GetEnbCapUserStat ( void )
{
	NX_ASSERT( CNULL != __g_pRegister );

	return ((__g_pRegister->CTRL & 0x1<<3) != 0) ? CTRUE : CFALSE;
}

CBOOL NX_SPDIF_GetDMA_DataOnly ( void )
{
	NX_ASSERT( CNULL != __g_pRegister );

 	return ((__g_pRegister->CTRL & 0x1<<2) != 0) ? CTRUE : CFALSE;
}

CBOOL NX_SPDIF_GetDMA_Swap ( void )
{
	NX_ASSERT( CNULL != __g_pRegister );

  	return ((__g_pRegister->CTRL & 0x1<<1) != 0) ? CTRUE : CFALSE;
}

CBOOL NX_SPDIF_GetConf_Sample ( void )
{
	NX_ASSERT( CNULL != __g_pRegister );

	return ((__g_pRegister->CTRL & 0x1<<0) != 0) ? CTRUE : CFALSE;
}

CBOOL NX_SPDIF_Get_UserData ( U32 *DataA, U32 *DataB )
{
	NX_ASSERT( CNULL != __g_pRegister );

	if (DataA && DataB) {
		DataA[0] = __g_pRegister->USERA0;	//  2 : 0x08 UserA [ 31:  0]
		DataA[1] = __g_pRegister->USERA1;	//  3 : 0x0C UserA [ 63: 32]
		DataA[2] = __g_pRegister->USERA2;	//  4 : 0x10 UserA [ 95: 64]
		DataA[3] = __g_pRegister->USERA3;	//  5 : 0x14 UserA [127: 96]
		DataA[4] = __g_pRegister->USERA4;	//  6 : 0x18 UserA [159:128]
		DataA[5] = __g_pRegister->USERA5;	//  7 : 0x1C UserA [191:160]

		DataB[0] = __g_pRegister->USERB0;	//  8 : 0x20 UserB [ 31:  0]
		DataB[1] = __g_pRegister->USERB1;	//  9 : 0x24 UserB [ 63: 32]
		DataB[2] = __g_pRegister->USERB2;	// 10 : 0x28 UserB [ 95: 64]
		DataB[3] = __g_pRegister->USERB3;	// 11 : 0x2C UserB [127: 96]
		DataB[4] = __g_pRegister->USERB4;	// 12 : 0x30 UserB [159:128]
		DataB[5] = __g_pRegister->USERB5;	// 13 : 0x34 UserB [191:160]
	} else
		return CFALSE;
	
	return CTRUE;
}

CBOOL NX_SPDIF_Get_Status ( U32 *StatusA, U32 *StatusB )
{
	NX_ASSERT( CNULL != __g_pRegister );

	if (StatusA && StatusB) {
		StatusA[0] = __g_pRegister->STATA0;	// 14 : 0x38 StatA [ 31:  0]
		StatusA[1] = __g_pRegister->STATA1;	// 15 : 0x3C StatA [ 63: 32]
		StatusA[2] = __g_pRegister->STATA2;	// 16 : 0x40 StatA [ 95: 64]
		StatusA[3] = __g_pRegister->STATA3;	// 17 : 0x44 StatA [127: 96]
		StatusA[4] = __g_pRegister->STATA4;	// 18 : 0x48 StatA [159:128]
		StatusA[5] = __g_pRegister->STATA5;	// 19 : 0x4C StatA [191:160]

		StatusB[0] = __g_pRegister->STATB0;	// 20 : 0x50 StatB [ 31:  0]
		StatusB[1] = __g_pRegister->STATB1;	// 21 : 0x54 StatB [ 63: 32]
		StatusB[2] = __g_pRegister->STATB2;	// 22 : 0x58 StatB [ 95: 64]
		StatusB[3] = __g_pRegister->STATB3;	// 23 : 0x5C StatB [127: 96]
		StatusB[4] = __g_pRegister->STATB4;	// 24 : 0x60 StatB [159:128]
		StatusB[5] = __g_pRegister->STATB5;	// 25 : 0x64 StatB [191:160]
	} else
		return CFALSE;
	
	return CTRUE;
}




