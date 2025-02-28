// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BackgroundSyncProvider_h
#define BackgroundSyncProvider_h

#include "modules/background_sync/SyncCallbacks.h"
#include "public/platform/modules/background_sync/background_sync.mojom-blink.h"
#include "wtf/Noncopyable.h"
#include <memory>

namespace blink {

class WebServiceWorkerRegistration;

// The BackgroundSyncProvider is called by the SyncManager and SyncRegistration
// objects and communicates with the BackgroundSyncManager object in the browser
// process. Each thread will have its own instance (e.g. main thread, worker
// threads), instantiated as needed by SyncManager.  Each instance of the
// provider creates a new mojo connection to a new BackgroundSyncManagerImpl,
// which then talks to the BackgroundSyncManager object.
class BackgroundSyncProvider {
  WTF_MAKE_NONCOPYABLE(BackgroundSyncProvider);

 public:
  BackgroundSyncProvider() = default;

  void registerBackgroundSync(mojom::blink::SyncRegistrationPtr options,
                              WebServiceWorkerRegistration*,
                              std::unique_ptr<SyncRegistrationCallbacks>);
  void getRegistrations(WebServiceWorkerRegistration*,
                        std::unique_ptr<SyncGetRegistrationsCallbacks>);

 private:
  // Callback handlers
  static void registerCallback(
      std::unique_ptr<blink::SyncRegistrationCallbacks>,
      mojom::blink::BackgroundSyncError,
      mojom::blink::SyncRegistrationPtr options);
  static void getRegistrationsCallback(
      std::unique_ptr<SyncGetRegistrationsCallbacks>,
      mojom::blink::BackgroundSyncError,
      mojo::WTFArray<mojom::blink::SyncRegistrationPtr> registrations);

  // Returns an initialized BackgroundSyncServicePtr. A connection with the
  // the browser's BackgroundSyncService is created the first time this method
  // is called.
  mojom::blink::BackgroundSyncServicePtr& GetBackgroundSyncServicePtr();

  mojom::blink::BackgroundSyncServicePtr m_backgroundSyncService;
};

}  // namespace blink

#endif  // BackgroundSyncProvider_h
