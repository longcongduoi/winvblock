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

/**
 * @file
 *
 * Disk device specifics
 *
 */

#include <ntddk.h>

#include "winvblock.h"
#include "portable.h"
#include "irp.h"
#include "driver.h"
#include "disk.h"
#include "disk_pnp.h"
#include "disk_dev_ctl.h"
#include "disk_scsi.h"
#include "debug.h"

#ifndef _MSC_VER
static long long
__divdi3 (
  long long u,
  long long v
 )
{
  return u / v;
}
#endif

static winvblock__uint32 next_disk = 0;
winvblock__bool disk__removable[disk__media_count] = { TRUE, FALSE, TRUE };
PWCHAR disk__compat_ids[disk__media_count] =
  { L"GenSFloppy", L"GenDisk", L"GenCdRom" };

disk__max_xfer_len_decl ( disk__default_max_xfer_len )
{
  return 1024 * 1024;
}

static
driver__dev_init_decl (
  init
 )
{
  disk__type_ptr disk_ptr = get_disk_ptr ( dev_ext_ptr );
  return disk_ptr->ops->init ( disk_ptr );
}

disk__init_decl ( disk__default_init )
{
  return TRUE;
}

static
irp__handler_decl (
  power
 )
{
  PoStartNextPowerIrp ( Irp );
  Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
  IoCompleteRequest ( Irp, IO_NO_INCREMENT );
  *completion_ptr = TRUE;
  return STATUS_NOT_SUPPORTED;
}

static
irp__handler_decl (
  sys_ctl
 )
{
  NTSTATUS status = Irp->IoStatus.Status;
  IoCompleteRequest ( Irp, IO_NO_INCREMENT );
  *completion_ptr = TRUE;
  return status;
}

static irp__handling handling_table[] = {
  /*
   * Major, minor, any major?, any minor?, handler
   */
  {IRP_MJ_DEVICE_CONTROL, 0, FALSE, TRUE, disk_dev_ctl__dispatch}
  ,
  {IRP_MJ_SYSTEM_CONTROL, 0, FALSE, TRUE, sys_ctl}
  ,
  {IRP_MJ_POWER, 0, FALSE, TRUE, power}
  ,
  {IRP_MJ_SCSI, 0, FALSE, TRUE, disk_scsi__dispatch}
  ,
  {IRP_MJ_PNP, 0, FALSE, TRUE, disk_pnp__simple}
  ,
  {IRP_MJ_PNP, IRP_MN_QUERY_CAPABILITIES, FALSE, FALSE,
   disk_pnp__query_capabilities}
  ,
  {IRP_MJ_PNP, IRP_MN_QUERY_BUS_INFORMATION, FALSE, FALSE,
   disk_pnp__query_bus_info}
  ,
  {IRP_MJ_PNP, IRP_MN_QUERY_DEVICE_RELATIONS, FALSE, FALSE,
   disk_pnp__query_dev_relations}
  ,
  {IRP_MJ_PNP, IRP_MN_QUERY_DEVICE_TEXT, FALSE, FALSE, disk_pnp__query_dev_text}
  ,
  {IRP_MJ_PNP, IRP_MN_QUERY_ID, FALSE, FALSE, disk_pnp__query_id}
};

/**
 * Create a disk PDO filled with the given disk parameters
 *
 * @v dev_ext_ptr     Populate PDO dev. ext. space from these details
 *
 * Returns a Physical Device Object pointer on success, NULL for failure.
 */
