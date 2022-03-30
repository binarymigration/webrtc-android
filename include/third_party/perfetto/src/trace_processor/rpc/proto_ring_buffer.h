/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SRC_TRACE_PROCESSOR_RPC_PROTO_RING_BUFFER_H_
#define SRC_TRACE_PROCESSOR_RPC_PROTO_RING_BUFFER_H_

#include <stdint.h>

#include "perfetto/ext/base/paged_memory.h"

namespace perfetto {
namespace trace_processor {

// This class buffers and tokenizes proto messages used for the TraceProcessor
// RPC interface (See comments in trace_processor.proto).
// From a logical level, the RPC is a sequence of protos like this.
// [ header 1 ] [ payload 1   ]
// [ header 2 ] [ payload 2  ]
// [ header 3 ] [ payload 3     ]
// Where [ header ] is a variable-length sequence of:
// [ Field ID = 1, type = length-delimited] [ length (varint) ].
// The RPC pipe is byte-oriented, not message-oriented (like a TCP stream).
// The pipe is not required to respect the boundaries of each message, it only
// guarantees that data is not lost or duplicated. The following sequence of
// inbound events is possible:
// 1. [ hdr 1 (incomplete) ... ]
// 2. [ ... hdr 1 ] [ payload 1 ] [ hdr 2 ] [ payoad 2 ] [ hdr 3 ] [ pay... ]
// 3. [ ...load 3 ]
//
// This class maintains inbound requests in a ring buffer.
// The expected usage is:
// ring_buf.Append(data, len);
// for (;;) {
//   auto msg = ring_buf.ReadMessage();
//   if (!msg.valid())
//     break;
//   Decode(msg);
// }
//
// After each call to Append, the caller is expected to call ReadMessage() until
// it returns an invalid message (signalling no more messages could be decoded).
// Note that a single Append can "unblock" > 1 messages, which is why the caller
// needs to keep calling ReadMessage in a loop.
//
// Internal architecture
// ---------------------
// Internally this is similar to a ring-buffer, with the caveat that it never
// wraps, it only expands. Expansions are rare. The deal is that in most cases
// the read cursor follows very closely the write cursor. For instance, if the
// uderlying behaves as a dgram socket, after each Append, the read cursor will
// chase completely the write cursor. Even if the underyling stream is not
// always atomic, the expectation is that the read cursor will eventually reach
// the write one within few messages.
// A visual example, imagine we have four messages: 2it 4will 2be 4fine
// Visually:
//
// Append("2it4wi"): A message and a bit:
// [ 2it 4wi                     ]
// ^R       ^W
//
// After the ReadMessage(), the 1st message will be read, but not the 2nd.
// [ 2it 4wi                     ]
//      ^R ^W
//
// Append("ll2be4f")
// [ 2it 4will 2be 4f            ]
//      ^R           ^W
//
// After the ReadMessage() loop:
// [ 2it 4will 2be 4f            ]
//                ^R ^W
// Append("ine")
// [ 2it 4will 2be 4fine         ]
//                ^R    ^W
//
// In the next ReadMessage() the R cursor will chase the W cursor. When this
// happens (very frequent) we can just reset both cursors to 0 and restart.
// If we are unlucky and get to the end of the buffer, two things happen:
// 1. We try first to recompact the buffer, moving everything left by R.
// 2. If still there isn't enough space, we expand the buffer.
// Given that each message is expected to be at most kMaxMsgSize (64 MB), the
// expansion is bound at 2 * kMaxMsgSize.
class ProtoRingBuffer {
 public:
  static constexpr size_t kMaxMsgSize = 64 * 1024 * 1024;
  struct Message {
    const uint8_t* start = nullptr;
    uint32_t len = 0;
    uint32_t field_id = 0;
    bool fatal_framing_error = false;
    const uint8_t* end() const { return start + len; }
    inline bool valid() const { return !!start; }
  };

  ProtoRingBuffer();
  ~ProtoRingBuffer();
  ProtoRingBuffer(const ProtoRingBuffer&) = delete;
  ProtoRingBuffer& operator=(const ProtoRingBuffer&) = delete;

  // Appends data into the ring buffer, recompacting or resizing it if needed.
  // Will invaildate the pointers previously handed out.
  void Append(const void* data, size_t len);

  // If a protobuf message can be read, it returns the boundaries of the message
  // (without including the preamble) and advances the read cursor.
  // If no message is avaiable, returns a null range.
  // The returned pointer is only valid until the next call to Append(), as
  // that can recompact or resize the underlying buffer.
  Message ReadMessage();

  // Exposed for testing.
  size_t capacity() const { return buf_.size(); }
  size_t avail() const { return buf_.size() - (wr_ - rd_); }

 private:
  base::PagedMemory buf_;
  Message fastpath_{};
  bool failed_ = false;  // Set in case of an unrecoverable framing faiulre.
  size_t rd_ = 0;        // Offset of the read cursor in |buf_|.
  size_t wr_ = 0;        // Offset of the write cursor in |buf_|.
};

}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_RPC_PROTO_RING_BUFFER_H_
