/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "shared/source/execution_environment/execution_environment.h"

#include "level_zero/core/source/device/device.h"
#include "level_zero/sysman/source/engine/engine.h"
#include "level_zero/sysman/source/fabric_port/fabric_port.h"
#include "level_zero/sysman/source/firmware/firmware.h"
#include "level_zero/sysman/source/frequency/frequency.h"
#include "level_zero/sysman/source/memory/memory.h"
#include "level_zero/sysman/source/power/power.h"
#include "level_zero/sysman/source/scheduler/scheduler.h"
#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>

namespace L0 {
namespace Sysman {

struct SysmanDevice : _ze_device_handle_t {
    static SysmanDevice *fromHandle(zes_device_handle_t handle) { return static_cast<SysmanDevice *>(handle); }
    inline zes_device_handle_t toHandle() { return this; }
    virtual ~SysmanDevice() = default;
    static SysmanDevice *create(NEO::ExecutionEnvironment &executionEnvironment, const uint32_t rootDeviceIndex);
    virtual const NEO::HardwareInfo &getHardwareInfo() const = 0;

    static ze_result_t powerGet(zes_device_handle_t hDevice, uint32_t *pCount, zes_pwr_handle_t *phPower);
    virtual ze_result_t powerGet(uint32_t *pCount, zes_pwr_handle_t *phPower) = 0;
    static ze_result_t powerGetCardDomain(zes_device_handle_t hDevice, zes_pwr_handle_t *phPower);
    virtual ze_result_t powerGetCardDomain(zes_pwr_handle_t *phPower) = 0;
    static ze_result_t fabricPortGet(zes_device_handle_t hDevice, uint32_t *pCount, zes_fabric_port_handle_t *phPort);
    virtual ze_result_t fabricPortGet(uint32_t *pCount, zes_fabric_port_handle_t *phPort) = 0;
    static ze_result_t memoryGet(zes_device_handle_t hDevice, uint32_t *pCount, zes_mem_handle_t *phMemory);
    virtual ze_result_t memoryGet(uint32_t *pCount, zes_mem_handle_t *phMemory) = 0;

    static ze_result_t engineGet(zes_device_handle_t hDevice, uint32_t *pCount, zes_engine_handle_t *phEngine);
    virtual ze_result_t engineGet(uint32_t *pCount, zes_engine_handle_t *phEngine) = 0;

    static ze_result_t frequencyGet(zes_device_handle_t hDevice, uint32_t *pCount, zes_freq_handle_t *phFrequency);
    virtual ze_result_t frequencyGet(uint32_t *pCount, zes_freq_handle_t *phFrequency) = 0;

    static ze_result_t schedulerGet(zes_device_handle_t hDevice, uint32_t *pCount, zes_sched_handle_t *phScheduler);
    virtual ze_result_t schedulerGet(uint32_t *pCount, zes_sched_handle_t *phScheduler) = 0;

    static ze_result_t firmwareGet(zes_device_handle_t hDevice, uint32_t *pCount, zes_firmware_handle_t *phFirmware);
    virtual ze_result_t firmwareGet(uint32_t *pCount, zes_firmware_handle_t *phFirmware) = 0;
};

} // namespace Sysman
} // namespace L0
