/*
 *  Copyright (C) 2017, The Linux Foundation. All rights reserved.
 *  Not a Contribution.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (C) 2016 The Android Open Source Project
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

/**
 * A2DP Codecs Configuration
 */

#define LOG_TAG "a2dp_codec"

#include "a2dp_codec_api.h"

#include <base/logging.h>
#include <inttypes.h>
#include <utils/Log.h>
#include <string.h>

#include "a2dp_aac.h"
#include "a2dp_sbc.h"
#include "a2dp_vendor.h"
#include "a2dp_vendor_aptx.h"
#include "a2dp_vendor_aptx_hd.h"
#include "a2dp_vendor_aptx_adaptive.h"
#include "a2dp_vendor_ldac.h"
#include "osi/include/log.h"
#include "a2dp_vendor_aptx_tws.h"
#include "bt_vendor_av.h"
/* The Media Type offset within the codec info byte array */
#define A2DP_MEDIA_TYPE_OFFSET 1

// Initializes the codec config.
// |codec_config| is the codec config to initialize.
// |codec_index| and |codec_priority| are the codec type and priority to use
// for the initialization.
bool mA2dp_offload_status = false;
bool mA2dp_offload_scrambling_support = false;
bool offload_capability = false;
bool sbc_offload = false;
bool aac_offload = false;
bool aptx_offload = false;
bool aptxhd_offload = false;
bool aptx_adaptive_offload = false;
bool ldac_offload = false;
bool aptxtws_offload = false;
static void init_btav_a2dp_codec_config(
    btav_a2dp_codec_config_t* codec_config, btav_a2dp_codec_index_t codec_index,
    btav_a2dp_codec_priority_t codec_priority) {
  memset(codec_config, 0, sizeof(btav_a2dp_codec_config_t));
  codec_config->codec_type = codec_index;
  codec_config->codec_priority = codec_priority;
}

A2dpCodecConfig::A2dpCodecConfig(btav_a2dp_codec_index_t codec_index,
                                 const std::string& name,
                                 btav_a2dp_codec_priority_t codec_priority)
    : codec_index_(codec_index),
      name_(name),
      default_codec_priority_(codec_priority) {
  setCodecPriority(codec_priority);

  init_btav_a2dp_codec_config(&codec_config_, codec_index_, codecPriority());
  init_btav_a2dp_codec_config(&codec_capability_, codec_index_,
                              codecPriority());
  init_btav_a2dp_codec_config(&codec_local_capability_, codec_index_,
                              codecPriority());
  init_btav_a2dp_codec_config(&codec_selectable_capability_, codec_index_,
                              codecPriority());
  init_btav_a2dp_codec_config(&codec_user_config_, codec_index_,
                              BTAV_A2DP_CODEC_PRIORITY_DEFAULT);
  init_btav_a2dp_codec_config(&codec_audio_config_, codec_index_,
                              BTAV_A2DP_CODEC_PRIORITY_DEFAULT);

  memset(ota_codec_config_, 0, sizeof(ota_codec_config_));
  memset(ota_codec_peer_capability_, 0, sizeof(ota_codec_peer_capability_));
  memset(ota_codec_peer_config_, 0, sizeof(ota_codec_peer_config_));
}

A2dpCodecConfig::~A2dpCodecConfig() {}

void A2dpCodecConfig::setCodecPriority(
    btav_a2dp_codec_priority_t codec_priority) {
  if (codec_priority == BTAV_A2DP_CODEC_PRIORITY_DEFAULT) {
    // Compute the default codec priority
    setDefaultCodecPriority();
  } else {
    codec_priority_ = codec_priority;
  }
}

void A2dpCodecConfig::setDefaultCodecPriority() {
  if (default_codec_priority_ != BTAV_A2DP_CODEC_PRIORITY_DEFAULT) {
    codec_priority_ = default_codec_priority_;
  } else {
    // Compute the default codec priority
    uint32_t priority = 1000 * (codec_index_ + 1) + 1;
    codec_priority_ = static_cast<btav_a2dp_codec_priority_t>(priority);
  }
}

