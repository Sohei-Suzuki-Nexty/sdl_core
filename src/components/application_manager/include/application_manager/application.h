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

#ifndef SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_APPLICATION_H_
#define SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_APPLICATION_H_

#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "application_manager/app_extension.h"
#include "application_manager/application_state.h"
#include "application_manager/help_prompt_manager.h"
#include "application_manager/hmi_state.h"
#include "application_manager/message.h"
#include "connection_handler/device.h"
#include "interfaces/HMI_API.h"
#include "interfaces/MOBILE_API.h"
#include "protocol_handler/protocol_handler.h"
#include "smart_objects/smart_object.h"
#include "utils/data_accessor.h"
#include "utils/macro.h"
#include "utils/semantic_version.h"

namespace application_manager {

namespace mobile_api = mobile_apis;
namespace custom_str = utils::custom_string;

typedef int32_t ErrorCode;

class UsageStatistics;

class DisplayCapabilitiesBuilder;

enum APIVersion {
  kUnknownAPI = -1,
  kAPIV0 = 0,
  kAPIV1 = 1,
  kAPIV2 = 2,
  kAPIV3 = 3,
  kAPIV4 = 4,
  kAPIV5 = 5,
  kAPIV6 = 6
};

enum TLimitSource { POLICY_TABLE = 0, CONFIG_FILE };

struct Version {
  APIVersion min_supported_api_version;
  APIVersion max_supported_api_version;

  Version()
      : min_supported_api_version(APIVersion::kUnknownAPI)
      , max_supported_api_version(APIVersion::kUnknownAPI) {}
};

struct AppFile {
  // need to use in std::map;
  AppFile()
      : is_persistent(false)
      , is_download_complete(false)
      , file_type(mobile_apis::FileType::INVALID_ENUM) {}
  AppFile(const std::string& name,
          bool persistent,
          bool download_complete,
          mobile_apis::FileType::eType type)
      : file_name(name)
      , is_persistent(persistent)
      , is_download_complete(download_complete)
      , file_type(type) {}
  std::string file_name;
  bool is_persistent;
  bool is_download_complete;
  mobile_apis::FileType::eType file_type;
};
typedef std::map<std::string, AppFile> AppFilesMap;
typedef std::map<int32_t, hmi_apis::Common_ButtonName::eType>
    ButtonSubscriptionsMap;
class InitialApplicationData {
 public:
  virtual ~InitialApplicationData() {}

  virtual const smart_objects::SmartObject* app_types() const = 0;
  virtual const smart_objects::SmartObject* vr_synonyms() const = 0;
  virtual const std::string& mac_address() const = 0;
  virtual const std::string& bundle_id() const = 0;
  virtual void set_bundle_id(const std::string& bundle_id) = 0;
  virtual std::string policy_app_id() const = 0;
  virtual const smart_objects::SmartObject* tts_name() const = 0;
  virtual const smart_objects::SmartObject* ngn_media_screen_name() const = 0;
  virtual const mobile_api::Language::eType& language() const = 0;
  virtual const mobile_api::Language::eType& ui_language() const = 0;
  virtual const utils::SemanticVersion& msg_version() const = 0;
  virtual void set_app_types(const smart_objects::SmartObject& app_types) = 0;
  virtual void set_vr_synonyms(
      const smart_objects::SmartObject& vr_synonyms) = 0;
  virtual void set_mobile_app_id(const std::string& policy_app_id) = 0;
  virtual void set_tts_name(const smart_objects::SmartObject& tts_name) = 0;
  virtual void set_ngn_media_screen_name(
      const smart_objects::SmartObject& ngn_name) = 0;
  virtual void set_language(const mobile_api::Language::eType& language) = 0;
  virtual void set_ui_language(
      const mobile_api::Language::eType& ui_language) = 0;
  virtual void set_msg_version(const utils::SemanticVersion& version) = 0;
};

/*
 * @brief Typedef for supported commands in application menu
 */
typedef std::map<uint32_t, smart_objects::SmartObject*> CommandsMap;

/*
 * @brief Typedef for supported sub menu in application menu
 */
typedef std::map<uint32_t, smart_objects::SmartObject*> SubMenuMap;

/*
 * @brief Typedef for interaction choice set
 */
typedef std::map<uint32_t, smart_objects::SmartObject*> ChoiceSetMap;

/*
 * @brief Typedef for map of window ids to data used as parameters
 * to CreateWindow request.
 */
typedef std::map<WindowID, smart_objects::SmartObjectSPtr> WindowParamsMap;

/*
 * @brief Typedef for perform interaction choice
 * @param choice id
 * @param SmartObject choice
 */
typedef std::map<uint32_t, smart_objects::SmartObject*> PerformChoice;

/*
 * @brief Typedef for perform interaction choice set
 * @param request corellation id
 * @param map of choices
 */
typedef std::map<uint32_t, PerformChoice> PerformChoiceSetMap;

/**
 * @brief Defines id of SoftButtons for specified WindowID
 */
typedef std::pair<WindowID, std::set<uint32_t> > WindowSoftButtons;

/**
 * @brief Defines id of SoftButtons related to a specified WindowID
 */
typedef std::set<WindowSoftButtons> SoftButtonIDs;

/**
 * @brief Defines set of buttons subscription
 */
typedef std::set<mobile_apis::ButtonName::eType> ButtonSubscriptions;

/**
 * @breif Collection for the mobile command smart object.
 */
typedef std::vector<smart_objects::SmartObjectSPtr> MobileMessageQueue;

class DynamicApplicationData {
 public:
  virtual ~DynamicApplicationData() {}
  virtual const smart_objects::SmartObject* help_prompt() const = 0;
  virtual const smart_objects::SmartObject* timeout_prompt() const = 0;
  virtual const smart_objects::SmartObject* vr_help_title() const = 0;
  virtual const smart_objects::SmartObject* vr_help() const = 0;
  virtual const mobile_api::TBTState::eType& tbt_state() const = 0;
  virtual const smart_objects::SmartObject* show_command() const = 0;
  virtual const smart_objects::SmartObject* tbt_show_command() const = 0;
  virtual DataAccessor<ButtonSubscriptions> SubscribedButtons() const = 0;
  virtual const smart_objects::SmartObject* keyboard_props() const = 0;
  virtual const smart_objects::SmartObject* menu_title() const = 0;
  virtual const smart_objects::SmartObject* menu_icon() const = 0;
  virtual const smart_objects::SmartObject* menu_layout() const = 0;
  virtual smart_objects::SmartObject day_color_scheme() const = 0;
  virtual smart_objects::SmartObject night_color_scheme() const = 0;
  virtual std::string display_layout() const = 0;

