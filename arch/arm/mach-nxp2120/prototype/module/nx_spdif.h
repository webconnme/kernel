/*******************************************************************************
	Copyright (C) 2009 NEXELL Co., All Rights Reserved
	NEXELL Co. Proprietary & Confidential

	project		: LF2000, DTV
	file name	: nx_spdif.h
	description	: spdif prototype header
	coded by	: Charles Park
................................................................................
	modification history
	2010.10.18	: Hans Han			change to c code
	2009.12.28	: Charles Park		1st version
*******************************************************************************/

#ifndef __NX_SPDIF_H__
#define __NX_SPDIF_H__

//------------------------------------------------------------------------------
//  Includes
//------------------------------------------------------------------------------
#include "../nx_base/nx_prototype.h"

#ifdef	__cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
//  SPDIF Register Set Structure
//------------------------------------------------------------------------------
struct NX_SPDIF_RegisterSet
{
    //--------------------------------------------------------------------------
    // SPDIF - Base Address : 0xc0011800
    //--------------------------------------------------------------------------
	volatile U32 CTRL;		//  0 : 0x00 {16'h0, lock, Clr_FIFO, EnbPhaseDet, conf_mode, EnbCapUserStat, DMA_DataOnly, DMA_Swap, conf_sample};                                        
    volatile U32 ENBIRQ;	//  1 : 0x04 {16'h0, 7'h0, PendLock, PendErr, PendParity, PendBlock, EnbIRQLock, EnbIRQErr, EnbIRQParity, EnbIRQBlock};
    volatile U32 USERA0;	//  2 : 0x08 UserA [ 31:  0]
    volatile U32 USERA1;	//  3 : 0x0C UserA [ 63: 32]
    volatile U32 USERA2;	//  4 : 0x10 UserA [ 95: 64]
    volatile U32 USERA3;	//  5 : 0x14 UserA [127: 96]
    volatile U32 USERA4;	//  6 : 0x18 UserA [159:128]
    volatile U32 USERA5;	//  7 : 0x1C UserA [191:160]
    volatile U32 USERB0;	//  8 : 0x20 UserB [ 31:  0]
    volatile U32 USERB1;	//  9 : 0x24 UserB [ 63: 32]
    volatile U32 USERB2;	// 10 : 0x28 UserB [ 95: 64]
    volatile U32 USERB3;	// 11 : 0x2C UserB [127: 96]
    volatile U32 USERB4;	// 12 : 0x30 UserB [159:128]
    volatile U32 USERB5;	// 13 : 0x34 UserB [191:160]
    volatile U32 STATA0;	// 14 : 0x38 StatA [ 31:  0]
    volatile U32 STATA1;	// 15 : 0x3C StatA [ 63: 32]
    volatile U32 STATA2;	// 16 : 0x40 StatA [ 95: 64]
    volatile U32 STATA3;	// 17 : 0x44 StatA [127: 96]
    volatile U32 STATA4;	// 18 : 0x48 StatA [159:128]
    volatile U32 STATA5;	// 19 : 0x4C StatA [191:160]
    volatile U32 STATB0;	// 20 : 0x50 StatB [ 31:  0]
    volatile U32 STATB1;	// 21 : 0x54 StatB [ 63: 32]
    volatile U32 STATB2;	// 22 : 0x58 StatB [ 95: 64]
    volatile U32 STATB3;	// 23 : 0x5C StatB [127: 96]
    volatile U32 STATB4;	// 24 : 0x60 StatB [159:128]
    volatile U32 STATB5;	// 25 : 0x64 StatB [191:160]
};

    //@}
    /// @brief  IRQ numbers
    enum IRQ 
    {
		IRQ_BLOCK		= 0,
		IRQ_PARITY		= 1,
		IRQ_ERROR		= 2,
		IRQ_LOCK		= 3	
    };  
    //@}

