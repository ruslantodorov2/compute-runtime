/*
 * Copyright (C) 2021-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/debug_settings/debug_settings_manager.h"
#include "shared/source/helpers/common_types.h"
#include "shared/source/helpers/constants.h"
#include "shared/source/os_interface/linux/cache_info.h"
#include "shared/source/os_interface/linux/drm_wrappers.h"
#include "shared/source/os_interface/linux/i915_upstream.h"
#include "shared/source/os_interface/linux/ioctl_helper.h"

namespace NEO {

bool IoctlHelperUpstream::initialize() {
    detectExtSetPatSupport();
    initializeGetGpuTimeFunction();
    return true;
}

bool IoctlHelperUpstream::isSetPairAvailable() {
    return false;
}

bool IoctlHelperUpstream::isChunkingAvailable() {
    return false;
}

bool IoctlHelperUpstream::isVmBindAvailable() {
    return false;
}

int IoctlHelperUpstream::createGemExt(const MemRegionsVec &memClassInstances, size_t allocSize, uint32_t &handle, uint64_t patIndex, std::optional<uint32_t> vmId, int32_t pairHandle, bool isChunked, uint32_t numOfChunks, std::optional<uint32_t> memPolicyMode, std::optional<std::vector<unsigned long>> memPolicyNodemask) {
    bool isPatIndexValid = (patIndex != CommonConstants::unsupportedPatIndex) && (patIndex <= std::numeric_limits<uint32_t>::max());
    bool useSetPat = this->isSetPatSupported && isPatIndexValid;

    uint32_t regionsSize = static_cast<uint32_t>(memClassInstances.size());
    std::vector<drm_i915_gem_memory_class_instance> regions(regionsSize);
    for (uint32_t i = 0; i < regionsSize; i++) {
        regions[i].memory_class = memClassInstances[i].memoryClass;
        regions[i].memory_instance = memClassInstances[i].memoryInstance;
    }

    drm_i915_gem_create_ext_memory_regions memRegions{};
    memRegions.num_regions = regionsSize;
    memRegions.regions = reinterpret_cast<uintptr_t>(regions.data());
    memRegions.base.name = I915_GEM_CREATE_EXT_MEMORY_REGIONS;

    drm_i915_gem_create_ext_set_pat setPat{};
    setPat.pat_index = static_cast<uint32_t>(patIndex);
    setPat.base.name = I915_GEM_CREATE_EXT_SET_PAT;

    drm_i915_gem_create_ext createExt{};
    createExt.size = allocSize;
    createExt.extensions = reinterpret_cast<uintptr_t>(&memRegions);

    if (useSetPat) {
        memRegions.base.next_extension = reinterpret_cast<uintptr_t>(&setPat);
    }

    if (DebugManager.flags.PrintBOCreateDestroyResult.get()) {
        printDebugString(DebugManager.flags.PrintBOCreateDestroyResult.get(), stdout, "Performing GEM_CREATE_EXT with { size: %lu",
                         allocSize);

        for (uint32_t i = 0; i < regionsSize; i++) {
            auto region = regions[i];
            printDebugString(DebugManager.flags.PrintBOCreateDestroyResult.get(), stdout, ", memory class: %d, memory instance: %d",
                             region.memory_class, region.memory_instance);
        }

        if (useSetPat) {
            printDebugString(DebugManager.flags.PrintBOCreateDestroyResult.get(), stdout, ", pat index: %lu", patIndex);
        }

        printDebugString(DebugManager.flags.PrintBOCreateDestroyResult.get(), stdout, "%s", " }\n");
    }

    auto ret = ioctl(DrmIoctl::GemCreateExt, &createExt);
    handle = createExt.handle;

    printDebugString(DebugManager.flags.PrintBOCreateDestroyResult.get(), stdout, "GEM_CREATE_EXT with EXT_MEMORY_REGIONS%s has returned: %d BO-%u with size: %lu\n",
                     (useSetPat) ? " with EXT_SET_PAT" : "",
                     ret, createExt.handle, createExt.size);

    return ret;
}

void IoctlHelperUpstream::detectExtSetPatSupport() {
    drm_i915_gem_create_ext_set_pat setPat{};
    setPat.pat_index = 0;
    setPat.base.name = I915_GEM_CREATE_EXT_SET_PAT;

    drm_i915_gem_create_ext createExt{};
    createExt.size = 1;
    createExt.extensions = reinterpret_cast<uintptr_t>(&setPat);

    auto isSetPatDisabled = DebugManager.flags.DisableGemCreateExtSetPat.get();

    if (!isSetPatDisabled) {
        int returnValue = ioctl(DrmIoctl::GemCreateExt, &createExt);
        this->isSetPatSupported = (returnValue == 0);

        if (returnValue == 0) {
            GemClose close{};
            close.handle = createExt.handle;
            returnValue = ioctl(DrmIoctl::GemClose, &close);
            UNRECOVERABLE_IF(returnValue);
        }
    }

    printDebugString(DebugManager.flags.PrintBOCreateDestroyResult.get(), stdout, "EXT_SET_PAT support is: %s\n",
                     this->isSetPatSupported ? "enabled" : "disabled");
}

CacheRegion IoctlHelperUpstream::closAlloc() {
    return CacheRegion::None;
}

uint16_t IoctlHelperUpstream::closAllocWays(CacheRegion closIndex, uint16_t cacheLevel, uint16_t numWays) {
    return 0;
}

CacheRegion IoctlHelperUpstream::closFree(CacheRegion closIndex) {
    return CacheRegion::None;
}

int IoctlHelperUpstream::waitUserFence(uint32_t ctxId, uint64_t address,
                                       uint64_t value, uint32_t dataWidth, int64_t timeout, uint16_t flags) {
    return 0;
}

uint32_t IoctlHelperUpstream::getAtomicAdvise(bool isNonAtomic) {
    return 0;
}

uint32_t IoctlHelperUpstream::getAtomicAccess(AtomicAccessMode mode) {
    return 0;
}

uint32_t IoctlHelperUpstream::getPreferredLocationAdvise() {
    return 0;
}

std::optional<MemoryClassInstance> IoctlHelperUpstream::getPreferredLocationRegion(PreferredLocation memoryLocation, uint32_t memoryInstance) {
    return std::nullopt;
}

bool IoctlHelperUpstream::setVmBoAdvise(int32_t handle, uint32_t attribute, void *region) {
    return true;
}

bool IoctlHelperUpstream::setVmBoAdviseForChunking(int32_t handle, uint64_t start, uint64_t length, uint32_t attribute, void *region) {
    return true;
}

bool IoctlHelperUpstream::setVmPrefetch(uint64_t start, uint64_t length, uint32_t region, uint32_t vmId) {
    return true;
}

uint32_t IoctlHelperUpstream::getDirectSubmissionFlag() {
    return 0u;
}

std::unique_ptr<uint8_t[]> IoctlHelperUpstream::prepareVmBindExt(const StackVec<uint32_t, 2> &bindExtHandles) {
    return {};
}

uint64_t IoctlHelperUpstream::getFlagsForVmBind(bool bindCapture, bool bindImmediate, bool bindMakeResident) {
    return 0u;
}

int IoctlHelperUpstream::queryDistances(std::vector<QueryItem> &queryItems, std::vector<DistanceInfo> &distanceInfos) {
    for (auto &query : queryItems) {
        query.length = -EINVAL;
    }
    return 0;
}

uint16_t IoctlHelperUpstream::getWaitUserFenceSoftFlag() {
    return 0;
}

int IoctlHelperUpstream::execBuffer(ExecBuffer *execBuffer, uint64_t completionGpuAddress, TaskCountType counterValue) {
    return ioctl(DrmIoctl::GemExecbuffer2, execBuffer);
}

bool IoctlHelperUpstream::completionFenceExtensionSupported(const bool isVmBindAvailable) {
    return false;
}

std::optional<DrmParam> IoctlHelperUpstream::getHasPageFaultParamId() {
    return std::nullopt;
};

bool IoctlHelperUpstream::getEuStallProperties(std::array<uint64_t, 12u> &properties, uint64_t dssBufferSize,
                                               uint64_t samplingRate, uint64_t pollPeriod, uint64_t engineInstance,
                                               uint64_t notifyNReports) {
    return false;
}

uint32_t IoctlHelperUpstream::getEuStallFdParameter() {
    return 0u;
}

std::unique_ptr<uint8_t[]> IoctlHelperUpstream::createVmControlExtRegion(const std::optional<MemoryClassInstance> &regionInstanceClass) {
    return {};
}

uint32_t IoctlHelperUpstream::getFlagsForVmCreate(bool disableScratch, bool enablePageFault, bool useVmBind) {
    return 0u;
}

uint32_t IoctlHelperUpstream::createContextWithAccessCounters(GemContextCreateExt &gcc) {
    return EINVAL;
}

uint32_t IoctlHelperUpstream::createCooperativeContext(GemContextCreateExt &gcc) {
    return EINVAL;
}

void IoctlHelperUpstream::fillVmBindExtSetPat(VmBindExtSetPatT &vmBindExtSetPat, uint64_t patIndex, uint64_t nextExtension) {}

void IoctlHelperUpstream::fillVmBindExtUserFence(VmBindExtUserFenceT &vmBindExtUserFence, uint64_t fenceAddress, uint64_t fenceValue, uint64_t nextExtension) {}

std::optional<uint64_t> IoctlHelperUpstream::getCopyClassSaturatePCIECapability() {
    return std::nullopt;
}

std::optional<uint64_t> IoctlHelperUpstream::getCopyClassSaturateLinkCapability() {
    return std::nullopt;
}

uint32_t IoctlHelperUpstream::getVmAdviseAtomicAttribute() {
    return 0;
}

int IoctlHelperUpstream::vmBind(const VmBindParams &vmBindParams) {
    return 0;
}

int IoctlHelperUpstream::vmUnbind(const VmBindParams &vmBindParams) {
    return 0;
}

UuidRegisterResult IoctlHelperUpstream::registerUuid(const std::string &uuid, uint32_t uuidClass, uint64_t ptr, uint64_t size) {
    return {0, 0};
}

UuidRegisterResult IoctlHelperUpstream::registerStringClassUuid(const std::string &uuid, uint64_t ptr, uint64_t size) {
    return {0, 0};
}

int IoctlHelperUpstream::unregisterUuid(uint32_t handle) {
    return 0;
}

bool IoctlHelperUpstream::isContextDebugSupported() {
    return false;
}

int IoctlHelperUpstream::setContextDebugFlag(uint32_t drmContextId) {
    return 0;
}

bool IoctlHelperUpstream::isDebugAttachAvailable() {
    return false;
}

unsigned int IoctlHelperUpstream::getIoctlRequestValue(DrmIoctl ioctlRequest) const {
    switch (ioctlRequest) {
    case DrmIoctl::GemCreateExt:
        return DRM_IOCTL_I915_GEM_CREATE_EXT;
    default:
        return getIoctlRequestValueBase(ioctlRequest);
    }
}

int IoctlHelperUpstream::getDrmParamValue(DrmParam drmParam) const {
    switch (drmParam) {
    case DrmParam::EngineClassCompute:
        return 4;
    case DrmParam::QueryHwconfigTable:
        return DRM_I915_QUERY_HWCONFIG_BLOB;
    case DrmParam::QueryComputeSlices:
        return 0;
    default:
        return getDrmParamValueBase(drmParam);
    }
}
std::string IoctlHelperUpstream::getDrmParamString(DrmParam param) const {
    return getDrmParamStringBase(param);
}
std::string IoctlHelperUpstream::getIoctlString(DrmIoctl ioctlRequest) const {
    switch (ioctlRequest) {
    case DrmIoctl::GemCreateExt:
        return "DRM_IOCTL_I915_GEM_CREATE_EXT";
    default:
        return getIoctlStringBase(ioctlRequest);
    }
}

bool IoctlHelperUpstream::getFabricLatency(uint32_t fabricId, uint32_t &latency, uint32_t &bandwidth) {
    return false;
}

bool IoctlHelperUpstream::isWaitBeforeBindRequired(bool bind) const {
    return false;
}
} // namespace NEO
