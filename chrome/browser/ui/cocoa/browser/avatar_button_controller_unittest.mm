// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/browser/avatar_button_controller.h"

#include "base/memory/scoped_nsobject.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/cocoa_profile_test.h"
#include "chrome/browser/ui/cocoa/run_loop_testing.h"

class AvatarButtonControllerTest : public CocoaProfileTest {
 public:
  virtual void SetUp() {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());
    browser()->InitBrowserWindow();

    controller_.reset(
        [[AvatarButtonController alloc] initWithBrowser:browser()]);
    [[controller_ view] setHidden:YES];
  }

  virtual void TearDown() {
    browser()->window()->Close();
    CocoaProfileTest::TearDown();
  }

  NSButton* button() { return [controller_ buttonView]; }

 private:
  scoped_nsobject<AvatarButtonController> controller_;
};

TEST_F(AvatarButtonControllerTest, AddRemoveProfiles) {
  EXPECT_TRUE([button() isHidden]);

  testing_profile_manager()->CreateTestingProfile("one");

  EXPECT_FALSE([button() isHidden]);

  testing_profile_manager()->CreateTestingProfile("two");
  EXPECT_FALSE([button() isHidden]);

  testing_profile_manager()->DeleteTestingProfile("one");
  EXPECT_FALSE([button() isHidden]);

  testing_profile_manager()->DeleteTestingProfile("two");
  EXPECT_TRUE([button() isHidden]);
}
