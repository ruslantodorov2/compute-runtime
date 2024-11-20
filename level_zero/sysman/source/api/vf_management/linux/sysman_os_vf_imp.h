/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "shared/source/helpers/gfx_core_helper.h"
#include "shared/source/os_interface/linux/drm_neo.h"
#include "shared/source/os_interface/product_helper.h"

#include "level_zero/sysman/source/api/vf_management/sysman_vf_imp.h"
#include "level_zero/sysman/source/shared/linux/zes_os_sysman_imp.h"

#include <string>

namespace L0 {
namespace Sysman {
class SysFsAccessInterface;

class LinuxVfImp : public OsVf, NEO::NonCopyableOrMovableClass {
  public:
    LinuxVfImp() = default;
    LinuxVfImp(OsSysman *pOsSysman, uint32_t vfId);
    ~LinuxVfImp() override = default;

    ze_result_t vfOsGetCapabilities(zes_vf_exp_capabilities_t *pCapability) override;
    ze_result_t vfOsGetMemoryUtilization(uint32_t *pCount, zes_vf_util_mem_exp2_t *pMemUtil) override;
    ze_result_t vfOsGetEngineUtilization(uint32_t *pCount, zes_vf_util_engine_exp2_t *pEngineUtil) override;
    bool vfOsGetLocalMemoryQuota(uint64_t &lMemQuota) override;
    bool vfOsGetLocalMemoryUsed(uint64_t &lMemUsed) override;

  protected:
    ze_result_t getVfBDFAddress(uint32_t vfIdMinusOne, zes_pci_address_t *address);
    LinuxSysmanImp *pLinuxSysmanImp = nullptr;
    SysFsAccessInterface *pSysfsAccess = nullptr;
    uint32_t vfId = 0;
    static const uint32_t maxMemoryTypes = 1; // Since only the Device Memory Utilization is Supported and not for the Host Memory, this value is 1
};

} // namespace Sysman
} // namespace L0