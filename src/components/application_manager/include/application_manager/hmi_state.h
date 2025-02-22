/*
 * Copyright (c) 2015, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_HMI_STATE_H_
#define SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_HMI_STATE_H_

#include <iosfwd>
#include <list>
#include <memory>
#include "interfaces/MOBILE_API.h"
#include "utils/macro.h"

namespace application_manager {

class HmiState;
class ApplicationManager;
class Application;

typedef std::shared_ptr<HmiState> HmiStatePtr;
typedef int32_t WindowID;

/**
 * @brief The HmiState class
 *  Handle Hmi state of application (hmi level,
 *  audio streaming state, system context)
 *
 */
class HmiState {
  friend std::ostream& operator<<(std::ostream& os, const HmiState& src);

 public:
  /**
   * @brief The StateID enum describes state of application
   * If no events occurred STATE_ID_DEFAULT should be presented
   */
  enum StateID {
    STATE_ID_CURRENT = 1,
    STATE_ID_REGULAR = 2,
    STATE_ID_POSTPONED = 3,
    STATE_ID_PHONE_CALL = 4,
    STATE_ID_SAFETY_MODE = 5,
    STATE_ID_VR_SESSION = 6,
    STATE_ID_TTS_SESSION = 7,
    STATE_ID_VIDEO_STREAMING = 8,
    STATE_ID_NAVI_STREAMING = 9,
    STATE_ID_DEACTIVATE_HMI = 10,
    STATE_ID_AUDIO_SOURCE = 11,
    STATE_ID_EMBEDDED_NAVI = 12
  };

  /**
   * @brief HmiState constructor
   * @param app Application pointer
   * @param app_mngr Application manager
   */
  HmiState(std::shared_ptr<Application> app,
           const ApplicationManager& app_mngr);

  /**
   * @brief HmiState constructor
   * @param app Application pointer
   * @param app_mngr Application manager
   * @param state_id HMI state to assign
   */
  HmiState(std::shared_ptr<Application> app,
           const ApplicationManager& app_mngr,
           StateID state_id);

  virtual ~HmiState() {}

  /**
   * @brief setParent setup parent state
   * @param parent state to setup
   */
  void set_parent(HmiStatePtr parent);

  /**
   * @brief parent get parent state
   * @return parent state
   */
  const HmiStatePtr parent() const {
    return parent_;
  }

  /**
   * @brief hmi_level
   * @return return hmi level member
   */
  virtual mobile_apis::HMILevel::eType hmi_level() const {
    if (parent_) {
      if (mobile_apis::HMILevel::INVALID_ENUM == hmi_level_) {
        return parent_->hmi_level();
      }
      // Higher values correlate to lower states
      // (FULL = 0, LIMITED = 1, etc.)
      return std::max(parent_->max_hmi_level(), hmi_level_);
    }
    return hmi_level_;
  }

  /**
   * @brief max_hmi_level
   * @return return maximum hmi level for app
   */
  virtual mobile_apis::HMILevel::eType max_hmi_level() const {
    if (parent_) {
      return parent_->max_hmi_level();
    }
    return mobile_apis::HMILevel::HMI_FULL;
  }

  /**
   * @brief set_hmi_level set hmi_level member
   * @param hmi_level hmi level to setup
   */
  void set_hmi_level(mobile_apis::HMILevel::eType hmi_level) {
    hmi_level_ = hmi_level;
  }

  /**
   * @brief audio_streaming_state
   * @return return audio streaming state member
   */
  virtual mobile_apis::AudioStreamingState::eType audio_streaming_state()
      const {
    if (parent_) {
      if (mobile_apis::AudioStreamingState::INVALID_ENUM ==
          audio_streaming_state_) {
        return parent_->audio_streaming_state();
      }
      // Higher values correlate to lower states
      // (AUDIBLE = 0, ATTENUATED = 1, etc.)
      return std::max(parent_->max_audio_streaming_state(),
                      audio_streaming_state_);
    }
    return audio_streaming_state_;
  }

