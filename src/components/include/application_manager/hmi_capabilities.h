/*
 * Copyright (c) 2016, Ford Motor Company
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

#ifndef SRC_COMPONENTS_INCLUDE_APPLICATION_MANAGER_HMI_CAPABILITIES_H_
#define SRC_COMPONENTS_INCLUDE_APPLICATION_MANAGER_HMI_CAPABILITIES_H_

#include "application_manager/hmi_language_handler.h"
#include "interfaces/HMI_API.h"
#include "interfaces/MOBILE_API.h"
#include "json/json.h"
#include "resumption/last_state_wrapper.h"
#include "smart_objects/smart_object.h"
#include "utils/macro.h"

namespace application_manager {
class ApplicationManager;

class HMICapabilities {
 public:
  /**
   * @brief Class destructor
   */
  virtual ~HMICapabilities() {}

  /**
   * @brief return component which follows for correctness of
   * languages
   * @return HMI language handler
   */
  virtual HMILanguageHandler& get_hmi_language_handler() = 0;

  /**
   * @brief Checks if image type(Static/Dynamic) requested by
   * Mobile Device is supported on current HMI.
   * @param image_type received type of image from Enum.
   * @return Bool true if supported
   */
  virtual bool VerifyImageType(const int32_t image_type) const = 0;

  /**
   * @brief Checks if all HMI capabilities received
   * @return TRUE if all information received, otherwise FALSE
   */
  virtual bool is_vr_cooperating() const = 0;
  virtual void set_is_vr_cooperating(const bool value) = 0;

  virtual bool is_tts_cooperating() const = 0;
  virtual void set_is_tts_cooperating(const bool value) = 0;

  virtual bool is_ui_cooperating() const = 0;
  virtual void set_is_ui_cooperating(const bool value) = 0;

  virtual bool is_navi_cooperating() const = 0;
  virtual void set_is_navi_cooperating(const bool value) = 0;

  virtual bool is_ivi_cooperating() const = 0;
  virtual void set_is_ivi_cooperating(const bool value) = 0;

  virtual bool is_rc_cooperating() const = 0;
  virtual void set_is_rc_cooperating(const bool value) = 0;

  /**
   * @brief Interface used to store information about software version of the
   * target
   * @param ccpu_version Received system/hmi software version
   */
  virtual void set_ccpu_version(const std::string& ccpu_version) = 0;

  /**
   * @brief Returns software version of the target
   * @return string representation of software version if supported, otherwise
   * empty string
   */
  virtual const std::string& ccpu_version() const = 0;

  /**
   * @brief Interface used to store information about hardware version of the
   * target
   * @param hardware_version Received system/hmi hardware version
   */
  virtual void set_hardware_version(const std::string& hardware_version) = 0;

  /**
   * @brief Returns hardware version of the target
   * @return string representation of hardware version if supported, otherwise
   * empty string
   */
  virtual const std::string& hardware_version() const = 0;

  /**
   * @brief Retrieves if mixing audio is supported by HMI
   * (ie recording TTS command and playing audio)
   * @return Current state of the mixing audio flag
   */
  virtual bool attenuated_supported() const = 0;

  /**
   * @brief Sets state for mixing audio
   * @param state New state to be set
   */
  virtual void set_attenuated_supported(const bool state) = 0;

  /**
   * @brief Retrieves currently active UI language
   * @return Currently active UI language
   */
  virtual const hmi_apis::Common_Language::eType active_ui_language() const = 0;

  /**
   * @brief Sets currently active UI language
   * @param language Currently active UI language
   */
  virtual void set_active_ui_language(
      const hmi_apis::Common_Language::eType language) = 0;

  /**
   * @brief Retrieves UI supported languages
   * @return Currently supported UI languages
   */
  virtual const smart_objects::SmartObjectSPtr ui_supported_languages()
      const = 0;

  /**
   * @brief Sets supported UI languages
   * @param supported_languages Supported UI languages
   */
  virtual void set_ui_supported_languages(
      const smart_objects::SmartObject& supported_languages) = 0;

  /**
   * @brief Retrieves currently active VR language
   * @return Currently active VR language
   */
  virtual const hmi_apis::Common_Language::eType active_vr_language() const = 0;

  /**
   * @brief Sets currently active VR language
   * @param language Currently active VR language
   */
  virtual void set_active_vr_language(
      const hmi_apis::Common_Language::eType language) = 0;

  /**
   * @brief Retrieves VR supported languages
   * @return Currently supported VR languages
   */
  virtual const smart_objects::SmartObjectSPtr vr_supported_languages()
      const = 0;

  /**
   * @brief Sets supported VR languages
   * @param supported_languages Supported VR languages
   */
  virtual void set_vr_supported_languages(
      const smart_objects::SmartObject& supported_languages) = 0;

  /**
   * @brief Retrieves currently active TTS language
   * @return Currently active TTS language
   */
  virtual const hmi_apis::Common_Language::eType active_tts_language()
      const = 0;

  /**
   * @brief Sets currently active TTS language
   * @param language Currently active TTS language
   */
  virtual void set_active_tts_language(
      const hmi_apis::Common_Language::eType language) = 0;

  /**
   * @brief Retrieves TTS  supported languages
   * @return Currently supported TTS languages
   */
  virtual const smart_objects::SmartObjectSPtr tts_supported_languages()
      const = 0;

  /**
   * @brief Sets supported TTS languages
   * @param supported_languages Supported TTS languages
   */
  virtual void set_tts_supported_languages(
      const smart_objects::SmartObject& supported_languages) = 0;

  /**
   * @brief Retrieves information about the display capabilities
   * @return Currently supported display capabilities
   */
  virtual const smart_objects::SmartObjectSPtr display_capabilities() const = 0;

  /**
   * @brief Sets supported display capabilities
   * @param display_capabilities supported display capabilities
   */
  virtual void set_display_capabilities(
      const smart_objects::SmartObject& display_capabilities) = 0;

  /**
   * @brief Retrieves information about the display capability
   * @return Currently supported display capability
   */
  virtual const smart_objects::SmartObjectSPtr system_display_capabilities()
      const = 0;

  /**
   * @brief Sets supported display capability
   * @param display_capabilities supported display capability
   */
  virtual void set_system_display_capabilities(
      const smart_objects::SmartObject& display_capabilities) = 0;

  /**
   * @brief Retrieves information about the HMI zone capabilities
   * @return Currently supported HMI zone capabilities
   */
  virtual const smart_objects::SmartObjectSPtr hmi_zone_capabilities()
      const = 0;

  /**
   * @brief Sets supported HMI zone capabilities
   * @param hmi_zone_capabilities supported HMI zone capabilities
   */
  virtual void set_hmi_zone_capabilities(
      const smart_objects::SmartObject& hmi_zone_capabilities) = 0;

  /**
   * @brief Retrieves information about the SoftButton's capabilities
   * @return Currently supported SoftButton's capabilities
   */
  virtual const smart_objects::SmartObjectSPtr soft_button_capabilities()
      const = 0;

  /**
   * @brief Sets supported SoftButton's capabilities
   * @param soft_button_capabilities supported SoftButton's capabilities
   */
  virtual void set_soft_button_capabilities(
      const smart_objects::SmartObject& soft_button_capabilities) = 0;

  /**
   * @brief Retrieves information about the Button's capabilities
   * @return Currently supported Button's capabilities
   */
  virtual const smart_objects::SmartObjectSPtr button_capabilities() const = 0;

  /**
   * @brief Sets supported Button's capabilities
   * @param button_capabilities supported Button's capabilities
   */
  virtual void set_button_capabilities(
      const smart_objects::SmartObject& button_capabilities) = 0;

  /**
   * @brief Sets supported speech capabilities
   * @param speech_capabilities supported speech capabilities
   */
  virtual void set_speech_capabilities(
      const smart_objects::SmartObject& speech_capabilities) = 0;

  /**
   * @brief Retrieves information about the speech capabilities
   * @return Currently supported speech capabilities
   */
  virtual const smart_objects::SmartObjectSPtr speech_capabilities() const = 0;

  /**
   * @brief Sets supported VR capabilities
   * @param vr_capabilities supported VR capabilities
   */
  virtual void set_vr_capabilities(
      const smart_objects::SmartObject& vr_capabilities) = 0;

  /**
   * @brief Retrieves information about the VR capabilities
   * @return Currently supported VR capabilities
   */
  virtual const smart_objects::SmartObjectSPtr vr_capabilities() const = 0;

  /**
   * @brief Sets supported audio_pass_thru capabilities
   * @param vr_capabilities supported audio_pass_thru capabilities
   */
  virtual void set_audio_pass_thru_capabilities(
      const smart_objects::SmartObject& audio_pass_thru_capabilities) = 0;

  /**
   * @brief Retrieves information about the audio_pass_thru capabilities
   * @return Currently supported audio_pass_thru capabilities
   */
  virtual const smart_objects::SmartObjectSPtr audio_pass_thru_capabilities()
      const = 0;

  /**
   * @brief Sets supported pcm_stream capabilities
   * @param supported pcm_stream capabilities
   */
  virtual void set_pcm_stream_capabilities(
      const smart_objects::SmartObject& pcm_stream_capabilities) = 0;

  /**
   * @brief Retrieves information about the pcm_stream capabilities
   * @return Currently supported pcm_streaming capabilities
   */
  virtual const smart_objects::SmartObjectSPtr pcm_stream_capabilities()
      const = 0;

  /**
   * @brief Retrieves information about the preset bank capabilities
   * @return Currently supported preset bank capabilities
   */
  virtual const smart_objects::SmartObjectSPtr preset_bank_capabilities()
      const = 0;

  /**
   * @brief Sets supported preset bank capabilities
   * @param soft_button_capabilities supported preset bank capabilities
   */
  virtual void set_preset_bank_capabilities(
      const smart_objects::SmartObject& preset_bank_capabilities) = 0;

  /**
   * @brief Sets vehicle information(make, model, modelYear)
   * @param vehicle_type vehicle information
   */
  virtual void set_vehicle_type(
      const smart_objects::SmartObject& vehicle_type) = 0;

  /**
   * @brief Retrieves vehicle information(make, model, modelYear)
   * @param vehicle_type Current vehicle information
   */
  virtual const smart_objects::SmartObjectSPtr vehicle_type() const = 0;

  /**
   * @brief Retrieves information about the prerecorded speech
   * @return Currently supported prerecorded speech
   */
  virtual const smart_objects::SmartObjectSPtr prerecorded_speech() const = 0;

  /**
   * @brief Sets supported prerecorded speech
   * @param prerecorded_speech supported prerecorded speech
   */
  virtual void set_prerecorded_speech(
      const smart_objects::SmartObject& prerecorded_speech) = 0;

  /**
   * @brief Interface used to store information if navigation
   * supported by the system
   * @param supported Indicates if navigation supported by the system
   */
  virtual void set_navigation_supported(const bool supported) = 0;

  /**
   * @brief Retrieves information if navi supported by the system
   * @return TRUE if it supported, otherwise FALSE
   */
  virtual bool navigation_supported() const = 0;

  /**
   * @brief Interface used to store information if phone call
   * supported by the system
   * @param supported Indicates if navigation supported by the sustem
   */
  virtual void set_phone_call_supported(const bool supported) = 0;

  /**
   * @brief Retrieves information if phone call supported by the system
   * @return TRUE if it supported, otherwise FALSE
   */
  virtual bool phone_call_supported() const = 0;

  /**
   * @brief Interface to store whether HMI supports video streaming
   * @param supported Indicates whether video streaming is supported by HMI
   */
  virtual void set_video_streaming_supported(const bool supported) = 0;

  /**
   * @brief Retrieves whether HMI supports video streaming
   * @return TRUE if it supported, otherwise FALSE
   */
  virtual bool video_streaming_supported() const = 0;

  /**
   * @brief Interface to store whether HMI supports remote control
   * @param supported Indicates whether video streaming is supported by HMI
   */
  virtual void set_rc_supported(const bool supported) = 0;

  /**
   * @brief Retrieves whether HMI supports remote control
   * @return TRUE if it supported, otherwise FALSE
   */
  virtual bool rc_supported() const = 0;

  /*
   * @brief Interface to store whether HMI supports driver distraction menu
   * limits
   *
   * @param supported Indicates whether driver distraction menu limits is
   * supported by HMI
   */
  virtual void set_driver_distraction_supported(const bool supported) = 0;

  /*
   * @brief Retrieves whether HMI supports driver distraction menu limits
   *
   * @return TRUE if it supported, otherwise FALSE
   */
  virtual bool driver_distraction_supported() const = 0;

  /*
   * @brief Interface used to store information regarding
   * the navigation "System Capability"
   * @param navigation_capability contains information related
   * to the navigation system capability.
   */
  virtual void set_navigation_capability(
      const smart_objects::SmartObject& navigation_capability) = 0;

  /**
   * @brief Retrieves information regarding the navigation system capability
   * @return NAVIGATION system capability
   */
  virtual const smart_objects::SmartObjectSPtr navigation_capability()
      const = 0;

  /**
   * @brief Interface used to store information regarding
   * the phone "System Capability"
   * @param phone_capability contains information related
   * to the phone system capability.
   */
  virtual void set_phone_capability(
      const smart_objects::SmartObject& phone_capability) = 0;

  /**
   * @brief Retrieves information regarding the phone call system capability
   * @return PHONE_CALL system capability
   */
  virtual const smart_objects::SmartObjectSPtr phone_capability() const = 0;

  /**
   * @brief Sets HMI's video streaming related capability information
   * @param video_streaming_capability the video streaming related capabilities
   */
  virtual void set_video_streaming_capability(
      const smart_objects::SmartObject& video_streaming_capability) = 0;

  /**
   * @brief Retrieves HMI's video streaming related capabilities
   * @return HMI's video streaming related capability information
   */
  virtual const smart_objects::SmartObjectSPtr video_streaming_capability()
      const = 0;

  /**
   * @brief Sets available RC capabilities for further usage by RC functionality
   * @param rc_capability capabilities to set
   */
  virtual void set_rc_capability(
      const smart_objects::SmartObject& rc_capability) = 0;

  /**
   * @brief Retrieves information regarding the remote control capabilities
   * @return RC capabilities
   */
  virtual const smart_objects::SmartObjectSPtr rc_capability() const = 0;

  virtual void set_driver_distraction_capability(
      const smart_objects::SmartObject& driver_distraction_capability) = 0;

  virtual const smart_objects::SmartObjectSPtr driver_distraction_capability()
      const = 0;

  /**
   * @brief Sets available SeatLocation capabilities for further usage by
   * RC functionality
   * @param seat_location_capability capabilities to set
   */
  virtual void set_seat_location_capability(
      const smart_objects::SmartObject& seat_location_capability) = 0;

  /**
   * @brief seat_location_capability Retrieves information regarding the
   * seat location capability
   * @return smart object of seat location capability
   */
  virtual const smart_objects::SmartObjectSPtr seat_location_capability()
      const = 0;

  virtual void Init(resumption::LastStateWrapperPtr last_state_wrapper) = 0;

  /**
   * @brief Trigger waiting for response
   * @param request Request object
   */
  virtual void set_handle_response_for(
      const smart_objects::SmartObject& request) = 0;

  /**
   * @brief Writes cached HMI capabilities from internal cache into the file if
   * any of updated sections are not present in the file
   * @param interface_name name of interface to be updated
   * @param sections_to_update vector of names of sections which were updated in
   * cache
   * @param schema reference to schema which should be unapplied before saving
   * stringified JSON data into the file
   * @return true if cache was saved successfully, otherwise returns false
   */
  virtual bool SaveCachedCapabilitiesToFile(
      const std::string& interface_name,
      const std::vector<std::string>& sections_to_update,
      const smart_objects::CSmartSchema& schema) = 0;

  /**
   * @brief Deletes cached HMI capabilities file from a file system
   */
  virtual bool DeleteCachedCapabilitiesFile() const = 0;

  /**
   * @brief Checks if request required for appropriate function id
   * @param function_id function ID that required request to get appropriate hmi
   * capabilities
   * @return true if request required for capabilities, otherwise false
   */
  virtual bool IsRequestsRequiredForCapabilities(
      hmi_apis::FunctionID::eType function_id) const = 0;

  /**
   * @brief Update collection of requests that should be send to
   * the HMI to get required HMI capabilities
   * @param requested_interface function id
   */
  virtual void UpdateRequestsRequiredForCapabilities(
      hmi_apis::FunctionID::eType requested_interface) = 0;

  /**
   * @brief Interface that update capabilities depending on ccpu_version
   * @param ccpu_version Received system/hmi software version
   */
  virtual void OnSoftwareVersionReceived(const std::string& ccpu_version) = 0;

  /**
   * @brief Interface that update cached hmi capabilities
   */
  virtual void UpdateCachedCapabilities() = 0;
};

}  //  namespace application_manager

#endif  // SRC_COMPONENTS_INCLUDE_APPLICATION_MANAGER_HMI_CAPABILITIES_H_
