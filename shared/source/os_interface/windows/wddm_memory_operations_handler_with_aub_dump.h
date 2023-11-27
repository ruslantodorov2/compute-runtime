/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "shared/source/aub/aub_center.h"
#include "shared/source/command_stream/command_stream_receiver.h"
#include "shared/source/execution_environment/execution_environment.h"
#include "shared/source/execution_environment/root_device_environment.h"
#include "shared/source/helpers/gfx_core_helper.h"
#include "shared/source/os_interface/aub_memory_operations_handler.h"

namespace NEO {
class Wddm;

template <typename BaseOperationsHandler>
class WddmMemoryOperationsHandlerWithAubDump : public BaseOperationsHandler {
  public:
    WddmMemoryOperationsHandlerWithAubDump(Wddm *wddm, RootDeviceEnvironment &rootDeviceEnvironment)
        : BaseOperationsHandler(wddm) {

        if (!rootDeviceEnvironment.aubCenter) {
            auto &gfxCoreHelper = rootDeviceEnvironment.getHelper<GfxCoreHelper>();
            auto hardwareInfo = rootDeviceEnvironment.getMutableHardwareInfo();
            auto localMemoryEnabled = gfxCoreHelper.getEnableLocalMemory(*hardwareInfo);
            rootDeviceEnvironment.initGmm();
            rootDeviceEnvironment.initAubCenter(localMemoryEnabled, "", static_cast<CommandStreamReceiverType>(CommandStreamReceiverType::CSR_HW_WITH_AUB));
        }

        const auto aubCenter = rootDeviceEnvironment.aubCenter.get();
        aubMemoryOperationsHandler = std::make_unique<AubMemoryOperationsHandler>(aubCenter->getAubManager());
    };

    ~WddmMemoryOperationsHandlerWithAubDump() override = default;

    MemoryOperationsStatus makeResident(Device *device, ArrayRef<GraphicsAllocation *> gfxAllocations) override {
        aubMemoryOperationsHandler->makeResident(device, gfxAllocations);
        return BaseOperationsHandler::makeResident(device, gfxAllocations);
    }

    MemoryOperationsStatus evict(Device *device, GraphicsAllocation &gfxAllocation) override {
        aubMemoryOperationsHandler->evict(device, gfxAllocation);
        return BaseOperationsHandler::evict(device, gfxAllocation);
    }

    MemoryOperationsStatus isResident(Device *device, GraphicsAllocation &gfxAllocation) override {
        aubMemoryOperationsHandler->isResident(device, gfxAllocation);
        return BaseOperationsHandler::isResident(device, gfxAllocation);
    }

    MemoryOperationsStatus makeResidentWithinOsContext(OsContext *osContext, ArrayRef<GraphicsAllocation *> gfxAllocations, bool evictable) override {
        aubMemoryOperationsHandler->makeResidentWithinOsContext(osContext, gfxAllocations, evictable);
        return BaseOperationsHandler::makeResidentWithinOsContext(osContext, gfxAllocations, evictable);
    }

    MemoryOperationsStatus evictWithinOsContext(OsContext *osContext, GraphicsAllocation &gfxAllocation) override {
        aubMemoryOperationsHandler->evictWithinOsContext(osContext, gfxAllocation);
        return BaseOperationsHandler::evictWithinOsContext(osContext, gfxAllocation);
    }

  protected:
    std::unique_ptr<AubMemoryOperationsHandler> aubMemoryOperationsHandler;
};
} // namespace NEO
