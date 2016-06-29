// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <vector>
#include <cassert>
#include <malloc.h>

#include <os>
#include <net/buffer_store.hpp>

namespace net {

  BufferStore::BufferStore(size_t num, size_t bufsize) :
    poolsize_      {num * bufsize},
    bufsize_       {bufsize},
    device_offset_ {device_offset},
    pool_          {static_cast<buffer_t>(memalign(PAGE_SIZE, num * bufsize))}
{
  assert(pool_);

  debug ("<BufferStore> Creating buffer store of %i * %i bytes.\n",
         num, bufsize);

  for (buffer_t b = pool_; b < pool_ + (num * bufsize); b += bufsize)
    available_buffers_.push_back(b);

  debug ("<BufferStore> I now have %i free buffers in range %p -> %p.\n",
         available_buffers_.size(), pool_, pool_ + (bufcount_ * bufsize_));
}

  BufferStore::~BufferStore() {
    free(pool_);
  }

  BufferStore::buffer_t BufferStore::get_buffer() {
    if (available_buffers_.empty())
      panic("<BufferStore> Storage pool full! Don't know how to increase pool size yet.\n");

    auto buf = available_buffers_.front();
    available_buffers_.pop_front();

    debug2("<BufferStore> Provisioned a buffer. %i buffers remaining.\n",
           available_buffers_.size());

    return buf;
  }

  void BufferStore::release_buffer(buffer_t b) {
    debug2("<BufferStore> Trying to release %i sized buffer @%p.\n", bufsize, b);
    // Make sure the buffer comes from here. Otherwise, ignore it.
    if (address_is_from_pool(b)
        and address_is_bufstart(b)
        and bufsize == bufsize_)
      {
        available_buffers_.push_back(b);
        debug("<BufferStore> Releasing %p. %i available buffers.\n", b, available_buffers_.size());
        return;
      }

    debug("<BufferStore> IGNORING buffer @%p. It isn't mine.\n", b);
  }

} //< namespace net
