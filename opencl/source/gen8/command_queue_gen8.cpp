/*
 * Copyright (C) 2018-2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/gen8/hw_cmds.h"
#include "shared/source/helpers/populate_factory.h"
#include "shared/source/memory_manager/unified_memory_manager.h"

#include "opencl/source/command_queue/command_queue_hw.h"
#include "opencl/source/command_queue/command_queue_hw_bdw_and_later.inl"
#include "opencl/source/command_queue/enqueue_resource_barrier.h"

namespace NEO {

typedef Gen8Family Family;
#include "opencl/source/command_queue/command_queue_process_dispatch_for_kernels_instance.inl"
static auto gfxCore = IGFX_GEN8_CORE;
template class CommandQueueHw<Family>;

template <>
void populateFactoryTable<CommandQueueHw<Family>>() {
    extern CommandQueueCreateFunc commandQueueFactory[IGFX_MAX_CORE];
    commandQueueFactory[gfxCore] = CommandQueueHw<Family>::create;
}

} // namespace NEO
