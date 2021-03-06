// Copyright 2019 Zhushi Tech, Inc.
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

#undef NDEBUG
#include <cassert>
#include "impl/filter.hpp"

int main()
{
  {
    // 10--1-->10--2-->10--3-->10--4-->(-1)--1-->10--1-->10--2-->10--3-->10--4-->10
    // none.
    float p[] = {10, 10, 10, 10, -1, 10, 10, 10, 10, 10};

    filter_length(p, 10, 1, 1, 2);  // length = 2

    // p[]: 10 10 10 10 -1 10 10 10 10 10
    for (auto i = 0; i < 10; ++i) {
      if (i == 4) {
        assert(p[i] == -1);
      } else {
        assert(p[i] != -1);
      }
    }
  }

  {
    // 10--1-->10--2-->10--3-->10--4-->(-1)--1-->10--1-->10--2-->10--3-->10--4-->10
    // first segment.
    float p[] = {10, 10, 10, 10, -1, 10, 10, 10, 10, 10};

    filter_length(p, 10, 1, 1, 3);  // length = 3

    // p[]: -1 -1 -1 -1 -1 10 10 10 10 10
    for (auto i = 0; i < 10; ++i) {
      if (i <= 4) {
        assert(p[i] == -1);
      } else {
        assert(p[i] != -1);
      }
    }
  }

  {
    // 10--1-->10--2-->10--3-->10--4-->(-1)--1-->10--1-->10--2-->10--3-->10--4-->10
    // all segments.
    float p[] = {10, 10, 10, 10, -1, 10, 10, 10, 10, 10};

    filter_length(p, 10, 1, 1, 4);  // length = 4

    // p[]: -1 -1 -1 -1 -1 -1 -1 -1 -1 -1
    for (auto i = 0; i < 10; ++i) {
      assert(p[i] == -1);
    }
  }

  return 0;
}