A2dpCodecConfig* A2dpCodecConfig::createCodec(
    btav_a2dp_codec_index_t codec_index,
    btav_a2dp_codec_priority_t codec_priority) {
  LOG_DEBUG(LOG_TAG, "%s: codec %s", __func__, A2DP_CodecIndexStr(codec_index));

  A2dpCodecConfig* codec_config = nullptr;
  switch (codec_index) {
    case BTAV_A2DP_CODEC_INDEX_SOURCE_SBC:
      codec_config = new A2dpCodecConfigSbc(codec_priority);
      break;
#if (TWS_ENABLED == TRUE)
    case BTAV_VENDOR_A2DP_CODEC_INDEX_SINK_SBC:
#else
    case BTAV_A2DP_CODEC_INDEX_SINK_SBC:
#endif
      codec_config = new A2dpCodecConfigSbcSink(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_AAC:
      codec_config = new A2dpCodecConfigAac(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX:
      codec_config = new A2dpCodecConfigAptx(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_HD:
      codec_config = new A2dpCodecConfigAptxHd(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_ADAPTIVE:
      codec_config = new A2dpCodecConfigAptxAdaptive(codec_priority);
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_LDAC:
      codec_config = new A2dpCodecConfigLdac(codec_priority);
      break;
#if (TWS_ENABLED == TRUE)
    case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_TWS:
      codec_config = new A2dpCodecConfigAptxTWS(codec_priority);
      break;
    // Add a switch statement for each vendor-specific codec
    case BTAV_VENDOR_A2DP_CODEC_INDEX_MAX:
#else
    case BTAV_A2DP_CODEC_INDEX_MAX:
#endif
      break;
    default:
      break;
  }

  if (codec_config != nullptr) {
    if (!codec_config->init()) {
      delete codec_config;
      codec_config = nullptr;
    }
  }

  return codec_config;
}

bool A2dpCodecConfig::isValid() const { return true; }

bool A2dpCodecConfig::copyOutOtaCodecConfig(uint8_t* p_codec_info) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  // TODO: We should use a mechanism to verify codec config,
  // not codec capability.
  if (!A2DP_IsSourceCodecValid(ota_codec_config_)) {
    return false;
  }
  memcpy(p_codec_info, ota_codec_config_, sizeof(ota_codec_config_));
  return true;
}

btav_a2dp_codec_config_t A2dpCodecConfig::getCodecConfig() {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  // TODO: We should check whether the codec config is valid
  return codec_config_;
}

btav_a2dp_codec_config_t A2dpCodecConfig::getCodecCapability() {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  // TODO: We should check whether the codec capability is valid
  return codec_capability_;
}

btav_a2dp_codec_config_t A2dpCodecConfig::getCodecLocalCapability() {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  // TODO: We should check whether the codec capability is valid
  return codec_local_capability_;
}

btav_a2dp_codec_config_t A2dpCodecConfig::getCodecSelectableCapability() {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  // TODO: We should check whether the codec capability is valid
  return codec_selectable_capability_;
}

btav_a2dp_codec_config_t A2dpCodecConfig::getCodecUserConfig() {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  return codec_user_config_;
}

btav_a2dp_codec_config_t A2dpCodecConfig::getCodecAudioConfig() {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  return codec_audio_config_;
}

uint8_t A2dpCodecConfig::getAudioBitsPerSample() {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  switch (codec_config_.bits_per_sample) {
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16:
      return 16;
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24:
      return 24;
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32:
      return 32;
    case BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE:
      break;
  }
  return 0;
}

bool A2dpCodecConfig::isCodecConfigEmpty(
    const btav_a2dp_codec_config_t& codec_config) {
  return (
      (codec_config.codec_priority == BTAV_A2DP_CODEC_PRIORITY_DEFAULT) &&
      (codec_config.sample_rate == BTAV_A2DP_CODEC_SAMPLE_RATE_NONE) &&
      (codec_config.bits_per_sample == BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE) &&
      (codec_config.channel_mode == BTAV_A2DP_CODEC_CHANNEL_MODE_NONE) &&
      (codec_config.codec_specific_1 == 0) &&
      (codec_config.codec_specific_2 == 0) &&
      (codec_config.codec_specific_3 == 0) &&
      (codec_config.codec_specific_4 == 0));
}

bool A2dpCodecConfig::setCodecUserConfig(
    const btav_a2dp_codec_config_t& codec_user_config,
    const btav_a2dp_codec_config_t& codec_audio_config,
    const tA2DP_ENCODER_INIT_PEER_PARAMS* p_peer_params,
    const uint8_t* p_peer_codec_info, bool is_capability,
    uint8_t* p_result_codec_config, bool* p_restart_input,
    bool* p_restart_output, bool* p_config_updated) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  *p_restart_input = false;
  *p_restart_output = false;
  *p_config_updated = false;

  // Save copies of the current codec config, and the OTA codec config, so they
  // can be compared for changes.
  btav_a2dp_codec_config_t saved_codec_config = getCodecConfig();

  LOG_DEBUG(LOG_TAG, "%s: saved_codec_config: ", __func__);
  print_codec_parameters(saved_codec_config);

  print_codec_config(ota_codec_config_);
  uint8_t saved_ota_codec_config[AVDT_CODEC_SIZE];
  memcpy(saved_ota_codec_config, ota_codec_config_, sizeof(ota_codec_config_));
  uint8_t zero[AVDT_CODEC_SIZE] = {0};

  btav_a2dp_codec_config_t saved_codec_user_config = codec_user_config_;

  LOG_DEBUG(LOG_TAG, "%s: saved_codec_user_config: ", __func__);
  print_codec_parameters(saved_codec_user_config);

  codec_user_config_ = codec_user_config;
  btav_a2dp_codec_config_t saved_codec_audio_config = codec_audio_config_;

  LOG_DEBUG(LOG_TAG, "%s: saved_codec_audio_config: ", __func__);
  print_codec_parameters(saved_codec_audio_config);

  codec_audio_config_ = codec_audio_config;
  bool success =
      setCodecConfig(p_peer_codec_info, is_capability, p_result_codec_config);
  LOG_DEBUG(LOG_TAG, "%s: success: %d, is_capability:%d", __func__, success, is_capability);
  if (!success) {
    // Restore the local copy of the user and audio config
    codec_user_config_ = saved_codec_user_config;
    codec_audio_config_ = saved_codec_audio_config;
    return false;
  }

  //
  // The input (audio data) should be restarted if the audio format has changed
  //
  btav_a2dp_codec_config_t new_codec_config = getCodecConfig();

  LOG_DEBUG(LOG_TAG, "%s: new_codec_config: ", __func__);
  print_codec_parameters(new_codec_config);

  if ((saved_codec_config.sample_rate != new_codec_config.sample_rate) ||
      (saved_codec_config.bits_per_sample !=
       new_codec_config.bits_per_sample) ||
      (saved_codec_config.channel_mode != new_codec_config.channel_mode)) {
    *p_restart_input = true;
  }

  //
  // The output (the connection) should be restarted if OTA codec config
  // has changed and OTA codec has been built.
  //
  if (!A2DP_CodecEquals(saved_ota_codec_config, p_result_codec_config) &&
      memcmp(zero, saved_ota_codec_config, sizeof(zero))) {
    *p_restart_output = true;
  }

   LOG_DEBUG(LOG_TAG, "%s: *p_restart_input:%d, *p_restart_output:%d, *p_config_updated:%d",
                     __func__, *p_restart_input, *p_restart_output, *p_config_updated);
  bool encoder_restart_input = *p_restart_input;
  bool encoder_restart_output = *p_restart_output;
  bool encoder_config_updated = *p_config_updated;
  if (updateEncoderUserConfig(p_peer_params, &encoder_restart_input,
                              &encoder_restart_output,
                              &encoder_config_updated)) {
    LOG_DEBUG(LOG_TAG, "%s: updated encoder user config:", __func__);
    if (encoder_restart_input) *p_restart_input = true;
    if (encoder_restart_output) *p_restart_output = true;
    if (encoder_config_updated) *p_config_updated = true;
  }
  if (*p_restart_input || *p_restart_output) *p_config_updated = true;

  return true;
}

bool A2dpCodecConfig::codecConfigIsValid(
    const btav_a2dp_codec_config_t& codec_config) {
  return 
#if (TWS_ENABLED == TRUE)
        (codec_config.codec_type < BTAV_VENDOR_A2DP_CODEC_INDEX_MAX) &&
#else
        (codec_config.codec_type < BTAV_A2DP_CODEC_INDEX_MAX) &&
#endif
         (codec_config.sample_rate != BTAV_A2DP_CODEC_SAMPLE_RATE_NONE) &&
         (codec_config.bits_per_sample !=
          BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE) &&
         (codec_config.channel_mode != BTAV_A2DP_CODEC_CHANNEL_MODE_NONE);
}

std::string A2dpCodecConfig::codecConfig2Str(
    const btav_a2dp_codec_config_t& codec_config) {
  std::string result;

  if (!codecConfigIsValid(codec_config)) return "Invalid";

  result.append("Rate=");
  result.append(codecSampleRate2Str(codec_config.sample_rate));
  result.append(" Bits=");
  result.append(codecBitsPerSample2Str(codec_config.bits_per_sample));
  result.append(" Mode=");
  result.append(codecChannelMode2Str(codec_config.channel_mode));

  return result;
}

std::string A2dpCodecConfig::codecSampleRate2Str(
    btav_a2dp_codec_sample_rate_t codec_sample_rate) {
  std::string result;

  if (codec_sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_44100) {
    if (!result.empty()) result += "|";
    result += "44100";
  }
  if (codec_sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_48000) {
    if (!result.empty()) result += "|";
    result += "48000";
  }
  if (codec_sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_88200) {
    if (!result.empty()) result += "|";
    result += "88200";
  }
  if (codec_sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_96000) {
    if (!result.empty()) result += "|";
    result += "96000";
  }
  if (codec_sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_176400) {
    if (!result.empty()) result += "|";
    result += "176400";
  }
  if (codec_sample_rate & BTAV_A2DP_CODEC_SAMPLE_RATE_192000) {
    if (!result.empty()) result += "|";
    result += "192000";
  }
  if (result.empty()) {
    std::stringstream ss;
    ss << "UnknownSampleRate(0x" << std::hex << codec_sample_rate << ")";
    ss >> result;
  }

  return result;
}

std::string A2dpCodecConfig::codecBitsPerSample2Str(
    btav_a2dp_codec_bits_per_sample_t codec_bits_per_sample) {
  std::string result;

  if (codec_bits_per_sample & BTAV_A2DP_CODEC_BITS_PER_SAMPLE_16) {
    if (!result.empty()) result += "|";
    result += "16";
  }
  if (codec_bits_per_sample & BTAV_A2DP_CODEC_BITS_PER_SAMPLE_24) {
    if (!result.empty()) result += "|";
    result += "24";
  }
  if (codec_bits_per_sample & BTAV_A2DP_CODEC_BITS_PER_SAMPLE_32) {
    if (!result.empty()) result += "|";
    result += "32";
  }
  if (result.empty()) {
    std::stringstream ss;
    ss << "UnknownBitsPerSample(0x" << std::hex << codec_bits_per_sample << ")";
    ss >> result;
  }

  return result;
}

std::string A2dpCodecConfig::codecChannelMode2Str(
    btav_a2dp_codec_channel_mode_t codec_channel_mode) {
  std::string result;

  if (codec_channel_mode & BTAV_A2DP_CODEC_CHANNEL_MODE_MONO) {
    if (!result.empty()) result += "|";
    result += "MONO";
  }
  if (codec_channel_mode & BTAV_A2DP_CODEC_CHANNEL_MODE_STEREO) {
    if (!result.empty()) result += "|";
    result += "STEREO";
  }
  if (result.empty()) {
    std::stringstream ss;
    ss << "UnknownChannelMode(0x" << std::hex << codec_channel_mode << ")";
    ss >> result;
  }

  return result;
}

void A2dpCodecConfig::debug_codec_dump(int fd) {
  if(fd == -1) {
     std::string result;
     ALOGE("\nA2DP %s State:\n", name().c_str());
     ALOGE("  Priority: %d\n", codecPriority());
     ALOGE("  Encoder interval (ms): %" PRIu64 "\n", encoderIntervalMs());

     result = codecConfig2Str(getCodecConfig());
     ALOGE("  Config: %s\n", result.c_str());

     result = codecConfig2Str(getCodecSelectableCapability());
     ALOGE("  Selectable: %s\n", result.c_str());

     result = codecConfig2Str(getCodecLocalCapability());
     ALOGE("  Local capability: %s\n", result.c_str());

  }
  else{
     std::string result;
     dprintf(fd, "\nA2DP %s State:\n", name().c_str());
     dprintf(fd, "  Priority: %d\n", codecPriority());
     dprintf(fd, "  Encoder interval (ms): %" PRIu64 "\n", encoderIntervalMs());

     result = codecConfig2Str(getCodecConfig());
     dprintf(fd, "  Config: %s\n", result.c_str());

     result = codecConfig2Str(getCodecSelectableCapability());
     dprintf(fd, "  Selectable: %s\n", result.c_str());

     result = codecConfig2Str(getCodecLocalCapability());
     dprintf(fd, "  Local capability: %s\n", result.c_str());
  }
}

//
// Compares two codecs |lhs| and |rhs| based on their priority.
// Returns true if |lhs| has higher priority (larger priority value).
// If |lhs| and |rhs| have same priority, the unique codec index is used
// as a tie-breaker: larger codec index value means higher priority.
//
static bool compare_codec_priority(const A2dpCodecConfig* lhs,
                                   const A2dpCodecConfig* rhs) {
  if (lhs->codecPriority() > rhs->codecPriority()) return true;
  if (lhs->codecPriority() < rhs->codecPriority()) return false;
  return (lhs->codecIndex() > rhs->codecIndex());
}

A2dpCodecs::A2dpCodecs(
    const std::vector<btav_a2dp_codec_config_t>& codec_priorities)
    : current_codec_config_(nullptr) {
  for (auto config : codec_priorities) {
    codec_priorities_.insert(
        std::make_pair(config.codec_type, config.codec_priority));
  }
}

A2dpCodecs::~A2dpCodecs() {
  std::unique_lock<std::recursive_mutex> lock(codec_mutex_);
  for (const auto& iter : indexed_codecs_) {
    delete iter.second;
  }
  for (const auto& iter : disabled_codecs_) {
    delete iter.second;
  }
  lock.unlock();
}

bool A2dpCodecs::init(bool isMulticastEnabled, bool isShoEnabled) {
  LOG_DEBUG(LOG_TAG, "%s", __func__);
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
#if (TWS_ENABLED == TRUE)
  for (int i = BTAV_A2DP_CODEC_INDEX_MIN; i < BTAV_VENDOR_A2DP_CODEC_INDEX_MAX; i++) {
#else
  for (int i = BTAV_A2DP_CODEC_INDEX_MIN; i < BTAV_A2DP_CODEC_INDEX_MAX; i++) {
#endif
    btav_a2dp_codec_index_t codec_index =
        static_cast<btav_a2dp_codec_index_t>(i);

    if ((isMulticastEnabled == true) &&
        (codec_index != BTAV_A2DP_CODEC_INDEX_SOURCE_SBC)) {
      LOG_INFO(LOG_TAG, "%s: Selecting SBC codec as multicast is enabled",
               __func__);
      continue;
    }

    // Select the codec priority if explicitly configured
    btav_a2dp_codec_priority_t codec_priority =
        BTAV_A2DP_CODEC_PRIORITY_DEFAULT;
    auto cp_iter = codec_priorities_.find(codec_index);
    if (cp_iter != codec_priorities_.end()) {
      codec_priority = cp_iter->second;
    }

    A2dpCodecConfig* codec_config =
        A2dpCodecConfig::createCodec(codec_index, codec_priority);
    if (codec_config == nullptr) continue;

    if (codec_priority != BTAV_A2DP_CODEC_PRIORITY_DEFAULT) {
      LOG_INFO(LOG_TAG, "%s: updated %s codec priority to %d", __func__,
               codec_config->name().c_str(), codec_priority);
    }

    // Test if the codec is disabled
    if (codec_config->codecPriority() == BTAV_A2DP_CODEC_PRIORITY_DISABLED) {
      disabled_codecs_.insert(std::make_pair(codec_index, codec_config));
      continue;
    }

    indexed_codecs_.insert(std::make_pair(codec_index, codec_config));
#if (TWS_ENABLED == TRUE)
    if (codec_index < BTAV_VENDOR_A2DP_CODEC_INDEX_SOURCE_MAX) {
#else
    if (codec_index < BTAV_A2DP_CODEC_INDEX_SOURCE_MAX) {
#endif
      ordered_source_codecs_.push_back(codec_config);
      ordered_source_codecs_.sort(compare_codec_priority);
    } else {
      ordered_sink_codecs_.push_back(codec_config);
      ordered_sink_codecs_.sort(compare_codec_priority);
    }
  }

  if (ordered_source_codecs_.empty()) {
    LOG_ERROR(LOG_TAG, "%s: no Source codecs were initialized", __func__);
  } else {
    for (auto iter : ordered_source_codecs_) {
      LOG_INFO(LOG_TAG, "%s: initialized Source codec %s", __func__,
               iter->name().c_str());
    }
  }
  if (ordered_sink_codecs_.empty()) {
    LOG_ERROR(LOG_TAG, "%s: no Sink codecs were initialized", __func__);
  } else {
    for (auto iter : ordered_sink_codecs_) {
      LOG_INFO(LOG_TAG, "%s: initialized Sink codec %s", __func__,
               iter->name().c_str());
    }
  }

  return (!ordered_source_codecs_.empty() && !ordered_sink_codecs_.empty());
}

A2dpCodecConfig* A2dpCodecs::findSourceCodecConfig(
    const uint8_t* p_codec_info) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  btav_a2dp_codec_index_t codec_index = A2DP_SourceCodecIndex(p_codec_info);
#if (TWS_ENABLED == TRUE)
  if (codec_index == BTAV_VENDOR_A2DP_CODEC_INDEX_MAX) return nullptr;
#else
  if (codec_index == BTAV_A2DP_CODEC_INDEX_MAX) return nullptr;
#endif

  auto iter = indexed_codecs_.find(codec_index);
  if (iter == indexed_codecs_.end()) return nullptr;
  return iter->second;
}

bool A2dpCodecs::setCodecConfig(const uint8_t* p_peer_codec_info,
                                bool is_capability,
                                uint8_t* p_result_codec_config,
                                bool select_current_codec) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  A2dpCodecConfig* a2dp_codec_config = findSourceCodecConfig(p_peer_codec_info);
  if (a2dp_codec_config == nullptr) return false;
  if (!a2dp_codec_config->setCodecConfig(p_peer_codec_info, is_capability,
                                         p_result_codec_config)) {
    LOG_DEBUG(LOG_TAG, "%s: Codec config didn't set, return false", __func__);
    return false;
  }
  LOG_DEBUG(LOG_TAG, "%s: select_current_codec:%d", __func__,select_current_codec);
  if (select_current_codec) {
    current_codec_config_ = a2dp_codec_config;
  }
  return true;
}

bool A2dpCodecs::setCodecUserConfig(
    const btav_a2dp_codec_config_t& codec_user_config,
    const tA2DP_ENCODER_INIT_PEER_PARAMS* p_peer_params,
    const uint8_t* p_peer_sink_capabilities, uint8_t* p_result_codec_config,
    bool* p_restart_input, bool* p_restart_output, bool* p_config_updated) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  btav_a2dp_codec_config_t codec_audio_config;
  A2dpCodecConfig* a2dp_codec_config = nullptr;
  A2dpCodecConfig* last_codec_config = current_codec_config_;
  *p_restart_input = false;
  *p_restart_output = false;
  *p_config_updated = false;

  LOG_DEBUG(LOG_TAG, "%s: configuring codec_user_config: ", __func__);
  print_codec_parameters(codec_user_config);
#if (TWS_ENABLED == TRUE)
  if (codec_user_config.codec_type < BTAV_VENDOR_A2DP_CODEC_INDEX_MAX) {
#else
  if (codec_user_config.codec_type < BTAV_A2DP_CODEC_INDEX_MAX) {
#endif
    auto iter = indexed_codecs_.find(codec_user_config.codec_type);
    if (iter == indexed_codecs_.end()) goto fail;
    a2dp_codec_config = iter->second;
  } else {
    // Update the default codec
    a2dp_codec_config = current_codec_config_;
  }
  if (a2dp_codec_config == nullptr) goto fail;

  // Reuse the existing codec audio config
  codec_audio_config = a2dp_codec_config->getCodecAudioConfig();
  if (!a2dp_codec_config->setCodecUserConfig(
          codec_user_config, codec_audio_config, p_peer_params,
          p_peer_sink_capabilities, true, p_result_codec_config,
          p_restart_input, p_restart_output, p_config_updated)) {
    goto fail;
  }

  // Update the codec priorities, and eventually restart the connection
  // if a new codec needs to be selected.
  do {
    // Update the codec priority
    btav_a2dp_codec_priority_t old_priority =
        a2dp_codec_config->codecPriority();
    btav_a2dp_codec_priority_t new_priority = codec_user_config.codec_priority;
    a2dp_codec_config->setCodecPriority(new_priority);
    // Get the actual (recomputed) priority
    new_priority = a2dp_codec_config->codecPriority();

    // Check if there was no previous codec
    if (last_codec_config == nullptr) {
      current_codec_config_ = a2dp_codec_config;
      *p_restart_output = true;
      break;
    }

    // Check if the priority of the current codec was updated
    if (a2dp_codec_config == last_codec_config) {
      if (old_priority == new_priority) break;  // No change in priority

      *p_config_updated = true;
      if (new_priority < old_priority) {
        // The priority has become lower - restart the connection to
        // select a new codec.
        *p_restart_output = true;
      }
      break;
    }

    if (new_priority <= old_priority) {
      // No change in priority, or the priority has become lower.
      // This wasn't the current codec, so we shouldn't select a new codec.
      if (*p_restart_input || *p_restart_output ||
          (old_priority != new_priority)) {
        *p_config_updated = true;
      }
      *p_restart_input = false;
      *p_restart_output = false;
      break;
    }

    *p_config_updated = true;
    if (new_priority >= last_codec_config->codecPriority()) {
      // The new priority is higher than the current codec. Restart the
      // connection to select a new codec.
      current_codec_config_ = a2dp_codec_config;
      last_codec_config->setDefaultCodecPriority();
      *p_restart_output = true;
    }
  } while (false);
  ordered_source_codecs_.sort(compare_codec_priority);

  if (*p_restart_input || *p_restart_output) *p_config_updated = true;

  LOG_DEBUG(LOG_TAG,
            "%s: Configured: restart_input = %d restart_output = %d "
            "config_updated = %d",
            __func__, *p_restart_input, *p_restart_output, *p_config_updated);

  return true;

fail:
  current_codec_config_ = last_codec_config;
  return false;
}

bool A2dpCodecs::setCodecAudioConfig(
    const btav_a2dp_codec_config_t& codec_audio_config,
    const tA2DP_ENCODER_INIT_PEER_PARAMS* p_peer_params,
    const uint8_t* p_peer_sink_capabilities, uint8_t* p_result_codec_config,
    bool* p_restart_output, bool* p_config_updated) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  btav_a2dp_codec_config_t codec_user_config;
  A2dpCodecConfig* a2dp_codec_config = current_codec_config_;
  *p_restart_output = false;
  *p_config_updated = false;

  if (a2dp_codec_config == nullptr) return false;

  // Reuse the existing codec user config
  codec_user_config = a2dp_codec_config->getCodecUserConfig();
  bool restart_input = false;  // Flag ignored - input was just restarted
  if (!a2dp_codec_config->setCodecUserConfig(
          codec_user_config, codec_audio_config, p_peer_params,
          p_peer_sink_capabilities, true, p_result_codec_config, &restart_input,
          p_restart_output, p_config_updated)) {
    return false;
  }

  return true;
}

bool A2dpCodecs::setCodecOtaConfig(
    const uint8_t* p_ota_codec_config,
    const tA2DP_ENCODER_INIT_PEER_PARAMS* p_peer_params,
    uint8_t* p_result_codec_config, bool* p_restart_input,
    bool* p_restart_output, bool* p_config_updated) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  btav_a2dp_codec_index_t codec_type;
  btav_a2dp_codec_config_t codec_user_config;
  btav_a2dp_codec_config_t codec_audio_config;
  A2dpCodecConfig* a2dp_codec_config = nullptr;
  A2dpCodecConfig* last_codec_config = current_codec_config_;
  *p_restart_input = false;
  *p_restart_output = false;
  *p_config_updated = false;

  LOG_DEBUG(LOG_TAG, "%s:  p_ota_codec_config[%x:%x:%x:%x:%x:%x]", __func__,
                   p_ota_codec_config[1],  p_ota_codec_config[2],  p_ota_codec_config[3],
                   p_ota_codec_config[4],  p_ota_codec_config[5],  p_ota_codec_config[6]);
  // Check whether the codec config for the same codec is explicitly configured
  // by user configuration. If yes, then the OTA codec configuration is
  // ignored.
  codec_type = A2DP_SourceCodecIndex(p_ota_codec_config);
#if (TWS_ENABLED == TRUE)
  if (codec_type == BTAV_VENDOR_A2DP_CODEC_INDEX_MAX) {
#else
  if (codec_type == BTAV_A2DP_CODEC_INDEX_MAX) {
#endif
    LOG_WARN(LOG_TAG,
             "%s: ignoring peer OTA codec configuration: "
             "invalid codec",
             __func__);
    goto fail;  // Invalid codec
  } else {
    auto iter = indexed_codecs_.find(codec_type);
    if (iter == indexed_codecs_.end()) {
      LOG_WARN(LOG_TAG,
               "%s: cannot find codec configuration for peer OTA codec %s",
               __func__, A2DP_CodecName(p_ota_codec_config));
      goto fail;
    }
    a2dp_codec_config = iter->second;
  }
  current_codec_config_ = a2dp_codec_config;

  // Reuse the existing codec user config and codec audio config
  codec_audio_config = a2dp_codec_config->getCodecAudioConfig();
  codec_user_config = a2dp_codec_config->getCodecUserConfig();

  LOG_DEBUG(LOG_TAG, "%s: codec_user_config After fetching: ", __func__);
  print_codec_parameters(codec_user_config);

  LOG_DEBUG(LOG_TAG, "%s: codec_audio_config After fetching: ", __func__);
  print_codec_parameters(codec_audio_config);
  if (!a2dp_codec_config->setCodecUserConfig(
          codec_user_config, codec_audio_config, p_peer_params,
          p_ota_codec_config, false, p_result_codec_config, p_restart_input,
          p_restart_output, p_config_updated)) {
    LOG_WARN(LOG_TAG,
             "%s: cannot set codec configuration for peer OTA codec %s",
             __func__, A2DP_CodecName(p_ota_codec_config));
    goto fail;
  }
  CHECK(current_codec_config_ != nullptr);

  LOG_WARN(LOG_TAG,
               "%s: *p_restart_input: %d, *p_restart_output: %d",
               __func__, *p_restart_input, *p_restart_output);

  if (*p_restart_input || *p_restart_output) *p_config_updated = true;

  return true;

fail:
  current_codec_config_ = last_codec_config;
  return false;
}

bool A2dpCodecs::getCodecConfigAndCapabilities(
    btav_a2dp_codec_config_t* p_codec_config,
    std::vector<btav_a2dp_codec_config_t>* p_codecs_local_capabilities,
    std::vector<btav_a2dp_codec_config_t>* p_codecs_selectable_capabilities) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);

  if (current_codec_config_ != nullptr) {
    *p_codec_config = current_codec_config_->getCodecConfig();
  } else {
    btav_a2dp_codec_config_t codec_config;
    memset(&codec_config, 0, sizeof(codec_config));
    *p_codec_config = codec_config;
  }

  std::vector<btav_a2dp_codec_config_t> codecs_capabilities;
  for (auto codec : orderedSourceCodecs()) {
    codecs_capabilities.push_back(codec->getCodecLocalCapability());
  }
  *p_codecs_local_capabilities = codecs_capabilities;

  codecs_capabilities.clear();
  for (auto codec : orderedSourceCodecs()) {
    btav_a2dp_codec_config_t codec_capability =
        codec->getCodecSelectableCapability();
    // Don't add entries that cannot be used
    if ((codec_capability.sample_rate == BTAV_A2DP_CODEC_SAMPLE_RATE_NONE) ||
        (codec_capability.bits_per_sample ==
         BTAV_A2DP_CODEC_BITS_PER_SAMPLE_NONE) ||
        (codec_capability.channel_mode == BTAV_A2DP_CODEC_CHANNEL_MODE_NONE)) {
      continue;
    }
    codecs_capabilities.push_back(codec_capability);
  }
  *p_codecs_selectable_capabilities = codecs_capabilities;

  return true;
}