  /**
   * @brief Specific display capabilities of application
   * @return display capabilities of application or NULL-initialized pointer if
   * not specified
   */
  virtual smart_objects::SmartObjectSPtr display_capabilities() const = 0;

  /**
   * @brief Specific display capabilities of application
   * @param window id - id of an affected widget
   * @return display capabilities of application or NULL-initialized pointer if
   * not specified
   */
  virtual smart_objects::SmartObjectSPtr display_capabilities(
      const WindowID window_id) const = 0;

  virtual void load_global_properties(const smart_objects::SmartObject& so) = 0;
  virtual void set_help_prompt(
      const smart_objects::SmartObject& help_prompt) = 0;
  virtual void set_timeout_prompt(
      const smart_objects::SmartObject& timeout_prompt) = 0;
  virtual void set_vr_help_title(
      const smart_objects::SmartObject& vr_help_title) = 0;
  virtual void reset_vr_help_title() = 0;
  virtual void set_vr_help(const smart_objects::SmartObject& vr_help) = 0;
  virtual void reset_vr_help() = 0;
  virtual void set_tbt_state(const mobile_api::TBTState::eType& tbt_state) = 0;
  virtual void set_show_command(
      const smart_objects::SmartObject& show_command) = 0;
  virtual void set_tbt_show_command(
      const smart_objects::SmartObject& tbt_show) = 0;
  virtual void set_keyboard_props(
      const smart_objects::SmartObject& keyboard_props) = 0;
  virtual void set_menu_title(const smart_objects::SmartObject& menu_title) = 0;
  virtual void set_menu_icon(const smart_objects::SmartObject& menu_icon) = 0;
  virtual void set_menu_layout(
      const smart_objects::SmartObject& menu_layout) = 0;

  virtual uint32_t audio_stream_retry_number() const = 0;

  virtual void set_audio_stream_retry_number(
      const uint32_t& audio_stream_retry_number) = 0;

  virtual uint32_t video_stream_retry_number() const = 0;

  virtual void set_video_stream_retry_number(
      const uint32_t& video_stream_retry_number) = 0;

  virtual void set_display_layout(const std::string& layout) = 0;
  virtual void set_day_color_scheme(
      const smart_objects::SmartObject& color_scheme) = 0;
  virtual void set_night_color_scheme(
      const smart_objects::SmartObject& color_scheme) = 0;

  /**
   * @brief Set specific application display capabilities
   * @param display_capabilities new display capabilities of application
   */
  virtual void set_display_capabilities(
      const smart_objects::SmartObject& display_capabilities) = 0;

  /**
   * @brief deletes stored window capability for given window id
   * @param window id of capability to remove
   */
  virtual void remove_window_capability(const WindowID window_id) = 0;

  /**
   * @brief checks whether a specific menu layout is supported
   * @param menu layout to check
   */
  virtual bool menu_layout_supported(
      const mobile_apis::MenuLayout::eType layout) const = 0;

  /**
   * @brief Sets layout for application's specific window
   * @param window_id window id for which layout should be set
   * @param layout - layout to be set
   */
  virtual void set_window_layout(const WindowID window_id,
                                 const std::string& layout) = 0;

  /**
   * @brief Sets day color scheme for application's specific window
   * @param window_id window id for which day color scheme should be set
   * @param color_scheme - color scheme to be set
   */
  virtual void set_day_color_scheme(
      const WindowID window_id,
      const smart_objects::SmartObject& color_scheme) = 0;

