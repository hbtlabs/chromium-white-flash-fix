// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sessions/tab_loader.h"

#include "base/memory/memory_coordinator_proxy.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/memory_coordinator_delegate.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "content/public/test/memory_coordinator_test_utils.h"
#include "content/public/test/test_browser_thread.h"
#include "content/public/test/test_web_contents_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

class TabLoaderTest : public testing::Test {
 public:
  using RestoredTab = SessionRestoreDelegate::RestoredTab;

  TabLoaderTest()
      : ui_thread_(content::BrowserThread::UI, &message_loop_) {}

  // testing::Test:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeature(features::kMemoryCoordinator);
    content::SetUpMemoryCoordinatorProxyForTesting();

    test_web_contents_factory_.reset(new content::TestWebContentsFactory);
    content::WebContents* contents =
        test_web_contents_factory_->CreateWebContents(&testing_profile_);
    restored_tabs_.push_back(RestoredTab(contents, false, false, false));
  }

  void TearDown() override {
    restored_tabs_.clear();
    test_web_contents_factory_.reset();
  }

 protected:
  std::unique_ptr<content::TestWebContentsFactory> test_web_contents_factory_;
  std::vector<RestoredTab> restored_tabs_;

  base::MessageLoop message_loop_;
  TestingProfile testing_profile_;
  content::TestBrowserThread ui_thread_;
  base::test::ScopedFeatureList scoped_feature_list_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabLoaderTest);
};

// TODO(hajimehoshi): Enable this test on macos when MemoryMonitorMac is
// implemented.
#if defined(OS_MACOSX)
#define MAYBE_OnMemoryStateChange DISABLED_OnMemoryStateChange
#else
#define MAYBE_OnMemoryStateChange OnMemoryStateChange
#endif
TEST_F(TabLoaderTest, MAYBE_OnMemoryStateChange) {
  TabLoader::RestoreTabs(restored_tabs_, base::TimeTicks());
  EXPECT_TRUE(TabLoader::shared_tab_loader_->loading_enabled_);
  base::MemoryCoordinatorProxy::GetInstance()->SetCurrentMemoryStateForTesting(
      base::MemoryState::THROTTLED);
  // ObserverListThreadsafe is used to notify the state to clients, so running
  // the loop is necessary here.
  base::RunLoop loop;
  loop.RunUntilIdle();
  EXPECT_FALSE(TabLoader::shared_tab_loader_->loading_enabled_);
}