  /**
   * @brief max_audio_streaming_state
   * @return return maximum audio streaming state for app
   */
  virtual mobile_apis::AudioStreamingState::eType max_audio_streaming_state()
      const {
    if (parent_) {
      return parent_->max_audio_streaming_state();
    }
    return mobile_apis::AudioStreamingState::AUDIBLE;
  }

  /**
   * @brief video_streaming_state
   * @return return video streaming state member
   */
  virtual mobile_apis::VideoStreamingState::eType video_streaming_state()
      const {
    if (parent_) {
      if (mobile_apis::VideoStreamingState::INVALID_ENUM ==
          video_streaming_state_) {
        return parent_->video_streaming_state();
      }
      // Higher values correlate to lower states
      // (STREAMABLE = 0, NOT_STREAMABLE = 1)
      return std::max(parent_->max_video_streaming_state(),
                      video_streaming_state_);
    }
    return video_streaming_state_;
  }

  /**
   * @brief max_video_streaming_state
   * @return return maximum video streaming state for app
   */
  virtual mobile_apis::VideoStreamingState::eType max_video_streaming_state()
      const {
    if (parent_) {
      return parent_->max_video_streaming_state();
    }
    return mobile_apis::VideoStreamingState::STREAMABLE;
  }

  /**
   * @brief set_video_streaming_state set set_video_streaming_state member
   * @param video_state video_state to setup
   */
  virtual void set_video_streaming_state(
      mobile_apis::VideoStreamingState::eType video_state) {
    video_streaming_state_ = video_state;
  }

  /**
   * @brief set_audio_streaming_state set audio_streaming_state member
   * @param audio_state audio_state to setup
   */
  virtual void set_audio_streaming_state(
      mobile_apis::AudioStreamingState::eType audio_state) {
    audio_streaming_state_ = audio_state;
  }

  /**
   * @brief system_context
   * @return return system context member
   */
  virtual mobile_apis::SystemContext::eType system_context() const {
    // Parent's context should be used if not available for current state
    if (parent_ &&
        system_context_ == mobile_apis::SystemContext::INVALID_ENUM) {
      return parent_->system_context();
    }
    return system_context_;
  }

  /**
   * @brief set_system_context set system_context member
   * @param system_context system_context to setup
   */
  virtual void set_system_context(
      mobile_apis::SystemContext::eType system_context) {
    system_context_ = system_context;
  }

  /**
   * @brief state_id state type
   * @return return state type
   */
  StateID state_id() const {
    return state_id_;
  }

  /**
   * @brief set_state_id sets state id
   * @param state_id state id to setup
   */
  void set_state_id(StateID state_id) {
    state_id_ = state_id;
  }

  /**
   * @brief window_type getter for a window type for this type
   * @return current state window type
   */
  mobile_apis::WindowType::eType window_type() const;

  /**
   * @brief set_window_type sets current state window type
   * @param window_type new window type
   */
  void set_window_type(const mobile_apis::WindowType::eType window_type);

 protected:
  uint32_t hmi_app_id_;
  StateID state_id_;
  const ApplicationManager& app_mngr_;
  HmiStatePtr parent_;
  mobile_apis::WindowType::eType window_type_;
  mobile_apis::HMILevel::eType hmi_level_;
  mobile_apis::AudioStreamingState::eType audio_streaming_state_;
  mobile_apis::VideoStreamingState::eType video_streaming_state_;
  mobile_apis::SystemContext::eType system_context_;

 protected:
  /**
   * @brief is_navi_app check if app is navi
   * @return true if app is navi, otherwise return false
   */
  bool is_navi_app() const;

  /**
   * @brief is_projection_app check if app is projection
   * @return true if app is projection, otherwise return false
   */
  bool is_projection_app() const;

  /**
   * @brief is_media_app check if app is media
   * @return true if media_app, otherwise return false
   */
  bool is_media_app() const;

  /**
   * @brief is_voice_communicationn_app check if app is voice comunication
   * @return true if voice_communicationn_app, otherwise return false
   */
  bool is_voice_communication_app() const;

  /**
   * @brief is_mobile_projection_app checks if application of 'projection'
   * HMI type
   * @return True if application is of 'projection' HMI type, otherwise - false
   */
  bool is_mobile_projection_app() const;