void A2dpCodecs::debug_codec_dump(int fd) {
  std::lock_guard<std::recursive_mutex> lock(codec_mutex_);
  dprintf(fd, "\nA2DP Codecs State:\n");

  // Print the current codec name
  if (current_codec_config_ != nullptr) {
    dprintf(fd, "  Current Codec: %s\n", current_codec_config_->name().c_str());
  } else {
    dprintf(fd, "  Current Codec: None\n");
  }

  // Print the codec-specific state
  for (auto codec_config : ordered_source_codecs_) {
    codec_config->debug_codec_dump(fd);
  }
}

tA2DP_CODEC_TYPE A2DP_GetCodecType(const uint8_t* p_codec_info) {
  return (tA2DP_CODEC_TYPE)(p_codec_info[AVDT_CODEC_TYPE_INDEX]);
}

bool A2DP_IsSourceCodecValid(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_IsSourceCodecValidSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_IsSourceCodecValidAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_IsVendorSourceCodecValid(p_codec_info);
    default:
      break;
  }

  return false;
}

bool A2DP_IsSinkCodecValid(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_IsSinkCodecValidSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_IsSinkCodecValidAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_IsVendorSinkCodecValid(p_codec_info);
    default:
      break;
  }

  return false;
}

bool A2DP_IsPeerSourceCodecValid(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_IsPeerSourceCodecValidSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_IsPeerSourceCodecValidAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_IsVendorPeerSourceCodecValid(p_codec_info);
    default:
      break;
  }

  return false;
}

