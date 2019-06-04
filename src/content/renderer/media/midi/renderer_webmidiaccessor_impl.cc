// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/midi/renderer_webmidiaccessor_impl.h"

#include "base/logging.h"
#include "content/renderer/media/midi/midi_session_client_impl.h"
#include "content/renderer/render_thread_impl.h"

namespace content {

RendererWebMIDIAccessorImpl::RendererWebMIDIAccessorImpl(
    blink::WebMIDIAccessorClient* client)
    : client_(client), is_client_added_(false) {
  DCHECK(client_);
}

RendererWebMIDIAccessorImpl::~RendererWebMIDIAccessorImpl() {
  if (is_client_added_)
    midi_session_client_impl()->RemoveClient(client_);
}

void RendererWebMIDIAccessorImpl::StartSession() {
  midi_session_client_impl()->AddClient(client_);
  is_client_added_ = true;
}

void RendererWebMIDIAccessorImpl::SendMIDIData(unsigned port_index,
                                               const unsigned char* data,
                                               size_t length,
                                               base::TimeTicks timestamp) {
  midi_session_client_impl()->SendMidiData(port_index, data, length, timestamp);
}

MidiSessionClientImpl* RendererWebMIDIAccessorImpl::midi_session_client_impl() {
  return RenderThreadImpl::current()->midi_session_client_impl();
}

}  // namespace content
