// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// IPC messages for the audio.
// Multiply-included message file, hence no include guard.

#include <string>

#include "base/basictypes.h"
#include "base/shared_memory.h"
#include "base/sync_socket.h"
#include "content/common/content_export.h"
#include "content/common/media/audio_param_traits.h"
#include "content/common/media/audio_stream_state.h"
#include "ipc/ipc_message_macros.h"
#include "media/audio/audio_buffers_state.h"
#include "media/audio/audio_parameters.h"

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT
#define IPC_MESSAGE_START AudioMsgStart

IPC_ENUM_TRAITS(AudioStreamState)

IPC_STRUCT_TRAITS_BEGIN(media::AudioBuffersState)
  IPC_STRUCT_TRAITS_MEMBER(pending_bytes)
  IPC_STRUCT_TRAITS_MEMBER(hardware_delay_bytes)
IPC_STRUCT_TRAITS_END()

// Messages sent from the browser to the renderer.

// Tell the renderer process that an audio stream has been created.
// The renderer process is given a shared memory handle for the audio data
// buffer it shares with the browser process. It is also given a SyncSocket that
// it uses to communicate with the browser process about the state of the
// buffered audio data.
#if defined(OS_WIN)
IPC_MESSAGE_CONTROL4(AudioMsg_NotifyStreamCreated,
                     int /* stream id */,
                     base::SharedMemoryHandle /* handle */,
                     base::SyncSocket::Handle /* socket handle */,
                     uint32 /* length */)
#else
IPC_MESSAGE_CONTROL4(AudioMsg_NotifyStreamCreated,
                     int /* stream id */,
                     base::SharedMemoryHandle /* handle */,
                     base::FileDescriptor /* socket handle */,
                     uint32 /* length */)
#endif

// Tell the renderer process that an audio input stream has been created.
// The renderer process would be given a SyncSocket that it should read
// from from then on.
#if defined(OS_WIN)
IPC_MESSAGE_CONTROL4(AudioInputMsg_NotifyStreamCreated,
                     int /* stream id */,
                     base::SharedMemoryHandle /* handle */,
                     base::SyncSocket::Handle /* socket handle */,
                     uint32 /* length */)
#else
IPC_MESSAGE_CONTROL4(AudioInputMsg_NotifyStreamCreated,
                     int /* stream id */,
                     base::SharedMemoryHandle /* handle */,
                     base::FileDescriptor /* socket handle */,
                     uint32 /* length */)
#endif

// Notification message sent from AudioRendererHost to renderer for state
// update after the renderer has requested a Create/Start/Close.
IPC_MESSAGE_CONTROL2(AudioMsg_NotifyStreamStateChanged,
                     int /* stream id */,
                     AudioStreamState /* new state */)

// Notification message sent from browser to renderer for state update.
IPC_MESSAGE_CONTROL2(AudioInputMsg_NotifyStreamStateChanged,
                     int /* stream id */,
                     AudioStreamState /* new state */)

IPC_MESSAGE_CONTROL2(AudioInputMsg_NotifyStreamVolume,
                     int /* stream id */,
                     double /* volume */)

IPC_MESSAGE_CONTROL2(AudioInputMsg_NotifyDeviceStarted,
                     int /* stream id */,
                     std::string /* device_id */)

// Messages sent from the renderer to the browser.

// Request that got sent to browser for creating an audio output stream
IPC_MESSAGE_CONTROL2(AudioHostMsg_CreateStream,
                     int /* stream_id */,
                     media::AudioParameters /* params */)

// Request that got sent to browser for creating an audio input stream
IPC_MESSAGE_CONTROL4(AudioInputHostMsg_CreateStream,
                     int /* stream_id */,
                     media::AudioParameters /* params */,
                     std::string /* device_id */,
                     bool /* automatic_gain_control */)

// Start buffering and play the audio stream specified by stream_id.
IPC_MESSAGE_CONTROL1(AudioHostMsg_PlayStream,
                     int /* stream_id */)

// Start recording the audio input stream specified by stream_id.
IPC_MESSAGE_CONTROL1(AudioInputHostMsg_RecordStream,
                     int /* stream_id */)

// Pause the audio stream specified by stream_id.
IPC_MESSAGE_CONTROL1(AudioHostMsg_PauseStream,
                     int /* stream_id */)

// Discard all buffered audio data for the specified audio stream.
IPC_MESSAGE_CONTROL1(AudioHostMsg_FlushStream,
                     int /* stream_id */)

// Close an audio stream specified by stream_id.
IPC_MESSAGE_CONTROL1(AudioHostMsg_CloseStream,
                     int /* stream_id */)

// Close an audio input stream specified by stream_id.
IPC_MESSAGE_CONTROL1(AudioInputHostMsg_CloseStream,
                     int /* stream_id */)

// Set audio volume of the stream specified by stream_id.
// TODO(hclam): change this to vector if we have channel numbers other than 2.
IPC_MESSAGE_CONTROL2(AudioHostMsg_SetVolume,
                     int /* stream_id */,
                     double /* volume */)

// Set audio volume of the input stream specified by stream_id.
IPC_MESSAGE_CONTROL2(AudioInputHostMsg_SetVolume,
                     int /* stream_id */,
                     double /* volume */)

// Start the device referenced by the session_id for the input stream specified
// by stream_id.
IPC_MESSAGE_CONTROL2(AudioInputHostMsg_StartDevice,
                     int /* stream_id */,
                     int /* session_id */)