bool A2DP_IsPeerSinkCodecValid(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_IsPeerSinkCodecValidSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_IsPeerSinkCodecValidAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_IsVendorPeerSinkCodecValid(p_codec_info);
    default:
      break;
  }

  return false;
}

bool A2DP_IsSinkCodecSupported(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_IsSinkCodecSupportedSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_IsSinkCodecSupportedAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_IsVendorSinkCodecSupported(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return false;
}

bool A2DP_IsPeerSourceCodecSupported(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_IsPeerSourceCodecSupportedSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_IsPeerSourceCodecSupportedAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_IsVendorPeerSourceCodecSupported(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return false;
}

void A2DP_InitDefaultCodec(uint8_t* p_codec_info) {
  A2DP_InitDefaultCodecSbc(p_codec_info);
}

tA2DP_STATUS A2DP_BuildSrc2SinkConfig(const uint8_t* p_src_cap,
                                      uint8_t* p_pref_cfg) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_src_cap);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_BuildSrc2SinkConfigSbc(p_src_cap, p_pref_cfg);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_BuildSrc2SinkConfigAac(p_src_cap, p_pref_cfg);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorBuildSrc2SinkConfig(p_src_cap, p_pref_cfg);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return A2DP_NS_CODEC_TYPE;
}

bool A2DP_UsesRtpHeader(bool content_protection_enabled,
                        const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  if (codec_type != A2DP_MEDIA_CT_NON_A2DP) return true;

  return A2DP_VendorUsesRtpHeader(content_protection_enabled, p_codec_info);
}

