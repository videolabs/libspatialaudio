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

#include "decorrelate.hpp"

#include <cmath>
#include <complex>
#include <random>

#include "kissfft/kissfft.hh"


const double PI = M_PI;

namespace ear {

  std::vector<long> genRandMt19937(int seed, int n) {
    std::mt19937 mtRand(seed);
    std::vector<long> ret;
    for (int i = 0; i < n; ++i) {
      ret.push_back(mtRand());
    }
    return ret;
  }

  std::vector<double> genRandFloat(int seed, int n) {
    std::vector<double> ret;
    for (long randomValue : genRandMt19937(seed, n)) {
      ret.push_back(randomValue / static_cast<double>(0x100000000l));
    }
    return ret;
  }

  /** @brief Design an all-pass random-phase FIR filter.
   *
   * @param decorrelator_id Random seed, to obtain different filters.
   * @param size filter length.
   *
   * @return  Filter coefficients.
   */
  std::vector<double> designDecorrelatorBasic(int decorrelatorId, int size) {
    std::vector<double> rand = genRandFloat(decorrelatorId, size / 2 - 1);
    std::vector<std::complex<double>> freqDomainData(size);
    freqDomainData[0] = std::complex<double>(1.0, 0.0);
    for (size_t i = 0; i < rand.size(); ++i) {
      freqDomainData[i + 1] =
          std::exp(std::complex<double>(0.0, 2.0 * PI * rand[i]));
    }
    freqDomainData[size / 2] = std::complex<double>(1.0, 0.0);
    for (size_t i = 0; i < freqDomainData.size() / 2; ++i) {
      freqDomainData[size / 2 + i] = std::conj(freqDomainData[size / 2 - i]);
    }
    kissfft<double> fft(size, true);
    std::vector<std::complex<double>> timeDomainData(size);
    fft.transform(&freqDomainData[0], &timeDomainData[0]);
    std::vector<double> timeDomainDataReal(size);
    for (size_t i = 0; i < timeDomainData.size(); ++i) {
      timeDomainDataReal[i] = timeDomainData[i].real() / size;
    }
    return timeDomainDataReal;
  }

  const int decorrelator_size = 512;

  template <>
  std::vector<std::vector<double>> designDecorrelators<double>(admrender::Layout layout) {
    std::vector<std::string> channelNames = layout.channelNames();
    std::vector<std::string> channelNamesSorted(channelNames);
    std::sort(channelNamesSorted.begin(), channelNamesSorted.end());
    std::vector<std::vector<double>> decorrelators;
    for (auto channelName : channelNames) {
      auto it =
          std::find_if(channelNamesSorted.begin(), channelNamesSorted.end(),
                       [&channelName](const std::string name) -> bool {
                         return channelName == name;
                       });
      int index =
          static_cast<int>(std::distance(channelNamesSorted.begin(), it));
      std::vector<double> coefficients =
          designDecorrelatorBasic(index, decorrelator_size);
      decorrelators.push_back(coefficients);
    }
    return decorrelators;
  }

  template <>
  std::vector<std::vector<float>> designDecorrelators<float>(
      admrender::Layout layout) {
    auto decorrelators = designDecorrelators<double>(layout);
    std::vector<std::vector<float>> decorrelators_float;

    for (auto &decorrelator : decorrelators) {
      std::vector<float> decorrelator_float(decorrelator.size());

      for (size_t i = 0; i < decorrelator.size(); i++)
        decorrelator_float[i] = (float)decorrelator[i];

      decorrelators_float.emplace_back(std::move(decorrelator_float));
    }

    return decorrelators_float;
  }

  int decorrelatorCompensationDelay() { return (decorrelator_size - 1) / 2; }
}  // namespace ear
