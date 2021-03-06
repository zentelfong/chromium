// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYNC_ENGINE_RESOLVE_CONFLICTS_COMMAND_H_
#define SYNC_ENGINE_RESOLVE_CONFLICTS_COMMAND_H_
#pragma once

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "sync/engine/model_changing_syncer_command.h"

namespace browser_sync {

class ResolveConflictsCommand : public ModelChangingSyncerCommand {
 public:
  ResolveConflictsCommand();
  virtual ~ResolveConflictsCommand();

 protected:
  // ModelChangingSyncerCommand implementation.
  virtual std::set<ModelSafeGroup> GetGroupsToChange(
      const sessions::SyncSession& session) const OVERRIDE;
  virtual SyncerError ModelChangingExecuteImpl(
      sessions::SyncSession* session) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(ResolveConflictsCommand);
};

}  // namespace browser_sync

#endif  // SYNC_ENGINE_RESOLVE_CONFLICTS_COMMAND_H_
