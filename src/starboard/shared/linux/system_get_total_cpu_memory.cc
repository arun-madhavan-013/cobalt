// Copyright 2016 Google Inc. All Rights Reserved.
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

#include "starboard/system.h"

#include <unistd.h>

#include "starboard/log.h"

int64_t SbSystemGetTotalCPUMemory() {
  long pages = sysconf(_SC_PHYS_PAGES);     // NOLINT[runtime/int]
  long page_size = sysconf(_SC_PAGE_SIZE);  // NOLINT[runtime/int]
  if (pages == -1 || page_size == -1) {
    SB_NOTREACHED();
    return 0;
  }

  return static_cast<int64_t>(pages) * page_size;
}
