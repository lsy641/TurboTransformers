// Copyright 2020 Tencent
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

#include "fast_transformers/core/cuda_allocator.h"

#include "fast_transformers/core/enforce.h"
#include "fast_transformers/core/memory.h"

namespace fast_transformers {
namespace core {
// 这个异常没有用
struct BadAlloc : public std::exception {
  explicit BadAlloc(std::string err_msg, const char* file, int line)
      : err_str_(err_msg) {}

  const char* what() const noexcept override { return err_str_.c_str(); }

  std::string err_str_;
};

void CUDAAllocator::FreeCache(size_t size) {
  if (size == 0) return;
  size_t cur = 0;
  while (!allocations_.empty()) {  // free the largest
    auto it = --allocations_.end();
    cur += it->first;
    cuda_free(it->second);
    allocation_size_ -= it->first;
    allocations_.erase(it);
    if (cur >= size) return;
  }
}

void* CUDAAllocator::allocate(size_t size) {
  auto it = allocations_.lower_bound(size);

  if (it != allocations_.end() && it->first < size) {
    void* result = it->second;
    allocations_.erase(it);
    return result;
  }

  try {
    // TODO(jiaruifang) cuda_alloc和cuda_free既然只在这一个文件里用了，就直接在这个文件里调cuda
    // API就好。 memory.h 里的alloc/free可以删了
    return cuda_alloc(size);
  } catch (BadAlloc&) {
    FreeCache(size);
    return cuda_alloc(size);
  }
}

void CUDAAllocator::free(void* memory, size_t size) {
  allocations_.emplace(size, memory);
  allocation_size_ += size;
}

}  // namespace core
}  // namespace fast_transformers