//------------------------------------------------------------------------------
/// @name	Module Interface
//@{
CBOOL	NX_SPDIF_Initialize( void );
U32		NX_SPDIF_GetNumberOfModule( void );
//@}


//------------------------------------------------------------------------------
///	@name	Basic Interface
//@{
U32		NX_SPDIF_GetPhysicalAddress( void );
U32		NX_SPDIF_GetSizeOfRegisterSet( void );
void	NX_SPDIF_SetBaseAddress( U32 BaseAddress );
U32		NX_SPDIF_GetBaseAddress( void );
CBOOL	NX_SPDIF_OpenModule( void );
CBOOL	NX_SPDIF_CloseModule( void );
CBOOL	NX_SPDIF_CheckBusy( void );
CBOOL	NX_SPDIF_CanPowerDown( void );
//@}

//------------------------------------------------------------------------------
///	@name	Interrupt Interface
//@{
S32		NX_SPDIF_GetInterruptNumber( void );

void	NX_SPDIF_SetInterruptEnable( S32 IntNum, CBOOL Enable );
CBOOL	NX_SPDIF_GetInterruptEnable( S32 IntNum );
CBOOL	NX_SPDIF_GetInterruptPending( S32 IntNum );
void	NX_SPDIF_ClearInterruptPending( S32 IntNum );

void	NX_SPDIF_SetInterruptEnableAll( CBOOL Enable );
CBOOL	NX_SPDIF_GetInterruptEnableAll( void );
CBOOL	NX_SPDIF_GetInterruptPendingAll( void );
void	NX_SPDIF_ClearInterruptPendingAll( void );

void	NX_SPDIF_SetInterruptEnable32( U32 EnableFlag );
U32		NX_SPDIF_GetInterruptEnable32( void );
U32		NX_SPDIF_GetInterruptPending32( void );
void	NX_SPDIF_ClearInterruptPending32( U32 PendingFlag );

S32		NX_SPDIF_GetInterruptPendingNumber( void );	// -1 if None
//@}
//--------------------------------------------------------------------------
/// @name   DMA interface
//--------------------------------------------------------------------------
//@{
U32		NX_SPDIF_GetDMABusWidth ( void );
U32		NX_SPDIF_GetDMABaseAddress ( void );
U32		NX_SPDIF_GetDMAIndex ( void );
//@}

//--------------------------------------------------------------------------
/// @name   SPDIF Function Declaration
//--------------------------------------------------------------------------
//@{

void	NX_SPDIF_SetPhaseDetect( CBOOL bEnable );

void	NX_SPDIF_SetCtrl ( U32 SPDIF_CTRL );
void 	NX_SPDIF_SetEnable( CBOOL bEnable );

CBOOL	NX_SPDIF_GetLock ( void );
CBOOL	NX_SPDIF_GetClrFIFO ( void );
CBOOL	NX_SPDIF_GetEnbPhaseDet ( void );
U8		NX_SPDIF_GetConf_Mode ( void );
CBOOL	NX_SPDIF_GetEnbCapUserStat ( void );
CBOOL	NX_SPDIF_GetDMA_DataOnly ( void );
CBOOL	NX_SPDIF_GetDMA_Swap ( void );
CBOOL	NX_SPDIF_GetConf_Sample ( void );
void NX_SPDIF_SetDataSampling( CBOOL bEnable );
CBOOL NX_SPDIF_GetDataSampling( void );
void NX_SPDIF_CaptureUserState( CBOOL bEnable );
void NX_SPDIF_NigateResetFIFO(void);
void NX_SPDIF_SetDataOnly(CBOOL bEnable);

void NX_SPDIF_AssertResetFIFO( void );

CBOOL NX_SPDIF_Get_UserData ( U32 *DataA, U32 *DataB );
CBOOL NX_SPDIF_Get_Status ( U32 *StatusA, U32 *StatusB );

#ifdef	__cplusplus
}
#endif

#endif // __NX_SPDIF_H__