  /**
   * @brief parent_hmi_level
   * @return return hmi level for parent state if available, otherwise return
   * HMI_NONE
   */
  mobile_apis::HMILevel::eType parent_hmi_level() const;

  /**
   * @brief parent_max_hmi_level
   * @return return maximum hmi level for parent state if available, otherwise
   * return HMI_FULL
   */
  mobile_apis::HMILevel::eType parent_max_hmi_level() const;

  /**
   * @brief parent_audio_state
   * @return return audio streaming state for parent state if available,
   * otherwise return NOT_AUDIBLE
   */
  mobile_apis::AudioStreamingState::eType parent_audio_state() const;

  /**
   * @brief parent_max_audio_state
   * @return return maximum audio streaming state for parent state if available,
   * otherwise return AUDIBLE
   */
  mobile_apis::AudioStreamingState::eType parent_max_audio_state() const;

  /**
   * @brief parent_video_state
   * @return return video streaming state for parent state if available,
   * otherwise return NOT_STREAMABLE
   */
  mobile_apis::VideoStreamingState::eType parent_video_state() const;

  /**
   * @brief parent_max_video_state
   * @return return maximum video streaming statefor parent state if available,
   * otherwise return STREAMABLE
   */
  mobile_apis::VideoStreamingState::eType parent_max_video_state() const;

 private:
  void operator=(const HmiState&);
};

/**
 * @brief The VRHmiState class implements logic of VR temporary state
 */
class VRHmiState : public HmiState {
 public:
  virtual mobile_apis::AudioStreamingState::eType audio_streaming_state()
      const OVERRIDE;
  virtual mobile_apis::AudioStreamingState::eType max_audio_streaming_state()
      const OVERRIDE;
  VRHmiState(std::shared_ptr<Application> app,
             const ApplicationManager& app_mngr);
};

/**
 * @brief The TTSHmiState class implements logic of TTS temporary state
 */
class TTSHmiState : public HmiState {
 public:
  TTSHmiState(std::shared_ptr<Application> app,
              const ApplicationManager& app_mngr);

  virtual mobile_apis::AudioStreamingState::eType audio_streaming_state()
      const OVERRIDE;

  virtual mobile_apis::AudioStreamingState::eType max_audio_streaming_state()
      const OVERRIDE;
};

/**
 * @brief The VideoStreamingState class implements logic of video streaming
 * temporary state
 */
class VideoStreamingHmiState : public HmiState {
 public:
  VideoStreamingHmiState(std::shared_ptr<Application> app,
                         const ApplicationManager& app_mngr);

  mobile_apis::VideoStreamingState::eType video_streaming_state()
      const OVERRIDE;

  mobile_apis::VideoStreamingState::eType max_video_streaming_state()
      const OVERRIDE;
};

/**
 * @brief The NaviStreamingHmiState class implements logic of navigation
 * streaming temporary state that is more specific than VideoStreamingHmiState
 */
class NaviStreamingHmiState : public VideoStreamingHmiState {
 public:
  NaviStreamingHmiState(std::shared_ptr<Application> app,
                        const ApplicationManager& app_mngr);

  mobile_apis::AudioStreamingState::eType audio_streaming_state()
      const OVERRIDE;

  mobile_apis::AudioStreamingState::eType max_audio_streaming_state()
      const OVERRIDE;
};

/**
 * @brief The PhoneCallHmiState class implements logic of PhoneCall temporary
 * state
 */
class PhoneCallHmiState : public HmiState {
 public:
  PhoneCallHmiState(std::shared_ptr<Application> app,
                    const ApplicationManager& app_mngr);

  mobile_apis::AudioStreamingState::eType audio_streaming_state()
      const OVERRIDE {
    return mobile_apis::AudioStreamingState::NOT_AUDIBLE;
  }

  mobile_apis::AudioStreamingState::eType max_audio_streaming_state()
      const OVERRIDE {
    return audio_streaming_state();
  }
};

/**
 * @brief The SafetyModeHmiState class implements logic of SafetyMode temporary
 * state
 */
class SafetyModeHmiState : public HmiState {
 public:
  SafetyModeHmiState(std::shared_ptr<Application> app,
                     const ApplicationManager& app_mngr);

