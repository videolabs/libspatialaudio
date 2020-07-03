/*
Copyright 2019 The libear Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

https://github.com/ebu/libear/

*/

/*
    Modified to work with the ADM renderer in libspatial audio
*/

#pragma once

#include <vector>

#include "AdmUtils.h"
#include "AdmLayouts.h"

namespace ear {
  /** @brief Design one filter for each channel in layout.
   *
   * @param layout Layout to design for; channel names are used to allocate
   * filters to channels.
   *
   * @return Decorrelation filters.
   */
  template <typename T = float>
  std::vector<std::vector<T>> designDecorrelators(admrender::Layout layout);

  /** @brief Get the delay length needed to compensate for decorrelators
   *
   * @return Delay length in samples.
   */
  int decorrelatorCompensationDelay();
}  // namespace ear
