/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

template <typename GfxFamily>
void StateBaseAddressHelper<GfxFamily>::appendExtraCacheSettings(StateBaseAddressHelperArgs<GfxFamily> &args) {
    auto &productHelper = args.gmmHelper->getRootDeviceEnvironment().template getHelper<ProductHelper>();
    auto cachePolicy = productHelper.getL1CachePolicy(args.isDebuggerActive);
    args.stateBaseAddressCmd->setL1CachePolicyL1CacheControl(static_cast<typename STATE_BASE_ADDRESS::L1_CACHE_POLICY>(cachePolicy));

    if (DebugManager.flags.ForceStatelessL1CachingPolicy.get() != -1 &&
        DebugManager.flags.ForceAllResourcesUncached.get() == false) {
        args.stateBaseAddressCmd->setL1CachePolicyL1CacheControl(static_cast<typename STATE_BASE_ADDRESS::L1_CACHE_POLICY>(DebugManager.flags.ForceStatelessL1CachingPolicy.get()));
    }
}
