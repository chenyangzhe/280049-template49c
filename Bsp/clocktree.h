//#############################################################################
//
// FILE:   clockTree.h
//
// TITLE:  Setups device clocking for examples.
//
//#############################################################################
// $Copyright:
// Copyright (C) 2026 Texas Instruments Incorporated - http://www.ti.com
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//###########################################################################

#ifndef CLOCKTREE_H
#define CLOCKTREE_H

//*****************************************************************************
//
// Summary of SYSPLL related clock configuration
//
//*****************************************************************************
//
// Input Clock to SYSPLL (OSCCLK)	= 20 MHz    (XTAL provides OSCCLK)
//
//##### SYSPLL ENABLED #####
//
// PLLRAWCLK				= 100 MHz   (Output of SYSPLL if enabled) 
// PLLSYSCLK				= 100 MHz
// CPUCLK					= 100 MHz
// SYSCLK					= 100 MHz
// LSPCLK					= 100 MHz 
	
//*****************************************************************************
//
// Macro definitions used in device.c (SYSPLL / LSPCLK)
//
//*****************************************************************************
//
//	Input Clock to SYSPLL (OSCCLK) = XTAL = 20 MHz
//
#define DEVICE_OSCSRC_FREQ          20000000U
//
// Define to pass to SysCtl_setClock(). Will configure the clock as follows:
// SYSPLL ENABLED
// SYSCLK = 100 MHz = 20 MHz (OSCCLK) * (10 (IMULT) + 0 (FMULT)) / (2 (ODIV) * 1 (SYSCLKDIVSEL))	
#define DEVICE_SYSCLK_FREQ          ((DEVICE_OSCSRC_FREQ * (10 + 0)) / (2 * 1))
//
#define DEVICE_SETCLOCK_CFG         (SYSCTL_OSCSRC_XTAL | SYSCTL_IMULT(10) | \
									 SYSCTL_FMULT_NONE | SYSCTL_ODIV(2)  | \
									 SYSCTL_SYSDIV(1)| SYSCTL_PLL_ENABLE)

//
// Define to pass to SysCtl_setLowSpeedClock().
// Low Speed Clock (LSPCLK) = 100 MHz / 1 = 100 MHz
//
#define DEVICE_LSPCLK_CFG  			SYSCTL_LSPCLK_PRESCALE_1

#define DEVICE_LSPCLK_FREQ          (DEVICE_SYSCLK_FREQ / 1)

//*****************************************************************************
//
// CPUCLK Domain (100 MHz)
//
//*****************************************************************************
// C28x
// FPU
// TMU
// VCU
// Flash
// M0_M1_RAM
// BOOTROM
// EPIE
// LocalSharedRAMs
// GlobalSharedRAMs
// GPIO
// DCSM
//	
//*****************************************************************************
//
// SYSCLK Domain (100 MHz)
//
//*****************************************************************************
// EPIE
// LocalSharedRAMs
// GlobalSharedRAMs
// GPIO
// DCSM
//	
/////////////////////	
// Gated CPU1 SYSCLK
/////////////////////
// CLA
// Timer0
// DMA
// ePWM
// eCAP
// eQEP
// SDFM
// ADC
// CMPSS
// DAC
// PGA
// I2C
// PMBus
// LIN
//

//*****************************************************************************
//
// Gated Peripheral SYSCLK Domain (100 MHz) 
//
//*****************************************************************************
// CLA
// Timer0
// DMA
// ePWM
// eCAP
// eQEP
// SDFM
// ADC
// CMPSS
// DAC
// PGA
// I2C
// PMBus
// LIN
//	
//*****************************************************************************
//
// Gated LSPCLK Domain (100 MHz) 
//
//*****************************************************************************
// SCI
// SPI

#endif // CLOCKTREE_H