  /**
   * @brief Sets night color scheme for application's specific window
   * @param window_id window id for which night color scheme should be set
   * @param color_scheme - color scheme to be set
   */
  virtual void set_night_color_scheme(
      const WindowID window_id,
      const smart_objects::SmartObject& color_scheme) = 0;

  /**
   * @brief Gets window layout for application's specific window_id
   * @param window_id window id for which display layout should be returned
   * @return string containing set display layout,
   * if no display layout is set - empty string is returned
   */
  virtual std::string window_layout(const WindowID window_id) const = 0;

  /**
   * @brief Gets day color scheme for application's specific window_id
   * @param window_id window id for which color scheme should be returned
   * @return day color scheme which is set for specified app window,
   * if no day color scheme is set - Smart object with NullType is returned
   */
  virtual smart_objects::SmartObject day_color_scheme(
      const WindowID window_id) const = 0;

  /**
   * @brief Gets night color scheme for application's specific window_id
   * @param window_id window id for which color scheme should be returned
   * @return night color scheme which is set for specified app window
   * if no night color scheme is set - Smart object with NullType is returned
   */
  virtual smart_objects::SmartObject night_color_scheme(
      const WindowID window_id) const = 0;

  /**
   * @brief Checks if application is media, voice communication or navigation
   * @return true if application is media, voice communication or navigation,
   * false otherwise
   */
  virtual bool is_audio() const = 0;

  /*
   * @brief Adds a command to the in application menu
   */
  virtual void AddCommand(uint32_t cmd_id,
                          const smart_objects::SmartObject& command) = 0;

  /*
   * @brief Deletes all commands from the application
   * menu with the specified command id
   */
  virtual void RemoveCommand(uint32_t cmd_id) = 0;

  /*
   * @brief Finds command with the specified command id
   */
  virtual smart_objects::SmartObject FindCommand(uint32_t cmd_id) = 0;

  /*
   * @brief Adds a menu to the application
   */
  virtual void AddSubMenu(uint32_t menu_id,
                          const smart_objects::SmartObject& menu) = 0;

  /*
   * @brief Deletes menu from the application menu
   */
  virtual void RemoveSubMenu(uint32_t menu_id) = 0;

  /*
   * @brief Finds menu with the specified id
   */
  virtual smart_objects::SmartObject FindSubMenu(uint32_t menu_id) const = 0;

  /*
   * @brief Adds a interaction choice set to the application
   *
   * @param choice_set_id Unique ID used for this interaction choice set
   * @param choice_set SmartObject that represent choice set
   */
  virtual void AddChoiceSet(uint32_t choice_set_id,
                            const smart_objects::SmartObject& choice_set) = 0;

  /*
   * @brief Deletes choice set from the application
   *
   * @param choice_set_id Unique ID of the interaction choice set
   */
  virtual void RemoveChoiceSet(uint32_t choice_set_id) = 0;

  /*
   * @brief Adds window info
   * @param window_id unique id of a window
   * @param window_info SmartObject that represent window info
   */
  virtual void SetWindowInfo(const WindowID window_id,
                             const smart_objects::SmartObject& window_info) = 0;

  /*
   * @brief Removes window info
   * @param window_id unique id of a window
   */
  virtual void RemoveWindowInfo(const WindowID window_id) = 0;

  /*
   * @brief Finds choice set with the specified choice_set_id id
   *
   * @param choice_set_id Unique ID of the interaction choice set
   */
  virtual smart_objects::SmartObject FindChoiceSet(uint32_t choice_set_id) = 0;

  /*
   * @brief Adds perform interaction choice set to the application
   *
   * @param correlation_id Unique ID of the request that added this choice set
   * @param choice_set_id  Unique ID used for this interaction choice set
   * @param choice_set SmartObject that represents choice set
   */
  virtual void AddPerformInteractionChoiceSet(
      uint32_t correlation_id,
      uint32_t choice_set_id,
      const smart_objects::SmartObject& choice_set) = 0;

  /*
   * @brief Deletes entirely perform interaction choice set for request
   * @param correlation_id Unique ID of the request that added this choice set
   *
   */
  virtual void DeletePerformInteractionChoiceSet(uint32_t correlation_id) = 0;

  /*
   * @brief Retrieves entirely ChoiceSet - VR commands map
   *
   * @return ChoiceSet map that is currently in use
   */
  virtual DataAccessor<PerformChoiceSetMap> performinteraction_choice_set_map()
      const = 0;

  /*
   * @brief Retrieves window info map
   */
  virtual DataAccessor<WindowParamsMap> window_optional_params_map() const = 0;

  /*
   * @brief Retrieves display capabilities builder
   */
  virtual DisplayCapabilitiesBuilder& display_capabilities_builder() = 0;

  /*
   * @brief Retrieve application commands
   */
  virtual DataAccessor<CommandsMap> commands_map() const = 0;

  /*
   * @brief Retrieve application sub menus
   */
  virtual DataAccessor<SubMenuMap> sub_menu_map() const = 0;