  mobile_apis::AudioStreamingState::eType audio_streaming_state()
      const OVERRIDE {
    return mobile_apis::AudioStreamingState::NOT_AUDIBLE;
  }
  mobile_apis::VideoStreamingState::eType video_streaming_state()
      const OVERRIDE {
    return mobile_apis::VideoStreamingState::NOT_STREAMABLE;
  }

  mobile_apis::AudioStreamingState::eType max_audio_streaming_state()
      const OVERRIDE {
    return audio_streaming_state();
  }
  mobile_apis::VideoStreamingState::eType max_video_streaming_state()
      const OVERRIDE {
    return video_streaming_state();
  }
};

/**
 * @brief The DeactivateHMI class implements logic of DeactivateHMI temporary
 * state
 */
class DeactivateHMI : public HmiState {
 public:
  DeactivateHMI(std::shared_ptr<Application> app,
                const ApplicationManager& app_mngr);

  mobile_apis::HMILevel::eType hmi_level() const OVERRIDE;
  mobile_apis::AudioStreamingState::eType audio_streaming_state()
      const OVERRIDE {
    return mobile_apis::AudioStreamingState::NOT_AUDIBLE;
  }
  mobile_apis::VideoStreamingState::eType video_streaming_state()
      const OVERRIDE {
    return mobile_apis::VideoStreamingState::NOT_STREAMABLE;
  }

  mobile_apis::HMILevel::eType max_hmi_level() const OVERRIDE;
  mobile_apis::AudioStreamingState::eType max_audio_streaming_state()
      const OVERRIDE {
    return audio_streaming_state();
  }
  mobile_apis::VideoStreamingState::eType max_video_streaming_state()
      const OVERRIDE {
    return video_streaming_state();
  }
};

/**
 * @brief The AudioSource class implements logic of OnEventChanged(AUDIO_SOURCE)
 * temporary state
 */
class AudioSource : public HmiState {
 public:
  AudioSource(std::shared_ptr<Application> app,
              const ApplicationManager& app_mngr);

  mobile_apis::HMILevel::eType hmi_level() const OVERRIDE;
  mobile_apis::AudioStreamingState::eType audio_streaming_state()
      const OVERRIDE;
  mobile_apis::VideoStreamingState::eType video_streaming_state()
      const OVERRIDE;

  mobile_apis::HMILevel::eType max_hmi_level() const OVERRIDE;
  mobile_apis::AudioStreamingState::eType max_audio_streaming_state()
      const OVERRIDE;
  mobile_apis::VideoStreamingState::eType max_video_streaming_state()
      const OVERRIDE;

 private:
  bool keep_context_;
};

/**
 * @brief The EmbeddedNavi class implements logic of
 * OnEventChanged(EMBEDDED_NAVI)
 * temporary state
 */
class EmbeddedNavi : public HmiState {
 public:
  EmbeddedNavi(std::shared_ptr<Application> app,
               const ApplicationManager& app_mngr);

  mobile_apis::HMILevel::eType hmi_level() const OVERRIDE;
  mobile_apis::AudioStreamingState::eType audio_streaming_state()
      const OVERRIDE;
  mobile_apis::VideoStreamingState::eType video_streaming_state()
      const OVERRIDE;

  mobile_apis::HMILevel::eType max_hmi_level() const OVERRIDE;
  mobile_apis::AudioStreamingState::eType max_audio_streaming_state()
      const OVERRIDE;
  mobile_apis::VideoStreamingState::eType max_video_streaming_state()
      const OVERRIDE;
};

/**
 * @brief Outputs StateID to ostream in human readable format
 * @param os output stream to insert data to
 * @param src StateID value to output
 * @return os
 */
std::ostream& operator<<(std::ostream& os, const HmiState::StateID src);

/**
 * @brief Outputs HmiState to ostream in human readable format
 * @param os output stream to insert data to
 * @param src HmiState value to output
 * @return os
 */
std::ostream& operator<<(std::ostream& os, const HmiState& src);

}  // namespace application_manager

#endif  // SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_HMI_STATE_H_
