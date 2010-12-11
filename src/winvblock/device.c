/**
 * Copyright (C) 2009-2010, Shao Miller <shao.miller@yrdsb.edu.on.ca>.
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
 * Device specifics.
 */

#include <ntddk.h>

#include "winvblock.h"
#include "wv_stdlib.h"
#include "portable.h"
#include "irp.h"
#include "driver.h"
#include "device.h"
#include "debug.h"

/* Forward declarations. */
static device__free_func device__free_dev_;
static device__create_pdo_func device__make_pdo_;

/**
 * Create a new device.
 *
 * @ret dev             The address of a new device, or NULL for failure.
 *
 * This function should not be confused with a PDO creation routine, which is
 * actually implemented for each device type.  This routine will allocate a
 * device__type, track it in a global list, as well as populate the device
 * with default values.
 */
winvblock__lib_func struct device__type * device__create(void) {
    struct device__type * dev;

    /*
     * Devices might be used for booting and should
     * not be allocated from a paged memory pool.
     */
    dev = wv_mallocz(sizeof *dev);
    if (dev == NULL)
      return NULL;
    /* Populate non-zero device defaults. */
    dev->dispatch = driver__default_dispatch;
    dev->DriverObject = driver__obj_ptr;
    dev->ops.create_pdo = device__make_pdo_;
    dev->ops.free = device__free_dev_;

    return dev;
  }

/**
 * Create a device PDO.
 *
 * @v dev               Points to the device that needs a PDO.
 */
winvblock__lib_func PDEVICE_OBJECT STDCALL device__create_pdo(
    IN struct device__type * dev
  ) {
    return dev->ops.create_pdo(dev);
  }

/**
 * Default PDO creation operation.
 *
 * @v dev               Points to the device that needs a PDO.
 * @ret NULL            Reports failure, no matter what.
 *
 * This function does nothing, since it doesn't make sense to create a PDO
 * for an unknown type of device.
 */
static PDEVICE_OBJECT STDCALL device__make_pdo_(IN struct device__type * dev) {
    DBG("No specific PDO creation operation for this device!\n");
    return NULL;
  }

/**
 * Close a device.
 *
 * @v dev               Points to the device to close.
 */
winvblock__lib_func void STDCALL device__close(
    IN struct device__type * dev
  ) {
    /* Call the device's close routine. */
    dev->ops.close(dev);
    return;
  }

/**
 * Delete a device.
 *
 * @v dev               Points to the device to delete.
 */
winvblock__lib_func void STDCALL device__free(IN struct device__type * dev) {
    /* Call the device's free routine. */
    dev->ops.free(dev);
  }

/**
 * Default device deletion operation.
 *
 * @v dev               Points to the device to delete.
 */
static void STDCALL device__free_dev_(IN struct device__type * dev) {
    wv_free(dev);
  }

/**
 * Get a device from a DEVICE_OBJECT.
 *
 * @v dev_obj           Points to the DEVICE_OBJECT to get the device from.
 * @ret                 Returns a pointer to the device on success, else NULL.
 */
winvblock__lib_func struct device__type * device__get(PDEVICE_OBJECT dev_obj) {
    driver__dev_ext_ptr dev_ext = dev_obj->DeviceExtension;
    return dev_ext->device;
  }

/**
 * Set the device for a DEVICE_OBJECT.
 *
 * @v dev_obj           Points to the DEVICE_OBJECT to set the device for.
 * @v dev               Points to the device to associate with.
 */
winvblock__lib_func void device__set(
    PDEVICE_OBJECT dev_obj,
    struct device__type * dev
  ) {
    driver__dev_ext_ptr dev_ext = dev_obj->DeviceExtension;
    dev_ext->device = dev;
    return;
  }