  /*
   * @brief Retrieve application choice set map
   */
  virtual DataAccessor<ChoiceSetMap> choice_set_map() const = 0;

  /*
   * @brief Sets perform interaction state
   *
   * @param active Current state of the perform interaction
   */
  virtual void set_perform_interaction_active(uint32_t active) = 0;

  /*
   * @brief Retrieves perform interaction state
   *
   * @return TRUE if perform interaction active, otherwise FALSE
   */
  virtual uint32_t is_perform_interaction_active() const = 0;

  /*
   * @brief Set perform interaction layout
   *
   * @param Current Interaction layout of the perform interaction
   */
  virtual void set_perform_interaction_layout(
      mobile_api::LayoutMode::eType layout) = 0;

  /*
   * @brief Retrieve perform interaction layout
   */
  virtual mobile_api::LayoutMode::eType perform_interaction_layout() const = 0;

  /*
   * @brief Sets the mode for perform interaction: UI/VR/BOTH
   *
   * @param mode Mode that was selected (MENU; VR; BOTH)
   */
  virtual void set_perform_interaction_mode(int32_t mode) = 0;

  /*
   * @brief Retrieve the mode that was PerformInteraction sent in
   *
   * @return mode of PerformInteraction
   */
  virtual int32_t perform_interaction_mode() const = 0;

  /*
   * @brief Sets reset global properties state
   *
   * @param active Current state of the reset global properties
   */
  virtual void set_reset_global_properties_active(bool active) = 0;

