/**
 * Copyright (C) 2010, Shao Miller <shao.miller@yrdsb.edu.on.ca>.
 *
 * This file is part of WinVBlock, originally derived from WinAoE.
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
 * AoE bus specifics.
 */

#include <stdio.h>
#include <ntddk.h>

#include "winvblock.h"
#include "portable.h"
#include "driver.h"
#include "device.h"
#include "bus.h"
#include "aoe.h"
#include "mount.h"
#include "debug.h"

/* TODO: Remove this pull from aoe/driver.c */
extern device__dispatch_func aoe__scan;
extern device__dispatch_func aoe__show;
extern device__dispatch_func aoe__mount;

/* Forward declarations. */
static device__dev_ctl_func aoe_bus__dev_ctl_dispatch_;
static device__pnp_id_func aoe_bus__pnp_id_;
winvblock__bool aoe_bus__create(void);
void aoe_bus__free(void);

/* Globals. */
WV_SP_BUS_T aoe_bus = NULL;

static NTSTATUS STDCALL aoe_bus__dev_ctl_dispatch_(
    IN struct device__type * dev,
    IN PIRP irp,
    IN ULONG POINTER_ALIGNMENT code
  ) {
    switch(code) {
        case IOCTL_AOE_SCAN:
          return aoe__scan(dev, irp);

        case IOCTL_AOE_SHOW:
          return aoe__show(dev, irp);

        case IOCTL_AOE_MOUNT:
          return aoe__mount(dev, irp);

        case IOCTL_AOE_UMOUNT: {
            WV_SP_BUS_T bus = WvBusFromDev(dev);

            /* Pretend it's an IOCTL_FILE_DETACH. */
            return device__get(bus->LowerDeviceObject)->irp_mj->dev_ctl(
                dev,
                irp,
                IOCTL_FILE_DETACH
              );
          }
        default:
          DBG("Unsupported IOCTL\n");
          return driver__complete_irp(irp, 0, STATUS_NOT_SUPPORTED);
      }
  }

/**
 * Create the AoE bus.
 *
 * @ret         TRUE for success, else FALSE.
 */
winvblock__bool aoe_bus__create(void) {
    WV_SP_BUS_T new_bus;

    /* We should only be called once. */
    if (aoe_bus) {
        DBG("AoE bus already created\n");
        return FALSE;
      }
    /* Try to create the AoE bus. */
    new_bus = WvBusCreate();
    if (!new_bus) {
        DBG("Failed to create AoE bus!\n");
        goto err_new_bus;
      }
    /* When the PDO is created, we need to handle PnP ID queries. */
    new_bus->Dev->ops.pnp_id = aoe_bus__pnp_id_;
    /* Name the bus when the PDO is created. */
    RtlInitUnicodeString(
        &new_bus->dev_name,
        L"\\Device\\AoE"
      );
    RtlInitUnicodeString(
        &new_bus->dos_dev_name,
        L"\\DosDevices\\AoE"
      );
    new_bus->named = TRUE;
    /* Add it as a sub-bus to WinVBlock. */
    if (!WvBusAddChild(driver__bus(), new_bus->Dev)) {
        DBG("Couldn't add AoE bus to WinVBlock bus!\n");
        goto err_add_child;
      }
    /* All done. */
    aoe_bus = new_bus;
    return TRUE;

    err_add_child:

    device__free(new_bus->Dev);
    err_new_bus:

    return FALSE;
  }

/* Destroy the AoE bus. */
void aoe_bus__free(void) {
    if (!aoe_bus)
      /* Nothing to do. */
      return;

    IoDeleteSymbolicLink(&aoe_bus->dos_dev_name);
    IoDeleteSymbolicLink(&aoe_bus->dev_name);
    IoDeleteDevice(aoe_bus->Dev->Self);
    #if 0
    bus__remove_child(driver__bus(), aoe_bus->Dev);
    #endif
    device__free(aoe_bus->Dev);
    return;
  }

static winvblock__uint32 STDCALL aoe_bus__pnp_id_(
    IN struct device__type * dev,
    IN BUS_QUERY_ID_TYPE query_type,
    IN OUT WCHAR (*buf)[512]
  ) {
    switch (query_type) {
        case BusQueryDeviceID:
          return swprintf(*buf, winvblock__literal_w L"\\AoE") + 1;

        case BusQueryInstanceID:
          return swprintf(*buf, L"0") + 1;

        case BusQueryHardwareIDs:
          return swprintf(*buf, winvblock__literal_w L"\\AoE") + 2;

        case BusQueryCompatibleIDs:
          return swprintf(*buf, winvblock__literal_w L"\\AoE") + 4;

        default:
          return 0;
      }
  }