/**
 * Copyright (C) 2009-2011, Shao Miller <shao.miller@yrdsb.edu.on.ca>.
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
#ifndef WV_M_DISK_H_
#  define WV_M_DISK_H_

/**
 * @file
 *
 * Disk device specifics.
 */

typedef enum WVL_DISK_MEDIA_TYPE {
    WvlDiskMediaTypeFloppy,
    WvlDiskMediaTypeHard,
    WvlDiskMediaTypeOptical,
    WvlDiskMediaTypes
  } WVL_E_DISK_MEDIA_TYPE, * WVL_EP_DISK_MEDIA_TYPE;

typedef char WVL_A_DISK_BOOT_SECT[512];
typedef WVL_A_DISK_BOOT_SECT * WVL_AP_DISK_BOOT_SECT;

extern BOOLEAN WvDiskIsRemovable[WvlDiskMediaTypes];
extern PWCHAR WvDiskCompatIds[WvlDiskMediaTypes];

typedef enum WVL_DISK_IO_MODE {
    WvlDiskIoModeRead,
    WvlDiskIoModeWrite,
    WvlDiskIoModes
  } WVL_E_DISK_IO_MODE, * WVL_EP_DISK_IO_MODE;

/* Forward declaration. */
typedef struct WV_DISK_T WV_S_DISK_T, * WV_SP_DISK_T;

/**
 * I/O Request.
 *
 * @v dev_ptr           Points to the disk's device structure.
 * @v mode              Read / write mode.
 * @v start_sector      First sector for request.
 * @v sector_count      Number of sectors to work with.
 * @v buffer            Buffer to read / write sectors to / from.
 * @v irp               Interrupt request packet for this request.
 */
typedef NTSTATUS STDCALL WV_F_DISK_IO(
    IN WV_SP_DEV_T,
    IN WVL_E_DISK_IO_MODE,
    IN LONGLONG,
    IN UINT32,
    IN PUCHAR,
    IN PIRP
  );
typedef WV_F_DISK_IO * WV_FP_DISK_IO;

/**
 * Maximum transfer length response routine.
 *
 * @v disk_ptr        The disk being queried.
 * @ret UINT32        The maximum transfer length.
 */
typedef UINT32 WV_F_DISK_MAX_XFER_LEN(IN WV_SP_DISK_T);
typedef WV_F_DISK_MAX_XFER_LEN * WV_FP_DISK_MAX_XFER_LEN;

/**
 * Disk initialization routine.
 *
 * @v disk_ptr        The disk device being initialized.
 * @ret BOOLEAN       FALSE if initialization failed, otherwise TRUE
 */
typedef BOOLEAN STDCALL WV_F_DISK_INIT(IN WV_SP_DISK_T);
typedef WV_F_DISK_INIT * WV_FP_DISK_INIT;

/**
 * Disk close routine.
 *
 * @v disk_ptr        The disk device being closed.
 */
typedef VOID STDCALL WV_F_DISK_CLOSE(IN WV_SP_DISK_T);
typedef WV_F_DISK_CLOSE * WV_FP_DISK_CLOSE;

typedef struct WV_DISK_OPS {
    WV_FP_DISK_IO Io;
    WV_FP_DISK_MAX_XFER_LEN MaxXferLen;
    WV_FP_DISK_INIT Init;
    WV_FP_DISK_CLOSE Close;
  } WV_S_DISK_OPS, * WV_SP_DISK_OPS;

struct WV_DISK_T {
    WV_S_DEV_T Dev[1];
    KEVENT SearchEvent;
    KSPIN_LOCK SpinLock;
    WVL_E_DISK_MEDIA_TYPE Media;
    WV_S_DISK_OPS disk_ops;
    ULONGLONG LBADiskSize;
    ULONGLONG Cylinders;
    UINT32 Heads;
    UINT32 Sectors;
    UINT32 SectorSize;
    UINT32 SpecialFileCount;
    LIST_ENTRY tracking;
    PVOID ext;
    PDRIVER_OBJECT DriverObj;
  };

/* Yield a pointer to the disk. */
#define disk__get_ptr(dev_ptr) ((WV_SP_DISK_T) dev_ptr->ext)

/* An MBR C/H/S address and ways to access its components. */
typedef UCHAR chs[3];

#define     chs_head(chs) chs[0]
#define   chs_sector(chs) (chs[1] & 0x3F)
#define chs_cyl_high(chs) (((UINT16) (chs[1] & 0xC0)) << 2)
#define  chs_cyl_low(chs) ((UINT16) chs[2])
#define chs_cylinder(chs) (chs_cyl_high(chs) | chs_cyl_low(chs))

/* An MBR. */
#ifdef _MSC_VER
#  pragma pack(1)
#endif
struct WVL_DISK_MBR {
    UCHAR code[440];
    UINT32 disk_sig;
    UINT16 pad;
    struct {
        UCHAR status;
        chs chs_start;
        UCHAR type;
        chs chs_end;
        UINT32 lba_start;
        UINT32 lba_count;
      } partition[4] __attribute__((packed));
    UINT16 mbr_sig;
  } __attribute__((__packed__));
typedef struct WVL_DISK_MBR WVL_S_DISK_MBR, * WVL_SP_DISK_MBR;
#ifdef _MSC_VER
#  pragma pack()
#endif

extern WV_F_DISK_IO disk__io;
extern WV_F_DISK_MAX_XFER_LEN disk__max_xfer_len;

/**
 * Attempt to guess a disk's geometry.
 *
 * @v boot_sect_ptr     The MBR or VBR with possible geometry clues.
 * @v disk_ptr          The disk to set the geometry for.
 */
extern WVL_M_LIB VOID disk__guess_geometry(
    IN WVL_AP_DISK_BOOT_SECT,
    IN OUT WV_SP_DISK_T disk_ptr
  );

extern WVL_M_LIB NTSTATUS STDCALL WvlDiskCreatePdo(
    IN PDRIVER_OBJECT,
    IN SIZE_T,
    IN WVL_E_DISK_MEDIA_TYPE,
    OUT PDEVICE_OBJECT *
  );
extern WVL_M_LIB VOID STDCALL WvDiskInit(IN WV_SP_DISK_T);
extern WVL_M_LIB WV_SP_DISK_T disk__create(void);

/* IRP-related. */
extern WV_F_DEV_DISPATCH WvDiskIrpPower;
extern WV_F_DEV_DISPATCH WvDiskIrpSysCtl;
/* IRP_MJ_DEVICE_CONTROL dispatcher from disk/dev_ctl.c */
extern WV_F_DEV_CTL disk_dev_ctl__dispatch;
/* IRP_MJ_SCSI dispatcher from disk/scsi.c */
extern WV_F_DEV_SCSI disk_scsi__dispatch;
/* IRP_MJ_PNP dispatcher from disk/pnp.c */
extern WV_F_DEV_PNP disk_pnp__dispatch;


#endif  /* WV_M_DISK_H_ */
