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
#ifndef SRC_COMPONENTS_APPLICATION_MANAGER_TEST_INCLUDE_APPLICATION_MANAGER_MOCK_APPLICATION_H_
#define SRC_COMPONENTS_APPLICATION_MANAGER_TEST_INCLUDE_APPLICATION_MANAGER_MOCK_APPLICATION_H_
#include <string>
#include "application_manager/app_extension.h"
#include "application_manager/application.h"
#include "application_manager/display_capabilities_builder.h"
#include "application_manager/hmi_state.h"
#include "application_manager/usage_statistics.h"
#include "gmock/gmock.h"
#include "smart_objects/smart_object.h"
#include "utils/custom_string.h"
#include "utils/semantic_version.h"

namespace test {
namespace components {
namespace application_manager_test {

namespace custom_str = utils::custom_string;
class MockApplication : public ::application_manager::Application {
 public:
  MockApplication() {}
  MOCK_CONST_METHOD0(active_message, const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(curHash, const std::string&());
  MOCK_METHOD0(UpdateHash, void());
  MOCK_CONST_METHOD0(IsHashChangedDuringSuspend, bool());
  MOCK_METHOD1(SetHashChangedDuringSuspend, void(const bool flag));
  MOCK_CONST_METHOD0(is_application_data_changed, bool());
  MOCK_METHOD1(set_is_application_data_changed,
               void(bool state_application_data));
  MOCK_CONST_METHOD0(is_app_data_resumption_allowed, bool());
  MOCK_METHOD1(set_app_data_resumption_allowance, void(bool allowed));
  MOCK_METHOD0(CloseActiveMessage, void());
  MOCK_CONST_METHOD0(IsFullscreen, bool());
  MOCK_METHOD0(ChangeSupportingAppHMIType, void());
  MOCK_CONST_METHOD0(is_navi, bool());
  MOCK_METHOD1(set_is_navi, void(bool allow));
  MOCK_CONST_METHOD0(mobile_projection_enabled, bool());
  MOCK_METHOD1(set_mobile_projection_enabled, void(bool allow));
  MOCK_CONST_METHOD0(webengine_projection_enabled, bool());
  MOCK_METHOD1(set_webengine_projection_enabled, void(const bool allow));
  MOCK_CONST_METHOD0(video_streaming_approved, bool());
  MOCK_METHOD1(set_video_streaming_approved, void(bool state));
  MOCK_CONST_METHOD0(audio_streaming_approved, bool());
  MOCK_METHOD1(set_audio_streaming_approved, void(bool state));
  MOCK_CONST_METHOD0(video_streaming_allowed, bool());
  MOCK_METHOD1(set_video_streaming_allowed, void(bool state));
  MOCK_CONST_METHOD0(audio_streaming_allowed, bool());
  MOCK_METHOD1(set_audio_streaming_allowed, void(bool state));
  MOCK_CONST_METHOD0(is_audio, bool());
  MOCK_METHOD2(SetVideoConfig,
               bool(protocol_handler::ServiceType service_type,
                    const smart_objects::SmartObject& params));
  MOCK_METHOD1(StartStreaming,
               void(protocol_handler::ServiceType service_type));
  MOCK_METHOD1(StopStreaming, void(protocol_handler::ServiceType service_type));
  MOCK_METHOD1(StopStreamingForce,
               void(protocol_handler::ServiceType service_type));
  MOCK_METHOD1(SuspendStreaming,
               void(protocol_handler::ServiceType service_type));
  MOCK_METHOD2(WakeUpStreaming,
               void(protocol_handler::ServiceType service_type,
                    uint32_t timer_len));
  MOCK_CONST_METHOD0(is_voice_communication_supported, bool());
  MOCK_METHOD1(set_voice_communication_supported,
               void(bool is_voice_communication_supported));
  MOCK_CONST_METHOD0(app_allowed, bool());
  MOCK_CONST_METHOD0(has_been_activated, bool());
  MOCK_METHOD1(set_activated, bool(bool is_active));
  MOCK_CONST_METHOD0(is_ready, bool());
  MOCK_METHOD1(set_is_ready, bool(bool is_active));
  MOCK_CONST_METHOD0(version, const ::application_manager::Version&());
  MOCK_METHOD1(set_hmi_application_id, void(uint32_t hmi_app_id));
  MOCK_CONST_METHOD0(hmi_app_id, uint32_t());
  MOCK_CONST_METHOD0(name, const custom_str::CustomString&());
  MOCK_METHOD1(set_folder_name, void(const std::string& folder_name));
  MOCK_CONST_METHOD0(folder_name, const std::string());
  MOCK_CONST_METHOD0(is_media_application, bool());
  MOCK_CONST_METHOD1(hmi_level,
                     const mobile_apis::HMILevel::eType(
                         const application_manager::WindowID window_id));
  MOCK_CONST_METHOD0(put_file_in_none_count, const uint32_t());
  MOCK_CONST_METHOD0(delete_file_in_none_count, const uint32_t());
  MOCK_CONST_METHOD0(list_files_in_none_count, const uint32_t());
  MOCK_CONST_METHOD1(system_context,
                     const mobile_apis::SystemContext::eType(
                         const application_manager::WindowID window_id));
  MOCK_CONST_METHOD0(audio_streaming_state,
                     const mobile_apis::AudioStreamingState::eType());
  MOCK_CONST_METHOD0(video_streaming_state,
                     const mobile_apis::VideoStreamingState::eType());
  MOCK_CONST_METHOD0(app_icon_path, const std::string&());
  MOCK_CONST_METHOD0(device, connection_handler::DeviceHandle());
  MOCK_CONST_METHOD0(secondary_device, connection_handler::DeviceHandle());
  MOCK_CONST_METHOD1(CurrentHmiState,
                     const application_manager::HmiStatePtr(
                         const application_manager::WindowID window_id));
  MOCK_CONST_METHOD1(RegularHmiState,
                     const application_manager::HmiStatePtr(
                         const application_manager::WindowID window_id));
  MOCK_CONST_METHOD0(GetWindowIds, application_manager::WindowIds());
  MOCK_CONST_METHOD0(GetWindowNames, application_manager::WindowNames());
  MOCK_CONST_METHOD1(WindowIdExists,
                     bool(const application_manager::WindowID window_id));
  MOCK_CONST_METHOD0(IsAllowedToChangeAudioSource, bool());
  MOCK_CONST_METHOD1(PostponedHmiState,
                     const application_manager::HmiStatePtr(
                         const application_manager::WindowID window_id));
  MOCK_METHOD1(set_tts_properties_in_none, void(bool active));
  MOCK_METHOD0(tts_properties_in_none, bool());
  MOCK_METHOD1(set_tts_properties_in_full, void(bool active));
  MOCK_METHOD0(tts_properties_in_full, bool());
  MOCK_METHOD1(set_keep_context, void(bool keep_context));
  MOCK_METHOD0(keep_context, bool());
  MOCK_METHOD1(set_version,
               void(const ::application_manager::Version& version));
  MOCK_METHOD1(set_name, void(const custom_str::CustomString& name));
  MOCK_METHOD1(set_is_media_application, void(bool is_media));
  MOCK_METHOD0(increment_put_file_in_none_count, void());
  MOCK_METHOD0(increment_delete_file_in_none_count, void());
  MOCK_METHOD0(increment_list_files_in_none_count, void());
  MOCK_METHOD1(set_app_icon_path, bool(const std::string& file_name));
  MOCK_METHOD1(set_app_allowed, void(const bool allowed));
  MOCK_METHOD1(set_device, void(connection_handler::DeviceHandle device));
  MOCK_METHOD1(set_secondary_device,
               void(connection_handler::DeviceHandle secondary_device));
  MOCK_CONST_METHOD0(get_grammar_id, uint32_t());
  MOCK_METHOD1(set_grammar_id, void(uint32_t value));
  MOCK_METHOD1(
      set_protocol_version,
      void(const ::protocol_handler::MajorProtocolVersion& protocol_version));
  MOCK_CONST_METHOD0(protocol_version,
                     ::protocol_handler::MajorProtocolVersion());
  MOCK_METHOD1(set_is_resuming, void(bool));
  MOCK_CONST_METHOD0(is_resuming, bool());
  MOCK_METHOD1(set_deferred_resumption_hmi_level,
               void(application_manager::mobile_api::HMILevel::eType level));
  MOCK_CONST_METHOD0(deferred_resumption_hmi_level,
                     application_manager::mobile_api::HMILevel::eType());
  MOCK_METHOD1(AddFile, bool(const ::application_manager::AppFile& file));
  MOCK_CONST_METHOD0(getAppFiles, const ::application_manager::AppFilesMap&());
  MOCK_METHOD1(UpdateFile, bool(const ::application_manager::AppFile& file));
  MOCK_METHOD1(DeleteFile, bool(const std::string& file_name));
  MOCK_METHOD1(
      GetFile,
      const ::application_manager::AppFile*(const std::string& file_name));
  MOCK_METHOD1(SubscribeToButton,
               bool(mobile_apis::ButtonName::eType btn_name));
  MOCK_METHOD1(IsSubscribedToButton,
               bool(mobile_apis::ButtonName::eType btn_name));
  MOCK_METHOD1(UnsubscribeFromButton,
               bool(mobile_apis::ButtonName::eType btn_name));
  MOCK_METHOD0(ResetDataInNone, void());
  MOCK_METHOD2(AreCommandLimitsExceeded,
               bool(mobile_apis::FunctionID::eType cmd_id,
                    ::application_manager::TLimitSource source));
  MOCK_METHOD0(usage_report, ::application_manager::UsageStatistics&());
  MOCK_METHOD0(help_prompt_manager,
               ::application_manager::HelpPromptManager&());
  MOCK_CONST_METHOD0(help_prompt_manager,
                     const ::application_manager::HelpPromptManager&());
  MOCK_METHOD3(SetInitialState,
               void(const application_manager::WindowID window_id,
                    const std::string& window_name,
                    application_manager::HmiStatePtr state));
  MOCK_METHOD2(SetRegularState,
               void(const application_manager::WindowID window_id,
                    application_manager::HmiStatePtr state));
  MOCK_METHOD2(SetPostponedState,
               void(const application_manager::WindowID window_id,
                    ::application_manager::HmiStatePtr state));
  MOCK_METHOD1(RemovePostponedState,
               void(const application_manager::WindowID window_id));
  MOCK_METHOD2(AddHMIState,
               void(const application_manager::WindowID window_id,
                    application_manager::HmiStatePtr state));
  MOCK_METHOD2(RemoveHMIState,
               void(const application_manager::WindowID window_id,
                    ::application_manager::HmiState::StateID state_id));
  MOCK_METHOD2(
      SubscribeToSoftButtons,
      void(int32_t cmd_id,
           const application_manager::WindowSoftButtons& window_softbuttons));
  MOCK_METHOD1(IsSubscribedToSoftButton, bool(const uint32_t softbutton_id));
  MOCK_METHOD1(UnsubscribeFromSoftButtons, void(int32_t cmd_id));
  MOCK_CONST_METHOD0(IsAudioApplication, bool());
  MOCK_CONST_METHOD0(IsVideoApplication, bool());
  MOCK_METHOD0(LoadPersistentFiles, void());
  // InitialApplicationData methods
  MOCK_CONST_METHOD0(app_types, const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(vr_synonyms, const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(policy_app_id, std::string());
  MOCK_CONST_METHOD0(tts_name, const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(ngn_media_screen_name,
                     const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(language, const mobile_apis::Language::eType&());
  MOCK_CONST_METHOD0(ui_language, const mobile_apis::Language::eType&());
  MOCK_CONST_METHOD0(msg_version, const utils::SemanticVersion&());
  MOCK_METHOD1(set_app_types,
               void(const smart_objects::SmartObject& app_types));
  MOCK_METHOD1(set_vr_synonyms,
               void(const smart_objects::SmartObject& vr_synonyms));
  MOCK_METHOD1(set_policy_app_id, void(const std::string& policy_app_id));
  MOCK_METHOD1(set_tts_name, void(const smart_objects::SmartObject& tts_name));
  MOCK_METHOD1(set_ngn_media_screen_name,
               void(const smart_objects::SmartObject& ngn_name));
  MOCK_METHOD1(set_language,
               void(const mobile_apis::Language::eType& language));
  MOCK_METHOD1(set_ui_language,
               void(const mobile_apis::Language::eType& ui_language));
  MOCK_METHOD1(set_msg_version, void(const utils::SemanticVersion& version));
  // DynamicApplicationData methods
  MOCK_CONST_METHOD0(help_prompt, const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(timeout_prompt, const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(vr_help_title, const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(vr_help, const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(tbt_state, const mobile_apis::TBTState::eType&());
  MOCK_CONST_METHOD0(show_command, const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(tbt_show_command, const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(
      SubscribedButtons,
      DataAccessor< ::application_manager::ButtonSubscriptions>());
  MOCK_CONST_METHOD0(keyboard_props, const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(menu_title, const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(menu_icon, const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(menu_layout, const smart_objects::SmartObject*());
  MOCK_CONST_METHOD0(day_color_scheme, smart_objects::SmartObject());
  MOCK_CONST_METHOD0(night_color_scheme, smart_objects::SmartObject());
  MOCK_CONST_METHOD0(display_layout, std::string());
  MOCK_CONST_METHOD0(display_capabilities, smart_objects::SmartObjectSPtr());
  MOCK_CONST_METHOD1(display_capabilities,
                     smart_objects::SmartObjectSPtr(
                         const application_manager::WindowID window_id));

  MOCK_METHOD2(set_window_layout,
               void(const application_manager::WindowID window_id,
                    const std::string& layout));
  MOCK_METHOD2(set_day_color_scheme,
               void(const application_manager::WindowID window_id,
                    const smart_objects::SmartObject& color_scheme));
  MOCK_METHOD2(set_night_color_scheme,
               void(const application_manager::WindowID window_id,
                    const smart_objects::SmartObject& color_scheme));
  MOCK_CONST_METHOD1(
      window_layout,
      std::string(const application_manager::WindowID window_id));
  MOCK_CONST_METHOD1(day_color_scheme,
                     smart_objects::SmartObject(
                         const application_manager::WindowID window_id));
  MOCK_CONST_METHOD1(night_color_scheme,
                     smart_objects::SmartObject(
                         const application_manager::WindowID window_id));
  MOCK_METHOD1(set_display_capabilities,
               void(const smart_objects::SmartObject& display_capabilities));

  MOCK_METHOD1(load_global_properties,
               void(const smart_objects::SmartObject& so));
  MOCK_METHOD1(set_help_prompt,
               void(const smart_objects::SmartObject& help_prompt));
  MOCK_METHOD1(set_timeout_prompt,
               void(const smart_objects::SmartObject& timeout_prompt));
  MOCK_METHOD1(set_vr_help_title,
               void(const smart_objects::SmartObject& vr_help_title));
  MOCK_METHOD0(reset_vr_help_title, void());
  MOCK_METHOD1(set_vr_help, void(const smart_objects::SmartObject& vr_help));
  MOCK_METHOD0(reset_vr_help, void());
  MOCK_METHOD1(set_tbt_state,
               void(const mobile_apis::TBTState::eType& tbt_state));
  MOCK_METHOD1(set_show_command,
               void(const smart_objects::SmartObject& show_command));
  MOCK_METHOD1(set_tbt_show_command,
               void(const smart_objects::SmartObject& tbt_show));
  MOCK_METHOD1(set_keyboard_props,
               void(const smart_objects::SmartObject& keyboard_props));
  MOCK_METHOD1(set_menu_title,
               void(const smart_objects::SmartObject& menu_title));
  MOCK_METHOD1(set_menu_icon,
               void(const smart_objects::SmartObject& menu_icon));
  MOCK_METHOD1(set_menu_layout,
               void(const smart_objects::SmartObject& menu_layout));
  MOCK_METHOD1(set_day_color_scheme,
               void(const smart_objects::SmartObject& color_scheme));
  MOCK_METHOD1(set_night_color_scheme,
               void(const smart_objects::SmartObject& color_scheme));
  MOCK_METHOD1(set_display_layout, void(const std::string& layout));
  MOCK_CONST_METHOD0(audio_stream_retry_number, uint32_t());
  MOCK_METHOD1(set_audio_stream_retry_number,
               void(const uint32_t& audio_stream_retry_number));
  MOCK_CONST_METHOD0(video_stream_retry_number, uint32_t());
  MOCK_METHOD1(set_video_stream_retry_number,
               void(const uint32_t& video_stream_retry_number));
  MOCK_METHOD2(AddCommand,
               void(uint32_t cmd_id,
                    const smart_objects::SmartObject& command));
  MOCK_METHOD1(RemoveCommand, void(uint32_t cmd_id));
  MOCK_METHOD1(FindCommand, smart_objects::SmartObject(uint32_t cmd_id));
  MOCK_METHOD2(AddSubMenu,
               void(uint32_t menu_id, const smart_objects::SmartObject& menu));
  MOCK_METHOD1(RemoveSubMenu, void(uint32_t menu_id));
  MOCK_CONST_METHOD1(FindSubMenu, smart_objects::SmartObject(uint32_t menu_id));
  MOCK_METHOD2(IsSubMenuNameAlreadyExist,
               bool(const std::string& name, const uint32_t parent_id));
  MOCK_METHOD2(AddChoiceSet,
               void(uint32_t choice_set_id,
                    const smart_objects::SmartObject& choice_set));
  MOCK_METHOD1(RemoveChoiceSet, void(uint32_t choice_set_id));
  MOCK_METHOD1(FindChoiceSet,
               smart_objects::SmartObject(uint32_t choice_set_id));
  MOCK_METHOD3(AddPerformInteractionChoiceSet,
               void(uint32_t correlation_id,
                    uint32_t choice_set_id,
                    const smart_objects::SmartObject& choice_set));
  MOCK_METHOD1(DeletePerformInteractionChoiceSet,
               void(uint32_t correlation_id));
  MOCK_CONST_METHOD0(
      performinteraction_choice_set_map,
      DataAccessor< ::application_manager::PerformChoiceSetMap>());
  MOCK_CONST_METHOD0(commands_map,
                     DataAccessor< ::application_manager::CommandsMap>());
  MOCK_CONST_METHOD0(sub_menu_map,
                     DataAccessor< ::application_manager::SubMenuMap>());
  MOCK_CONST_METHOD0(choice_set_map,
                     DataAccessor< ::application_manager::ChoiceSetMap>());
  MOCK_METHOD1(set_perform_interaction_active, void(uint32_t active));
  MOCK_CONST_METHOD0(is_perform_interaction_active, uint32_t());
  MOCK_METHOD1(set_perform_interaction_mode, void(int32_t mode));
  MOCK_CONST_METHOD0(perform_interaction_mode, int32_t());
  MOCK_METHOD1(set_perform_interaction_layout,
               void(mobile_apis::LayoutMode::eType mode));
  MOCK_CONST_METHOD0(perform_interaction_layout,
                     mobile_apis::LayoutMode::eType());
  MOCK_METHOD1(set_reset_global_properties_active, void(bool active));
  MOCK_CONST_METHOD0(is_reset_global_properties_active, bool());
  MOCK_CONST_METHOD0(app_id, uint32_t());
  MOCK_CONST_METHOD0(mac_address, const std::string&());
  MOCK_CONST_METHOD0(bundle_id, const std::string&());
  MOCK_METHOD1(set_bundle_id, void(const std::string& bundle_id));
  MOCK_METHOD0(GetAvailableDiskSpace, uint32_t());

  MOCK_METHOD1(set_mobile_app_id, void(const std::string& policy_app_id));
  MOCK_CONST_METHOD0(is_foreground, bool());
  MOCK_METHOD1(set_foreground, void(bool is_foreground));
  MOCK_CONST_METHOD0(IsRegistered, bool());
  MOCK_CONST_METHOD0(SchemaUrl, std::string());
  MOCK_CONST_METHOD0(PackageName, std::string());
  MOCK_METHOD1(PushMobileMessage, void(smart_objects::SmartObjectSPtr message));
  MOCK_METHOD1(
      SwapMobileMessageQueue,
      void(::application_manager::MobileMessageQueue& mobile_messages));

  MOCK_METHOD2(
      set_system_context,
      void(const application_manager::WindowID window_id,
           const application_manager::mobile_api::SystemContext::eType&));
  MOCK_METHOD1(
      set_audio_streaming_state,
      void(const application_manager::mobile_api::AudioStreamingState::eType&
               state));
  MOCK_METHOD1(IsSubscribedToInteriorVehicleData,
               bool(smart_objects::SmartObject module));
  MOCK_METHOD1(SubscribeToInteriorVehicleData,
               bool(smart_objects::SmartObject module));
  MOCK_METHOD1(UnsubscribeFromInteriorVehicleData,
               bool(smart_objects::SmartObject module));
  MOCK_METHOD2(
      set_hmi_level,
      void(const application_manager::WindowID window_id,
           const application_manager::mobile_api::HMILevel::eType& hmi_level));
  MOCK_METHOD1(QueryInterface,
               application_manager::AppExtensionPtr(
                   application_manager::AppExtensionUID uid));
  MOCK_METHOD1(AddExtension,
               bool(application_manager::AppExtensionPtr extention));
  MOCK_METHOD1(RemoveExtension, bool(application_manager::AppExtensionUID uid));
  MOCK_CONST_METHOD0(Extensions,
                     const std::list<application_manager::AppExtensionPtr>&());
  MOCK_CONST_METHOD0(is_remote_control_supported, bool());
  MOCK_METHOD1(set_remote_control_supported, void(const bool allow));
  MOCK_CONST_METHOD0(cloud_app_endpoint, const std::string&());
  MOCK_CONST_METHOD0(auth_token, const std::string&());
  MOCK_CONST_METHOD0(cloud_app_transport_type, const std::string&());
  MOCK_CONST_METHOD0(hybrid_app_preference,
                     const mobile_apis::HybridAppPreference::eType&());
  MOCK_CONST_METHOD0(cloud_app_certificate, const std::string&());
  MOCK_CONST_METHOD0(is_cloud_app, bool());
  MOCK_METHOD1(set_cloud_app_endpoint, void(const std::string& endpoint));
  MOCK_METHOD1(set_auth_token, void(const std::string& auth_token));
  MOCK_METHOD1(set_cloud_app_transport_type,
               void(const std::string& transport_type));
  MOCK_METHOD1(set_hybrid_app_preference,
               void(const mobile_apis::HybridAppPreference::eType&
                        hybrid_app_preference));
  MOCK_METHOD1(set_cloud_app_certificate, void(const std::string& certificate));
  MOCK_METHOD2(SetWindowInfo,
               void(const ::application_manager::WindowID windowID,
                    const smart_objects::SmartObject& window_info));
  MOCK_METHOD1(RemoveWindowInfo,
               void((const ::application_manager::WindowID window_id)));
  MOCK_CONST_METHOD0(window_optional_params_map,
                     DataAccessor< ::application_manager::WindowParamsMap>());
  MOCK_METHOD0(display_capabilities_builder,
               application_manager::DisplayCapabilitiesBuilder&());
  MOCK_METHOD1(GetSoftButtonWindowID,
               application_manager::WindowID(const uint32_t button_id));
  MOCK_METHOD1(remove_window_capability,
               void(const application_manager::WindowID window_id));
  MOCK_CONST_METHOD1(menu_layout_supported,
                     bool(const mobile_apis::MenuLayout::eType layout));
  MOCK_METHOD1(set_user_location,
               void(const smart_objects::SmartObject& user_location));
  MOCK_CONST_METHOD0(get_user_location, const smart_objects::SmartObject&());
};

}  // namespace application_manager_test
}  // namespace components
}  // namespace test

#endif  // SRC_COMPONENTS_APPLICATION_MANAGER_TEST_INCLUDE_APPLICATION_MANAGER_MOCK_APPLICATION_H_