  /*
   * @brief Retrieves reset global properties state
   *
   * @return TRUE if perform interaction active, otherwise FALSE
   */
  virtual bool is_reset_global_properties_active() const = 0;
};

class Application : public virtual InitialApplicationData,
                    public virtual DynamicApplicationData {
 public:
  /**
   * @brief The StreamingState enum defines current streaming state
   */
  enum class StreamingState { kStopped, kStarted, kSuspended };

  enum ApplicationRegisterState { kRegistered = 0, kWaitingForRegistration };

  Application() : is_greyed_out_(false) {}
  virtual ~Application() {}

  /**
   * @brief Returns message belonging to the application
   * that is currently executed (i.e. on HMI).
   * @return smart_objects::SmartObject * Active message
   */
  virtual const smart_objects::SmartObject* active_message() const = 0;

  /**
   * @brief returns current hash value
   * @return current hash value
   */
  virtual const std::string& curHash() const = 0;

  /**
   * @brief Change Hash for current application
   * and send notification to mobile
   * @return updated_hash
   */
  virtual void UpdateHash() = 0;

  /**
   * @brief checks is hashID was changed during suspended state
   * @return Returns TRUE if hashID was changed during suspended state
   * otherwise returns FALSE.
   */
  virtual bool IsHashChangedDuringSuspend() const = 0;

  /**
   * @brief changes state of the flag which tracks is hashID was changed during
   * suspended state or not
   * @param state new state of the flag
   */
  virtual void SetHashChangedDuringSuspend(const bool state) = 0;

  /**
   * @brief method is called when SDL is saving application data for resumption
   * @return TRUE if data of application need to save for resumption, otherwise
   * return FALSE
   */
  virtual bool is_application_data_changed() const = 0;

  /**
   * @brief method is called after SDL saved application data for resumption
   * @param state_application_data contains FALSE after saving data
   */
  virtual void set_is_application_data_changed(bool state_application_data) = 0;

  /**
   * @brief Checks if application data is allowed to be resumed
   * @return TRUE if data of application is allowed to be resumed, otherwise
   * return FALSE
   */
  virtual bool is_app_data_resumption_allowed() const = 0;

  /**
   * @brief Sets resumption allowance for application data
   * @param allowed - true if app data allowed to be resumed,
   * false value means that app data is disallowed for resumption
   */
  virtual void set_app_data_resumption_allowance(const bool allowed) = 0;

  virtual void CloseActiveMessage() = 0;
  virtual bool IsFullscreen() const = 0;
  virtual void ChangeSupportingAppHMIType() = 0;

  virtual bool is_navi() const = 0;
  virtual void set_is_navi(bool allow) = 0;

  /**
   * @brief Returns is_remote_control_supported_
   * @return true if app supports remote control, else false
   */
  virtual bool is_remote_control_supported() const = 0;

  /**
   * @brief Sets remote control supported,
   * which is used to determine app with remote control
   * @param allow, if true - remote control is supported,
   * else remote control is disable
   */
  virtual void set_remote_control_supported(const bool allow) = 0;

  virtual void set_mobile_projection_enabled(bool option) = 0;
  virtual bool mobile_projection_enabled() const = 0;

  virtual void set_webengine_projection_enabled(const bool option) = 0;
  virtual bool webengine_projection_enabled() const = 0;

  virtual bool video_streaming_approved() const = 0;
  virtual void set_video_streaming_approved(bool state) = 0;
  virtual bool audio_streaming_approved() const = 0;
  virtual void set_audio_streaming_approved(bool state) = 0;

  virtual bool video_streaming_allowed() const = 0;
  virtual void set_video_streaming_allowed(bool state) = 0;
  virtual bool audio_streaming_allowed() const = 0;
  virtual void set_audio_streaming_allowed(bool state) = 0;

  /**
   * @brief Sends SetVideoConfig request to HMI to configure streaming
   * @param service_type Type of streaming service, should be kMobileNav
   * @param params parameters of video streaming in key-value format
   * @return true if SetVideoConfig is sent, false otherwise
   */
  virtual bool SetVideoConfig(protocol_handler::ServiceType service_type,
                              const smart_objects::SmartObject& params) = 0;

  /**
   * @brief Starts streaming service for application
   * @param service_type Type of streaming service
   */
  virtual void StartStreaming(protocol_handler::ServiceType service_type) = 0;

  /**
   * @brief Stops streaming service for application
   * @param service_type Type of streaming service
   */
  virtual void StopStreaming(protocol_handler::ServiceType service_type) = 0;

  /**
   * @brief Stops streaming for application whether it is allowed or not HMI
   * @param service_type video or audio
   */
  virtual void StopStreamingForce(
      protocol_handler::ServiceType service_type) = 0;

  /**
   * @brief Suspends streaming process for application
   * @param service_type Type of streaming service
   */
  virtual void SuspendStreaming(protocol_handler::ServiceType service_type) = 0;

  /**
   * @brief Wakes up streaming process for application
   * @param service_type Type of streaming service
   * @param timer_len The amount of time in ms the timer will wait
   */
  virtual void WakeUpStreaming(protocol_handler::ServiceType service_type,
                               uint32_t timer_len = 0) = 0;

  virtual bool is_voice_communication_supported() const = 0;
  virtual void set_voice_communication_supported(
      bool is_voice_communication_supported) = 0;
  virtual bool app_allowed() const = 0;
  virtual bool has_been_activated() const = 0;
  virtual bool set_activated(bool is_active) = 0;
  virtual bool is_ready() const = 0;
  virtual bool set_is_ready(bool is_ready) = 0;

  virtual const Version& version() const = 0;
  virtual void set_hmi_application_id(uint32_t hmi_app_id) = 0;
  virtual uint32_t hmi_app_id() const = 0;
  virtual uint32_t app_id() const = 0;
  virtual const custom_str::CustomString& name() const = 0;
  /**
   * @brief Sets application folder name, which is used for storing of related
   * files, e.g. icons
   * @param folder_name Name of folder
   */
  virtual void set_folder_name(const std::string& folder_name) = 0;
  virtual const std::string folder_name() const = 0;
  virtual bool is_media_application() const = 0;
  virtual bool is_foreground() const = 0;
  virtual void set_foreground(const bool is_foreground) = 0;

  /**
   * @brief hmi_level current HMI level of application's window
   * @param window_id id of application's window to get
   * @return HMI level of application's window
   */
  virtual const mobile_api::HMILevel::eType hmi_level(
      const WindowID window_id) const = 0;

  virtual const uint32_t put_file_in_none_count() const = 0;
  virtual const uint32_t delete_file_in_none_count() const = 0;
  virtual const uint32_t list_files_in_none_count() const = 0;

  /**
   * @brief system_context current system context of application's window
   * @param window_id id of application's window to get
   * @return system context of application's window
   */
  virtual const mobile_api::SystemContext::eType system_context(
      const WindowID window_id) const = 0;
  virtual const mobile_api::AudioStreamingState::eType audio_streaming_state()
      const = 0;
  virtual const mobile_api::VideoStreamingState::eType video_streaming_state()
      const = 0;
  virtual const std::string& app_icon_path() const = 0;
  virtual connection_handler::DeviceHandle device() const = 0;
  /**
   * @brief Returns handle of the device on which secondary transport of this
   * app is running
   * @return handle of the device on which secondary transport is running
   */
  virtual connection_handler::DeviceHandle secondary_device() const = 0;

  /**
   * @brief sets true if application has sent TTS GlobalProperties
   * request with empty array help_prompt to HMI with level
   * NONE BACKGROUND
   * @param active contains state of sending TTS GlobalProperties
   */
  virtual void set_tts_properties_in_none(bool active) = 0;
  /**
   * @brief returns true if application has sent TTS GlobalProperties
   * otherwise return false
   * @return flag tts_properties_in_none
   */
  virtual bool tts_properties_in_none() = 0;
  /**
   * @brief sets true if application has sent TTS GlobalProperties
   * request with default array help_prompt to HMI with level
   * FULL LIMITED
   * @param active contains state of sending TTS GlobalProperties
   */
  virtual void set_tts_properties_in_full(bool active) = 0;
  /**
   * @brief  returns true if application has sent TTS GlobalProperties
   * otherwise return false
   * @return flag tts_properties_in_full
   */
  virtual bool tts_properties_in_full() = 0;
  /**
   * @brief sets true if application should keep it's HMI Level after an audio
   * source change
   * @param value of keep context
   */
  virtual void set_keep_context(bool keep_context) = 0;
  /**
   * @brief  returns true if application should keep keep it's HMI Level after
   * an audio source change, otherwise return false
   * @return value of keep_context flag
   */
  virtual bool keep_context() = 0;
  virtual void set_version(const Version& version) = 0;
  virtual void set_name(const custom_str::CustomString& name) = 0;
  virtual void set_is_media_application(bool is_media) = 0;
  virtual void increment_put_file_in_none_count() = 0;
  virtual void increment_delete_file_in_none_count() = 0;
  virtual void increment_list_files_in_none_count() = 0;
  virtual bool set_app_icon_path(const std::string& file_name) = 0;
  virtual void set_app_allowed(const bool allowed) = 0;
  /**
   * @brief Sets the handle of the device on which secondary transport of this
   * app is running
   * @param handle of the device on which secondary transport is running
   */
  virtual void set_secondary_device(
      connection_handler::DeviceHandle secondary_device) = 0;
  virtual uint32_t get_grammar_id() const = 0;
  virtual void set_grammar_id(uint32_t value) = 0;

  virtual void set_protocol_version(
      const protocol_handler::MajorProtocolVersion& protocol_version) = 0;
  virtual protocol_handler::MajorProtocolVersion protocol_version() const = 0;

  virtual void set_is_resuming(bool is_resuming) = 0;
  virtual bool is_resuming() const = 0;

  /**
   * @brief Remembers the HMI level which the app would resume into if high-
   * bandwidth transport were available.
   * @param level The HMI level which the app would resume into. Specify
   * INVALID_ENUM to clear the state.
   */
  virtual void set_deferred_resumption_hmi_level(
      mobile_api::HMILevel::eType level) = 0;
  /**
   * @brief Returns the HMI level which the app would resume into if high-
   * bandwidth transport were available.
   *
   * A value of INVALID_ENUM indicates that the app does not have deferred
   * HMI level.
   * @return HMI level which the app would resume into
   */
  virtual mobile_api::HMILevel::eType deferred_resumption_hmi_level() const = 0;

  virtual bool AddFile(const AppFile& file) = 0;
  virtual const AppFilesMap& getAppFiles() const = 0;

  /**
   * @brief Updates fields of existing file
   * @param file_name File name, that need to update
   * @param is_persistent Bollean describes is file persistent?
   * @param is_download_complete Bollean describes is file downloaded fully on
   * need to finish downloading?
   * @return TRUE if file exist and updated sucsesfuly, othervise return false
   */
  virtual bool UpdateFile(const AppFile& file) = 0;
  virtual bool DeleteFile(const std::string& file_name) = 0;
  virtual const AppFile* GetFile(const std::string& file_name) = 0;

  virtual bool SubscribeToButton(mobile_apis::ButtonName::eType btn_name) = 0;
  virtual bool IsSubscribedToButton(
      mobile_apis::ButtonName::eType btn_name) = 0;
  virtual bool UnsubscribeFromButton(
      mobile_apis::ButtonName::eType btn_name) = 0;

  /**
   * @brief ResetDataInNone reset data counters in NONE
   */
  virtual void ResetDataInNone() = 0;

  /**
   * @brief Check, if limits for command number per time is exceeded
   * @param cmd_id Unique command id from mobile API
   * @param source Limits source, e.g. policy table, config file etc.
   * @return true, if - excedeed, otherwise - false
   */
  virtual bool AreCommandLimitsExceeded(mobile_apis::FunctionID::eType cmd_id,
                                        TLimitSource source) = 0;

  /**
   * Returns object for recording statistics
   * @return object for recording statistics
   */
  virtual UsageStatistics& usage_report() = 0;

  /**
   * @brief Access to HelpPromptManager interface
   * @return object for Handling VR help
   */
  virtual HelpPromptManager& help_prompt_manager() = 0;

  /**
   * @brief SetInitialState sets initial HMI state for application on
   * registration
   * @param window_id window id for HMI state
   * @param window_name name of inited window
   * @param state Hmi state value
   */
  virtual void SetInitialState(const WindowID window_id,
                               const std::string& window_name,
                               HmiStatePtr state) = 0;

  /**
   * @brief SetRegularState set permanent state of application
   * @param window_id window id for HMI state
   * @param state state to setup
   */
  virtual void SetRegularState(const WindowID window_id, HmiStatePtr state) = 0;

  /**
   * @brief SetPostponedState sets postponed state to application.
   * This state could be set as regular later
   * @param window_id window id for HMI state
   * @param state state to setup
   */
  virtual void SetPostponedState(const WindowID window_id,
                                 HmiStatePtr state) = 0;

  /**
   * @brief RemovePostponedState removes postponed state for application
   * After removal, this state will not be set as regular later
   * @param window_id window id for HMI state
   */
  virtual void RemovePostponedState(const WindowID window_id) = 0;

  /**
   * @brief AddHMIState the function that will change application's
   * hmi state.
   *
   * @param window_id window id for HMI state
   *
   * @param state new hmi state for certain application.
   */
  virtual void AddHMIState(const WindowID window_id, HmiStatePtr state) = 0;

  /**
   * @brief RemoveHMIState the function that will turn back hmi_level after end
   * of some event
   *
   * @param window_id window id for HMI state
   *
   * @param state_id that should be removed
   */
  virtual void RemoveHMIState(const WindowID window_id,
                              HmiState::StateID state_id) = 0;

  /**
   * @brief HmiState of application within active events PhoneCall, TTS< etc ...
   * @param window_id window id for HMI state
   * @return Active HmiState of application
   */
  virtual const HmiStatePtr CurrentHmiState(const WindowID window_id) const = 0;

  /**
   * @brief Getter for a list of available application windows including the
   * main
   * @return list of available window IDs created by application
   */
  virtual WindowIds GetWindowIds() const = 0;

  /**
   * @brief Getter for a list of all existing window names
   * @return list of available window names created by application
   */
  virtual WindowNames GetWindowNames() const = 0;

  /**
   * @brief Check if application has specified window
   * @param window_id - id of window to check
   * @return true if app has specified window , otherwise false
   */
  virtual bool WindowIdExists(const WindowID window_id) const = 0;

  /**
   * @brief RegularHmiState of application without active events VR, TTS etc ...
   * @param window_id window id for HMI state
   * @return HmiState of application
   */
  virtual const HmiStatePtr RegularHmiState(const WindowID window_id) const = 0;

  /**
   * @brief Checks if app is allowed to change audio source
   * @return True - if allowed, otherwise - False
   */
  virtual bool IsAllowedToChangeAudioSource() const = 0;

  /**
   * @brief PostponedHmiState returns postponed hmi state of application
   * if it's present
   * @param window_id window id for HMI state
   * @return Postponed hmi state of application
   */
  virtual const HmiStatePtr PostponedHmiState(
      const WindowID window_id) const = 0;

  /**
   * @brief Keeps id of softbuttons which is created in commands:
   * Alert, Show, ScrollableMessage, ShowConstantTBT, AlertManeuver,
   * UpdateTurnList
   * @param cmd_id Unique command id from mobile API
   * @param window_softbuttons list of softbuttons were created by command.
   */
  virtual void SubscribeToSoftButtons(
      int32_t cmd_id, const WindowSoftButtons& window_softbuttons) = 0;

  /**
   * @brief Retreives window id on which given button is created
   * @param button_id identifier of a button
   * @param window id of a widget containing button
   */
  virtual WindowID GetSoftButtonWindowID(const uint32_t button_id) = 0;

  /**
   * @brief Determine the existence of softbutton
   * @param Softbutton_id contains id of softbutton
   * @return Returns true if application contains softbutton id otherwise
   * returns false.
   */
  virtual bool IsSubscribedToSoftButton(const uint32_t softbutton_id) = 0;

  /**
   * @brief Removes list of softbuttons which is created in commands
   * @param cmd_id Unique command id from mobile API
   */
  virtual void UnsubscribeFromSoftButtons(int32_t cmd_id) = 0;

  /**
   * @brief Check's if it is media, voice communication or navigation
   * application
   *
   * @return true if application is media, voice communication or navigation
   */
  virtual bool IsAudioApplication() const = 0;

  /**
   * @brief Check's if it is projection or navigation application
   *
   * @return true if application is projection or navigation
   */
  virtual bool IsVideoApplication() const = 0;

  /**
   * @brief IsRegistered allows to distinguish if this
   * application has been registered.
   *
   * @return true if registered, false otherwise.
   */
  virtual bool IsRegistered() const = 0;
  /**
   * @brief MarkRegistered allows to mark application as registered.
   */
  void MarkRegistered() {
    app_state_ = kRegistered;
  }

  /**
   * @brief MarkUnregistered allows to mark application as unregistered.
   */
  void MarkUnregistered() {
    app_state_ = kWaitingForRegistration;
  }

  /**
   * @brief schemaUrl contains application's url (for 4th protocol version)
   *
   * @return application's url.
   */
  std::string SchemaUrl() const {
    return url_;
  }

  /**
   * @brief SetShemaUrl allows to store schema url for application.
   *
   * @param url url to store.
   */
  void SetShemaUrl(const std::string& url) {
    url_ = url;
  }

  /**
   * @brief packagName allows to obtain application's package name.
   *
   * @return pakage name.
   */
  std::string PackageName() const {
    return package_name_;
  }

  /**
   * @brief SetPackageName allows to store package name for application.
   *
   * @param packageName package name to store.
   */
  void SetPackageName(const std::string& packageName) {
    package_name_ = packageName;
  }

  /**
   * @brief Returns is application should be greyed out on HMI
   */
  bool is_greyed_out() const {
    return is_greyed_out_;
  }

  /**
   * @brief Sets application as should be greyed out on HMI
   * @param is_greyed_out True, if should be greyed out on HMI,
   * otherwise - false
   */
  void set_greyed_out(bool is_greyed_out) {
    is_greyed_out_ = is_greyed_out;
  }
  /**
   * @brief Load persistent files from application folder.
   */
  virtual void LoadPersistentFiles() = 0;

  /**
   * @brief Get available app space
   * @param name of the app folder(make + mobile app id)
   * @return free app space.
   */
  virtual uint32_t GetAvailableDiskSpace() = 0;

  /**
   * @brief Allows to save mobile's command smart object in order to perform
   * this command later.
   * @param mobile_message the message smart_object.
   */
  virtual void PushMobileMessage(
      smart_objects::SmartObjectSPtr mobile_message) = 0;

  /**
   * @brief Allows to obtain the whole list of pending commands in order to
   * process them.
   * @param mobile_message the messages array which is filled by the method.
   */
  virtual void SwapMobileMessageQueue(MobileMessageQueue& mobile_messages) = 0;

  /**
   * @brief set_system_context Set system context for application
   * @param window_id window id for HMI state
   * @param system_context Current context
   */
  virtual void set_system_context(
      const WindowID window_id,
      const mobile_api::SystemContext::eType& system_context) = 0;

  /**
   * @brief set_audio_streaming_state Set audio streaming state for application
   * @param state Current audio streaming state
   */
  virtual void set_audio_streaming_state(
      const mobile_api::AudioStreamingState::eType& state) = 0;

  /**
   * @brief set_hmi_level Set HMI level for application
   * @param window_id window id for HMI state
   * @param hmi_level Current HMI level
   */
  virtual void set_hmi_level(const WindowID window_id,
                             const mobile_api::HMILevel::eType& hmi_level) = 0;

  /**
   * @brief Return pointer to extension by uid
   * @param uid uid of extension
   * @return Pointer to extension, if extension was initialized, otherwise NULL
   */
  virtual AppExtensionPtr QueryInterface(AppExtensionUID uid) = 0;

  /**
   * @brief Add extension to application
   * @param extension pointer to extension
   * @return true if success, false if extension already initialized
   */
  virtual bool AddExtension(AppExtensionPtr extention) = 0;

  /**
   * @brief Remove extension from application
   * @param uid uid of extension
   * @return true if success, false if extension is not present
   */
  virtual bool RemoveExtension(AppExtensionUID uid) = 0;

  /**
   * @brief Get list of available application extensions
   * @return application extensions
   */
  virtual const std::list<AppExtensionPtr>& Extensions() const = 0;

  /**
   * @brief Get cloud app endpoint for websocket connection
   * @return cloud app endpoint
   */
  virtual const std::string& cloud_app_endpoint() const = 0;

  /**
   * @brief Get cloud app auth token to be used in connection handshake after
   * websocket open.
   * @return cloud app auth token
   */
  virtual const std::string& auth_token() const = 0;

  /**
   * @brief Get cloud app transport type. Defines the type of websocket
   * connection used.
   * @return cloud app transport type
   */
  virtual const std::string& cloud_app_transport_type() const = 0;

  /**
   * @brief Get hybrid app preference. Defines behaviour for when a similar
   * mobile and cloud app are connected simultaneously.
   * @return hybrid app preference
   */
  virtual const mobile_apis::HybridAppPreference::eType& hybrid_app_preference()
      const = 0;

  /**
   * @brief Get cloud app certificate. Used for secured websocket connections.
   * @return cloud app certificate.
   */
  virtual const std::string& cloud_app_certificate() const = 0;

  /**
   * @brief Check whether the given application is a cloud app.
   * @return true if the application is a cloud application, false otherwise.
   */
  virtual bool is_cloud_app() const = 0;

  /**
   * @brief Set cloud app endpoint
   */
  virtual void set_cloud_app_endpoint(const std::string& endpoint) = 0;

  /**
   * @brief Set cloud app auth token
   */
  virtual void set_auth_token(const std::string& auth_token) = 0;

  /**
   * @brief Set cloud app transport type
   */
  virtual void set_cloud_app_transport_type(
      const std::string& transport_type) = 0;

  /**
   * @brief Set hybrid app preference
   */
  virtual void set_hybrid_app_preference(
      const mobile_apis::HybridAppPreference::eType& hybrid_app_preference) = 0;

  /**
   * @brief Set cloud app certificate
   */
  virtual void set_cloud_app_certificate(const std::string& certificate) = 0;

  /**
   * @brief Set user location
   * @param smart object of user location
   */
  virtual void set_user_location(
      const smart_objects::SmartObject& user_location) = 0;

  /**
   * @brief Get user location
   * @return smart object of user location
   */
  virtual const smart_objects::SmartObject& get_user_location() const = 0;

 protected:
  mutable sync_primitives::Lock hmi_states_lock_;

  ApplicationRegisterState app_state_;
  ApplicationState state_;
  std::string url_;
  std::string package_name_;
  std::string device_id_;
  ssize_t connection_id_;
  bool is_greyed_out_;
};

typedef std::shared_ptr<Application> ApplicationSharedPtr;
typedef std::shared_ptr<const Application> ApplicationConstSharedPtr;
typedef uint32_t ApplicationId;

}  // namespace application_manager

#endif  // SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_APPLICATION_H_
