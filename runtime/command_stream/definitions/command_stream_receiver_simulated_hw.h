/*
 * Copyright (C) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "runtime/command_stream/command_stream_receiver_hw.h"
#include "runtime/memory_manager/memory_banks.h"

namespace OCLRT {
class GraphicsAllocation;
template <typename GfxFamily>
class CommandStreamReceiverSimulatedHw : public CommandStreamReceiverHw<GfxFamily> {
    using CommandStreamReceiverHw<GfxFamily>::CommandStreamReceiverHw;

  public:
    uint32_t getMemoryBank(GraphicsAllocation *allocation) const {
        return MemoryBanks::getBank(this->deviceIndex);
    }
};
} // namespace OCLRT
