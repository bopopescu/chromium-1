// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/fake_layer_tree_frame_sink_client.h"

#include "cc/trees/layer_tree_frame_sink.h"
#include "components/viz/common/hit_test/hit_test_region_list.h"

namespace cc {

void FakeLayerTreeFrameSinkClient::SetBeginFrameSource(
    viz::BeginFrameSource* source) {
  begin_frame_source_ = source;
}

base::Optional<viz::HitTestRegionList>
FakeLayerTreeFrameSinkClient::BuildHitTestData() {
  return {};
}

void FakeLayerTreeFrameSinkClient::DidReceiveCompositorFrameAck() {
  ack_count_++;
}

void FakeLayerTreeFrameSinkClient::DidLoseLayerTreeFrameSink() {
  did_lose_layer_tree_frame_sink_called_ = true;
}

void FakeLayerTreeFrameSinkClient::SetMemoryPolicy(
    const ManagedMemoryPolicy& policy) {
  memory_policy_ = policy;
}

}  // namespace cc