uint8_t A2DP_GetMediaType(const uint8_t* p_codec_info) {
  uint8_t media_type = (p_codec_info[A2DP_MEDIA_TYPE_OFFSET] >> 4) & 0x0f;
  return media_type;
}

const char* A2DP_CodecName(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_CodecNameSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_CodecNameAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorCodecName(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return "UNKNOWN CODEC";
}

bool A2DP_CodecTypeEquals(const uint8_t* p_codec_info_a,
                          const uint8_t* p_codec_info_b) {
  tA2DP_CODEC_TYPE codec_type_a = A2DP_GetCodecType(p_codec_info_a);
  tA2DP_CODEC_TYPE codec_type_b = A2DP_GetCodecType(p_codec_info_b);

  if (codec_type_a != codec_type_b) return false;

  switch (codec_type_a) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_CodecTypeEqualsSbc(p_codec_info_a, p_codec_info_b);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_CodecTypeEqualsAac(p_codec_info_a, p_codec_info_b);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorCodecTypeEquals(p_codec_info_a, p_codec_info_b);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type_a);
  return false;
}

bool A2DP_CodecEquals(const uint8_t* p_codec_info_a,
                      const uint8_t* p_codec_info_b) {
  tA2DP_CODEC_TYPE codec_type_a = A2DP_GetCodecType(p_codec_info_a);
  tA2DP_CODEC_TYPE codec_type_b = A2DP_GetCodecType(p_codec_info_b);

  if (codec_type_a != codec_type_b) return false;

  switch (codec_type_a) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_CodecEqualsSbc(p_codec_info_a, p_codec_info_b);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_CodecEqualsAac(p_codec_info_a, p_codec_info_b);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorCodecEquals(p_codec_info_a, p_codec_info_b);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type_a);
  return false;
}

