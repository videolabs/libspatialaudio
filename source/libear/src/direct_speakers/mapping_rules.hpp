#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include "AdmLayouts.h"
// This file contains code take from libear (Apache 2.0 license) and adapted to work with libspatialaudio's ADM renderer

namespace ear {

  // Remap a particular channel if all output loudspeakers in gains exist and
  // the input layout is as given.
  struct MappingRule {
    // Label of speaker to match.
    std::string speakerLabel;
    // Gains to match and apply.
    std::vector<std::pair<std::string, double>> gains;
    // Optional ITU names of input layouts to match against. If this isn't given
    // then the rule applies for any input layout.
    std::vector<std::string> input_layouts;
    // Optional ITU names of output layouts to match against. If this isn't
    // given then the rule applies for any output layout.
    std::vector<std::string> output_layouts;
  };

  // mapping rules, after processing with _add_symmetric_rules
  extern const std::vector<MappingRule> rules;

  // mapping from common definitions audioPackFormatID to ITU pack
  extern const std::map<std::string, std::string> itu_packs;

  // This namesoace is originally from libear src/direct_speakers/gain_calculator_direct_speakers.cpp
  namespace {

      // does a sequence contain a value?
      template <typename Seq, typename Value>
      bool contains(const Seq& seq, const Value& value) {
          return std::find(seq.begin(), seq.end(), value) != seq.end();
      }

      // does a rule apply to a given input layout (itu name), canonical
      // speakerLabel and output layout?
      bool rule_applies(const MappingRule& rule, const std::string& input_layout,
          const std::string& speakerLabel,
          admrender::Layout& output_layout) {
          if (rule.input_layouts.size() &&
              !contains(rule.input_layouts, input_layout))
              return false;

          if (rule.output_layouts.size() &&
              !contains(rule.output_layouts, output_layout.name))
              return false;

          if (speakerLabel != rule.speakerLabel)
              return false;

          for (auto& channel_gain : rule.gains)
              if (!contains(output_layout.channelNames(), channel_gain.first))
                  return false;

          return true;
      }
  }  // namespace
  // End of block ==================================================================

};  // namespace ear