static
driver__dev_create_pdo_decl (
  create_pdo
 )
{
  /**
   * @v disk_ptr          Used for pointing to disk details
   * @v status            Status of last operation
   * @v dev_obj_ptr       The new node's device object
   * @v new_ext_size      The extension space size
   * @v disk_types[]      Floppy, hard disk, optical disc specifics
   * @v characteristics[] Floppy, hard disk, optical disc specifics
   */
  disk__type_ptr disk_ptr;
  NTSTATUS status;
  PDEVICE_OBJECT dev_obj_ptr;
  size_t new_ext_size;
  static DEVICE_TYPE disk_types[disk__media_count] =
    { FILE_DEVICE_DISK, FILE_DEVICE_DISK, FILE_DEVICE_CD_ROM };
  static winvblock__uint32 characteristics[disk__media_count] =
    { FILE_REMOVABLE_MEDIA | FILE_FLOPPY_DISKETTE, 0,
    FILE_REMOVABLE_MEDIA | FILE_READ_ONLY_DEVICE
  };

  DBG ( "Entry\n" );
  /*
   * Point to the disk details provided
   */
  disk_ptr = get_disk_ptr ( dev_ext_ptr );
  /*
   * Create the disk device.  Whoever calls us should have set
   * the device extension space size requirement appropriately
   */
  new_ext_size =
    disk_ptr->dev_ext.size + driver__handling_table_size +
    sizeof ( handling_table );
  status =
    IoCreateDevice ( driver__obj_ptr, new_ext_size, NULL,
		     disk_types[disk_ptr->media],
		     FILE_AUTOGENERATED_DEVICE_NAME | FILE_DEVICE_SECURE_OPEN |
		     characteristics[disk_ptr->media], FALSE, &dev_obj_ptr );
  if ( !NT_SUCCESS ( status ) )
    {
      Error ( "IoCreateDevice", status );
      return NULL;
    }
  /*
   * Re-purpose dev_ext_ptr to point into the PDO's device
   * extension space.  We have disk_ptr still for original details
   */
  dev_ext_ptr = dev_obj_ptr->DeviceExtension;
  /*
   * Clear the extension space and establish parameters
   */
  RtlZeroMemory ( dev_ext_ptr, disk_ptr->dev_ext.size );
  /*
   * Copy the provided disk parameters into the disk extension space
   */
  RtlCopyMemory ( dev_ext_ptr, &disk_ptr->dev_ext, disk_ptr->dev_ext.size );
  /*
   * Universal disk properties the caller needn't bother with
   */
  dev_ext_ptr->IsBus = FALSE;
  dev_ext_ptr->Self = dev_obj_ptr;
  dev_ext_ptr->DriverObject = driver__obj_ptr;
  dev_ext_ptr->State = NotStarted;
  dev_ext_ptr->OldState = NotStarted;
  dev_ext_ptr->irp_handler_stack_ptr =
    ( irp__handling_ptr ) ( ( winvblock__uint8 * ) dev_ext_ptr +
			    dev_ext_ptr->size );
  RtlCopyMemory ( dev_ext_ptr->irp_handler_stack_ptr, driver__handling_table,
		  driver__handling_table_size );
  RtlCopyMemory ( ( winvblock__uint8 * ) dev_ext_ptr->irp_handler_stack_ptr +
		  driver__handling_table_size, handling_table,
		  sizeof ( handling_table ) );
  dev_ext_ptr->irp_handler_stack_size =
    ( driver__handling_table_size +
      sizeof ( handling_table ) ) / sizeof ( irp__handling );
  /*
   * Now we update the device extension's size to account for the
   * mini IRP handling stack, in case anyone ever cares
   */
  dev_ext_ptr->size = new_ext_size;
  /*
   * Establish a pointer into the disk device's extension space
   */
  disk_ptr = get_disk_ptr ( dev_ext_ptr );
  KeInitializeEvent ( &disk_ptr->SearchEvent, SynchronizationEvent, FALSE );
  KeInitializeSpinLock ( &disk_ptr->SpinLock );
  disk_ptr->Unmount = FALSE;
  disk_ptr->DiskNumber = InterlockedIncrement ( &next_disk ) - 1;
  /*
   * Some device parameters
   */
  dev_obj_ptr->Flags |= DO_DIRECT_IO;	/* FIXME? */
  dev_obj_ptr->Flags |= DO_POWER_INRUSH;	/* FIXME? */
  return dev_obj_ptr;
}

/* Device operations for disks */
driver__dev_ops disk__dev_ops = {
  create_pdo,
  init
};