int A2DP_GetTrackBitRate(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetBitRate(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return -1;
}

int A2DP_GetTrackSampleRate(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_GetTrackSampleRateSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_GetTrackSampleRateAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetTrackSampleRate(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return -1;
}

int A2DP_GetTrackChannelCount(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_GetTrackChannelCountSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_GetTrackChannelCountAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetTrackChannelCount(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return -1;
}

int A2DP_GetTrackBitsPerSample(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return 16;
    case A2DP_MEDIA_CT_AAC:
      return 16;
    case A2DP_MEDIA_CT_NON_A2DP: {
      uint32_t vendor_id = A2DP_VendorCodecGetVendorId(p_codec_info);
      uint16_t codec_id = A2DP_VendorCodecGetCodecId(p_codec_info);
      // Check for aptX
      if (vendor_id == A2DP_APTX_VENDOR_ID &&
          codec_id == A2DP_APTX_CODEC_ID_BLUETOOTH) {
        return 16;
      }

      // Check for aptX-HD
      if (vendor_id == A2DP_APTX_HD_VENDOR_ID &&
          codec_id == A2DP_APTX_HD_CODEC_ID_BLUETOOTH) {
        return 24;
      }

      // Check for LDAC
      if (vendor_id == A2DP_LDAC_VENDOR_ID && codec_id == A2DP_LDAC_CODEC_ID) {
        return 32;
      }
    }
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return -1;
}

int A2DP_GetSinkTrackChannelType(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_GetSinkTrackChannelTypeSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_GetSinkTrackChannelTypeAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetSinkTrackChannelType(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return -1;
}

int A2DP_GetSinkFramesCountToProcess(uint64_t time_interval_ms,
                                     const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_GetSinkFramesCountToProcessSbc(time_interval_ms,
                                                 p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_GetSinkFramesCountToProcessAac(time_interval_ms,
                                                 p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetSinkFramesCountToProcess(time_interval_ms,
                                                    p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return -1;
}

bool A2DP_GetPacketTimestamp(const uint8_t* p_codec_info, const uint8_t* p_data,
                             uint32_t* p_timestamp) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_GetPacketTimestampSbc(p_codec_info, p_data, p_timestamp);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_GetPacketTimestampAac(p_codec_info, p_data, p_timestamp);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetPacketTimestamp(p_codec_info, p_data, p_timestamp);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return false;
}

bool A2DP_BuildCodecHeader(const uint8_t* p_codec_info, BT_HDR* p_buf,
                           uint16_t frames_per_packet) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_BuildCodecHeaderSbc(p_codec_info, p_buf, frames_per_packet);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_BuildCodecHeaderAac(p_codec_info, p_buf, frames_per_packet);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorBuildCodecHeader(p_codec_info, p_buf,
                                         frames_per_packet);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return false;
}

const tA2DP_ENCODER_INTERFACE* A2DP_GetEncoderInterface(
    const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_GetEncoderInterfaceSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_GetEncoderInterfaceAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorGetEncoderInterface(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return NULL;
}

bool A2DP_AdjustCodec(uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_AdjustCodecSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_AdjustCodecAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorAdjustCodec(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return false;
}

btav_a2dp_codec_index_t A2DP_SourceCodecIndex(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_SourceCodecIndexSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_SourceCodecIndexAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorSourceCodecIndex(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
#if (TWS_ENABLED == TRUE)
  return (btav_a2dp_codec_index_t)BTAV_VENDOR_A2DP_CODEC_INDEX_MAX;
#else
  return BTAV_A2DP_CODEC_INDEX_MAX;
#endif
}

const char* A2DP_CodecIndexStr(btav_a2dp_codec_index_t codec_index) {
  switch (codec_index) {
    case BTAV_A2DP_CODEC_INDEX_SOURCE_SBC:
      return A2DP_CodecIndexStrSbc();
#if (TWS_ENABLED == TRUE)
    case (btav_a2dp_codec_index_t)BTAV_VENDOR_A2DP_CODEC_INDEX_SINK_SBC:
#else
    case BTAV_A2DP_CODEC_INDEX_SINK_SBC:
#endif
      return A2DP_CodecIndexStrSbcSink();
    case BTAV_A2DP_CODEC_INDEX_SOURCE_AAC:
      return A2DP_CodecIndexStrAac();
    default:
      break;
  }

#if (TWS_ENABLED == TRUE)
  if (codec_index < BTAV_VENDOR_A2DP_CODEC_INDEX_MAX)
#else
  if (codec_index < BTAV_A2DP_CODEC_INDEX_MAX)
#endif
    return A2DP_VendorCodecIndexStr(codec_index);

  return "UNKNOWN CODEC INDEX";
}

bool A2DP_InitCodecConfig(btav_a2dp_codec_index_t codec_index,
                          tAVDT_CFG* p_cfg) {
  LOG_VERBOSE(LOG_TAG, "%s: codec %s", __func__,
              A2DP_CodecIndexStr(codec_index));

  /* Default: no content protection info */
  p_cfg->num_protect = 0;
  p_cfg->protect_info[0] = 0;

  switch (codec_index) {
    case BTAV_A2DP_CODEC_INDEX_SOURCE_SBC:
      return A2DP_InitCodecConfigSbc(p_cfg);
#if (TWS_ENABLED == TRUE)
    case BTAV_VENDOR_A2DP_CODEC_INDEX_SINK_SBC:
#else
    case BTAV_A2DP_CODEC_INDEX_SINK_SBC:
#endif
      return A2DP_InitCodecConfigSbcSink(p_cfg);
    case BTAV_A2DP_CODEC_INDEX_SOURCE_AAC:
      return A2DP_InitCodecConfigAac(p_cfg);
    default:
      break;
  }

#if (TWS_ENABLED == TRUE)
  if (codec_index < BTAV_VENDOR_A2DP_CODEC_INDEX_MAX)
#else
  if (codec_index < BTAV_A2DP_CODEC_INDEX_MAX)
#endif
    return A2DP_VendorInitCodecConfig(codec_index, p_cfg);

  return false;
}

void A2DP_SetOffloadStatus(bool offload_status, char *offload_cap, bool scrambling_support) {
  //char value[PROPERTY_VALUE_MAX] = {'\0'};
  char *tok = NULL;
  char *tmp_token = NULL;
  LOG_INFO(LOG_TAG,"A2dp_SetOffloadStatus:status = %d",
                     offload_status);
  mA2dp_offload_status = offload_status;
  offload_capability = true;
  if (strcmp(offload_cap,"false") == 0) offload_capability = false;

  if (mA2dp_offload_status) {
    tok = strtok_r((char*)offload_cap, "-", &tmp_token);
    while (tok != NULL)
    {
      if (strcmp(tok,"sbc") == 0) {
        LOG_INFO(LOG_TAG,"%s: SBC offload supported",__func__);
        sbc_offload = true;
      } else if (strcmp(tok,"aptx") == 0) {
        LOG_INFO(LOG_TAG,"%s: aptX offload supported",__func__);
        aptx_offload = true;
      } else if (strcmp(tok,"aac") == 0) {
        LOG_INFO(LOG_TAG,"%s: AAC offload supported",__func__);
        aac_offload = TRUE;
      } else if (strcmp(tok,"aptxhd") == 0) {
        LOG_INFO(LOG_TAG,"%s: APTXHD offload supported",__func__);
        aptxhd_offload = TRUE;
      } else if (strcmp(tok,"aptxadaptive") == 0) {
        LOG_INFO(LOG_TAG,"%s: APTXHD offload supported",__func__);
        aptx_adaptive_offload = TRUE;
      } else if (strcmp(tok,"ldac") == 0) {
        LOG_INFO(LOG_TAG,"%s: ldac offload supported",__func__);
        ldac_offload = TRUE;
      } else if (strcmp(tok,"aptxtws") == 0) {
#if (TWS_ENABLED == TRUE)
        LOG_INFO(LOG_TAG,"%s: APTXTWS offload supported",__func__);
        aptxtws_offload = TRUE;
#else
        LOG_INFO(LOG_TAG,"%s: APTXTWS is not supported in host",__func__);
        aptxtws_offload = FALSE;
#endif
      }
      tok = strtok_r(NULL, "-", &tmp_token);
    };
  }
  mA2dp_offload_scrambling_support = scrambling_support;
}

bool A2DP_GetOffloadStatus() {
  return mA2dp_offload_status;
}

bool A2DP_IsScramblingSupported() {
  return mA2dp_offload_scrambling_support;
}

bool A2DP_IsCodecEnabledInOffload(btav_a2dp_codec_index_t codec_index) {
  bool codec_status = false;
  if (offload_capability) {
    switch (codec_index) {
    case BTAV_A2DP_CODEC_INDEX_SOURCE_SBC:
      codec_status = sbc_offload;
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_AAC:
      codec_status = aac_offload;
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX:
      codec_status = aptx_offload;
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_HD:
      codec_status = aptxhd_offload;
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_ADAPTIVE:
      codec_status = aptx_adaptive_offload;
      break;
    case BTAV_A2DP_CODEC_INDEX_SOURCE_LDAC:
      if (!ldac_offload)
          LOG_INFO(LOG_TAG,"LDAC not enabled in offload currently");
      codec_status = ldac_offload;
      break;
#if (TWS_ENABLED == TRUE)
    case BTAV_A2DP_CODEC_INDEX_SOURCE_APTX_TWS:
      codec_status = aptxtws_offload;
      break;
    case BTAV_VENDOR_A2DP_CODEC_INDEX_SOURCE_MAX:
    case BTAV_VENDOR_A2DP_CODEC_INDEX_SINK_MAX:
#else
    case BTAV_A2DP_CODEC_INDEX_SOURCE_MAX:
    case BTAV_A2DP_CODEC_INDEX_SINK_MAX:
#endif
    default:
      break;
    }
  } else {
    if (codec_index != BTAV_A2DP_CODEC_INDEX_SOURCE_LDAC &&
#if (TWS_ENABLED == TRUE)
      codec_index != BTAV_VENDOR_A2DP_CODEC_INDEX_SINK_MAX &&
      codec_index != BTAV_VENDOR_A2DP_CODEC_INDEX_SOURCE_MAX
#else
      codec_index != BTAV_A2DP_CODEC_INDEX_SINK_MAX &&
      codec_index != BTAV_A2DP_CODEC_INDEX_SOURCE_MAX
#endif
    )
      LOG_INFO(LOG_TAG,"SplitA2dp enabled, but offload capability not set");
      return true;
  }
  return codec_status;
}

bool A2DP_DumpCodecInfo(const uint8_t* p_codec_info) {
  tA2DP_CODEC_TYPE codec_type = A2DP_GetCodecType(p_codec_info);

  LOG_VERBOSE(LOG_TAG, "%s: codec_type = 0x%x", __func__, codec_type);

  switch (codec_type) {
    case A2DP_MEDIA_CT_SBC:
      return A2DP_DumpCodecInfoSbc(p_codec_info);
    case A2DP_MEDIA_CT_AAC:
      return A2DP_DumpCodecInfoAac(p_codec_info);
    case A2DP_MEDIA_CT_NON_A2DP:
      return A2DP_VendorDumpCodecInfo(p_codec_info);
    default:
      break;
  }

  LOG_ERROR(LOG_TAG, "%s: unsupported codec type 0x%x", __func__, codec_type);
  return false;
}
void print_codec_config(uint8_t codec_arry[]) {
   for(int i = 0; i < AVDT_CODEC_SIZE; i++)
   {
      LOG_INFO(LOG_TAG, "%s: codec_arry[%d] = %d", __func__, i, codec_arry[i]);
   }
}

void print_codec_parameters(btav_a2dp_codec_config_t config) {
    LOG_DEBUG(
     LOG_TAG,
     "codec_type=%d codec_priority=%d "
     "sample_rate=0x%x bits_per_sample=0x%x "
     "channel_mode=0x%x codec_specific_1=%" PRIi64
     " "
     "codec_specific_2=%" PRIi64
     " "
     "codec_specific_3=%" PRIi64
     " "
     "codec_specific_4=%" PRIi64,
     config.codec_type, config.codec_priority,
     config.sample_rate, config.bits_per_sample,
     config.channel_mode, config.codec_specific_1,
     config.codec_specific_2, config.codec_specific_3,
     config.codec_specific_4);
}

