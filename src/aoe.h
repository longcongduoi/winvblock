/**
 * Copyright (C) 2009, Shao Miller <shao.miller@yrdsb.edu.on.ca>.
 * Copyright 2006-2008, V.
 * For WinAoE contact information, see http://winaoe.org/
 *
 * This file is part of WinVBlock, derived from WinAoE.
 *
 * WinVBlock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * WinVBlock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WinVBlock.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _AOE_H
#  define _AOE_H

/**
 * @file
 *
 * AoE specifics
 *
 */

#  include "portable.h"
#  include "driver.h"

#  define htons(x) (winvblock__uint16)((((x) << 8) & 0xff00) | (((x) >> 8) & 0xff))
#  define ntohs(x) (winvblock__uint16)((((x) << 8) & 0xff00) | (((x) >> 8) & 0xff))

typedef enum
{ AoE_RequestMode_Read, AoE_RequestMode_Write } AOE_REQUESTMODE,
*PAOE_REQUESTMODE;

extern winvblock__bool STDCALL AoE_SearchDrive (
	IN PDRIVER_DEVICEEXTENSION DeviceExtension
 );
extern NTSTATUS STDCALL AoE_Request (
	IN PDRIVER_DEVICEEXTENSION DeviceExtension,
	IN AOE_REQUESTMODE Mode,
	IN LONGLONG StartSector,
	IN ULONG SectorCount,
	IN winvblock__uint8_ptr Buffer,
	IN PIRP Irp
 );
extern NTSTATUS STDCALL AoE_Reply (
	IN winvblock__uint8_ptr SourceMac,
	IN winvblock__uint8_ptr DestinationMac,
	IN winvblock__uint8_ptr Data,
	IN winvblock__uint32 DataSize
 );
extern VOID STDCALL AoE_ResetProbe (
	void
 );
extern NTSTATUS STDCALL AoE_Start (
	void
 );
extern VOID STDCALL AoE_Stop (
	void
 );

#endif													/* _AOE_H */
