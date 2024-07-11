/*
 * Copyright (C) 2019-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/helpers/blit_commands_helper.h"
#include "shared/source/utilities/wait_util.h"

#include "opencl/source/built_ins/aux_translation_builtin.h"
#include "opencl/source/command_queue/enqueue_barrier.h"
#include "opencl/source/command_queue/enqueue_copy_buffer.h"
#include "opencl/source/command_queue/enqueue_copy_buffer_rect.h"
#include "opencl/source/command_queue/enqueue_copy_buffer_to_image.h"
#include "opencl/source/command_queue/enqueue_copy_image.h"
#include "opencl/source/command_queue/enqueue_copy_image_to_buffer.h"
#include "opencl/source/command_queue/enqueue_fill_buffer.h"
#include "opencl/source/command_queue/enqueue_fill_image.h"
#include "opencl/source/command_queue/enqueue_kernel.h"
#include "opencl/source/command_queue/enqueue_marker.h"
#include "opencl/source/command_queue/enqueue_migrate_mem_objects.h"
#include "opencl/source/command_queue/enqueue_read_buffer.h"
#include "opencl/source/command_queue/enqueue_read_buffer_rect.h"
#include "opencl/source/command_queue/enqueue_read_image.h"
#include "opencl/source/command_queue/enqueue_svm.h"
#include "opencl/source/command_queue/enqueue_write_buffer.h"
#include "opencl/source/command_queue/enqueue_write_buffer_rect.h"
#include "opencl/source/command_queue/enqueue_write_image.h"
#include "opencl/source/command_queue/finish.h"
#include "opencl/source/command_queue/flush.h"
#include "opencl/source/command_queue/gpgpu_walker.h"

namespace NEO {
template <typename Family>
void CommandQueueHw<Family>::notifyEnqueueReadBuffer(Buffer *buffer, bool blockingRead, bool notifyBcsCsr) {
    if (debugManager.flags.AUBDumpAllocsOnEnqueueReadOnly.get()) {
        buffer->getGraphicsAllocation(getDevice().getRootDeviceIndex())->setAllocDumpable(blockingRead, notifyBcsCsr);
        buffer->forceDisallowCPUCopy = blockingRead;
    }
}
template <typename Family>
void CommandQueueHw<Family>::notifyEnqueueReadImage(Image *image, bool blockingRead, bool notifyBcsCsr) {
    if (debugManager.flags.AUBDumpAllocsOnEnqueueReadOnly.get()) {
        image->getGraphicsAllocation(getDevice().getRootDeviceIndex())->setAllocDumpable(blockingRead, notifyBcsCsr);
    }
}

template <typename Family>
void CommandQueueHw<Family>::notifyEnqueueSVMMemcpy(GraphicsAllocation *gfxAllocation, bool blockingCopy, bool notifyBcsCsr) {
    if (debugManager.flags.AUBDumpAllocsOnEnqueueSVMMemcpyOnly.get()) {
        gfxAllocation->setAllocDumpable(blockingCopy, notifyBcsCsr);
    }
}

template <typename Family>
cl_int CommandQueueHw<Family>::enqueueReadWriteBufferOnCpuWithMemoryTransfer(cl_command_type commandType, Buffer *buffer,
                                                                             size_t offset, size_t size, void *ptr, cl_uint numEventsInWaitList,
                                                                             const cl_event *eventWaitList, cl_event *event) {
    cl_int retVal = CL_SUCCESS;
    EventsRequest eventsRequest(numEventsInWaitList, eventWaitList, event);

    TransferProperties transferProperties(buffer, commandType, 0, true, &offset, &size, ptr, true, getDevice().getRootDeviceIndex());
    cpuDataTransferHandler(transferProperties, eventsRequest, retVal);
    return retVal;
}

template <typename Family>
cl_int CommandQueueHw<Family>::enqueueReadWriteBufferOnCpuWithoutMemoryTransfer(cl_command_type commandType, Buffer *buffer,
                                                                                size_t offset, size_t size, void *ptr, cl_uint numEventsInWaitList,
                                                                                const cl_event *eventWaitList, cl_event *event) {
    cl_int retVal = CL_SUCCESS;
    EventsRequest eventsRequest(numEventsInWaitList, eventWaitList, event);

    TransferProperties transferProperties(buffer, CL_COMMAND_MARKER, 0, true, &offset, &size, ptr, false, getDevice().getRootDeviceIndex());
    cpuDataTransferHandler(transferProperties, eventsRequest, retVal);
    if (event) {
        auto pEvent = castToObjectOrAbort<Event>(*event);
        pEvent->setCmdType(commandType);
    }

    if (context->isProvidingPerformanceHints()) {
        context->providePerformanceHintForMemoryTransfer(commandType, false, static_cast<cl_mem>(buffer), ptr);
    }
    return retVal;
}

template <typename Family>
cl_int CommandQueueHw<Family>::enqueueMarkerForReadWriteOperation(MemObj *memObj, void *ptr, cl_command_type commandType, cl_bool blocking, cl_uint numEventsInWaitList,
                                                                  const cl_event *eventWaitList, cl_event *event) {
    MultiDispatchInfo multiDispatchInfo;
    NullSurface s;
    Surface *surfaces[] = {&s};
    const auto enqueueResult = enqueueHandler<CL_COMMAND_MARKER>(
        surfaces,
        blocking == CL_TRUE,
        multiDispatchInfo,
        numEventsInWaitList,
        eventWaitList,
        event);

    if (enqueueResult != CL_SUCCESS) {
        return enqueueResult;
    }

    if (event) {
        auto pEvent = castToObjectOrAbort<Event>(*event);
        pEvent->setCmdType(commandType);
    }

    if (context->isProvidingPerformanceHints()) {
        context->providePerformanceHintForMemoryTransfer(commandType, false, static_cast<cl_mem>(memObj), ptr);
    }

    return CL_SUCCESS;
}

template <typename Family>
void CommandQueueHw<Family>::dispatchAuxTranslationBuiltin(MultiDispatchInfo &multiDispatchInfo,
                                                           AuxTranslationDirection auxTranslationDirection) {
    auto &builder = BuiltInDispatchBuilderOp::getBuiltinDispatchInfoBuilder(EBuiltInOps::auxTranslation, getClDevice());
    auto &auxTranslationBuilder = static_cast<BuiltInOp<EBuiltInOps::auxTranslation> &>(builder);
    BuiltinOpParams dispatchParams;

    dispatchParams.auxTranslationDirection = auxTranslationDirection;

    auxTranslationBuilder.buildDispatchInfosForAuxTranslation<Family>(multiDispatchInfo, dispatchParams);
}

template <typename Family>
bool CommandQueueHw<Family>::forceStateless(size_t size) {
    return size >= 4ull * MemoryConstants::gigaByte;
}

template <typename Family>
bool CommandQueueHw<Family>::isCacheFlushForBcsRequired() const {
    if (debugManager.flags.ForceCacheFlushForBcs.get() != -1) {
        return !!debugManager.flags.ForceCacheFlushForBcs.get();
    }
    return true;
}

template <typename TSPacketType>
inline bool waitForTimestampsWithinContainer(TimestampPacketContainer *container, CommandStreamReceiver &csr, WaitStatus &status) {
    bool waited = false;
    status = WaitStatus::notReady;

    if (container) {
        auto lastHangCheckTime = std::chrono::high_resolution_clock::now();
        for (const auto &timestamp : container->peekNodes()) {
            for (uint32_t i = 0; i < timestamp->getPacketsUsed(); i++) {
                while (timestamp->getContextEndValue(i) == 1) {
                    csr.downloadAllocation(*timestamp->getBaseGraphicsAllocation()->getGraphicsAllocation(csr.getRootDeviceIndex()));
                    WaitUtils::waitFunctionWithPredicate<const TSPacketType>(static_cast<TSPacketType const *>(timestamp->getContextEndAddress(i)), 1u, std::not_equal_to<TSPacketType>());
                    if (csr.checkGpuHangDetected(std::chrono::high_resolution_clock::now(), lastHangCheckTime)) {
                        status = WaitStatus::gpuHang;
                        return false;
                    }
                }
                status = WaitStatus::ready;
                waited = true;
            }
        }
    }

    return waited;
}

template <typename Family>
bool CommandQueueHw<Family>::waitForTimestamps(Range<CopyEngineState> copyEnginesToWait, WaitStatus &status, TimestampPacketContainer *mainContainer, TimestampPacketContainer *deferredContainer) {
    using TSPacketType = typename Family::TimestampPacketType;
    bool waited = false;

    if (isWaitForTimestampsEnabled()) {
        {
            TakeOwnershipWrapper<CommandQueue> queueOwnership(*this);
            waited = waitForTimestampsWithinContainer<TSPacketType>(mainContainer, getGpgpuCommandStreamReceiver(), status);
        }

        if (waited) {
            getGpgpuCommandStreamReceiver().downloadAllocations();
            for (const auto &copyEngine : copyEnginesToWait) {
                auto bcsCsr = getBcsCommandStreamReceiver(copyEngine.engineType);
                bcsCsr->downloadAllocations();
            }
        }
    }

    return waited;
}

template <typename Family>
void CommandQueueHw<Family>::setupBlitAuxTranslation(MultiDispatchInfo &multiDispatchInfo) {
    multiDispatchInfo.begin()->dispatchInitCommands.registerMethod(
        TimestampPacketHelper::programSemaphoreForAuxTranslation<Family, AuxTranslationDirection::auxToNonAux>);

    multiDispatchInfo.begin()->dispatchInitCommands.registerCommandsSizeEstimationMethod(
        TimestampPacketHelper::getRequiredCmdStreamSizeForAuxTranslationNodeDependency<Family, AuxTranslationDirection::auxToNonAux>);

    multiDispatchInfo.rbegin()->dispatchEpilogueCommands.registerMethod(
        TimestampPacketHelper::programSemaphoreForAuxTranslation<Family, AuxTranslationDirection::nonAuxToAux>);

    multiDispatchInfo.rbegin()->dispatchEpilogueCommands.registerCommandsSizeEstimationMethod(
        TimestampPacketHelper::getRequiredCmdStreamSizeForAuxTranslationNodeDependency<Family, AuxTranslationDirection::nonAuxToAux>);
}

template <typename Family>
bool CommandQueueHw<Family>::isGpgpuSubmissionForBcsRequired(bool queueBlocked, TimestampPacketDependencies &timestampPacketDependencies) const {
    if (queueBlocked || timestampPacketDependencies.barrierNodes.peekNodes().size() > 0u) {
        return true;
    }
    bool required = false;
    switch (latestSentEnqueueType) {
    case NEO::EnqueueProperties::Operation::explicitCacheFlush:
    case NEO::EnqueueProperties::Operation::enqueueWithoutSubmission:
    case NEO::EnqueueProperties::Operation::gpuKernel:
    case NEO::EnqueueProperties::Operation::profilingOnly:
        required = isCacheFlushForBcsRequired() || !(getGpgpuCommandStreamReceiver().getDispatchMode() == DispatchMode::immediateDispatch || getGpgpuCommandStreamReceiver().isLatestTaskCountFlushed());
        break;
    case NEO::EnqueueProperties::Operation::dependencyResolveOnGpu:
        return true;
        break;
    case NEO::EnqueueProperties::Operation::none:
    case NEO::EnqueueProperties::Operation::blit:
    default:
        break;
    }

    if (debugManager.flags.ForceGpgpuSubmissionForBcsEnqueue.get() == 1) {
        required = true;
    }

    return required;
}

template <typename Family>
void CommandQueueHw<Family>::setupEvent(EventBuilder &eventBuilder, cl_event *outEvent, uint32_t cmdType) {
    if (outEvent) {
        eventBuilder.create<Event>(this, cmdType, CompletionStamp::notReady, 0);
        auto eventObj = eventBuilder.getEvent();
        *outEvent = eventObj;

        if (eventObj->isProfilingEnabled()) {
            eventObj->setQueueTimeStamp();

            if (isCommandWithoutKernel(cmdType) && !isFlushForProfilingRequired(cmdType)) {
                eventObj->setCPUProfilingPath(true);
            }
        }
        DBG_LOG(EventsDebugEnable, "enqueueHandler commandType", cmdType, "output Event", eventObj);
    }
}

template <typename Family>
CommandQueueHw<Family>::~CommandQueueHw() {
    unregisterGpgpuAndBcsCsrClients();
}
} // namespace NEO
