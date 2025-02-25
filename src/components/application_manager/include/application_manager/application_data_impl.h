/*
 * Copyright (c) 2013, Ford Motor Company
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

#ifndef SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_APPLICATION_DATA_IMPL_H_
#define SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_APPLICATION_DATA_IMPL_H_

#include <string>
#include "application_manager/application.h"
#include "application_manager/display_capabilities_builder.h"
#include "interfaces/MOBILE_API.h"
#include "smart_objects/smart_object.h"
#include "utils/lock.h"
#include "utils/semantic_version.h"

namespace application_manager {

namespace mobile_api = mobile_apis;

class InitialApplicationDataImpl : public virtual Application {
 public:
  InitialApplicationDataImpl();
  ~InitialApplicationDataImpl();

  const smart_objects::SmartObject* app_types() const;
  const smart_objects::SmartObject* vr_synonyms() const;
  virtual std::string policy_app_id() const;
  const smart_objects::SmartObject* tts_name() const;
  const smart_objects::SmartObject* ngn_media_screen_name() const;
  const mobile_api::Language::eType& language() const;
  const mobile_api::Language::eType& ui_language() const;
  const utils::SemanticVersion& msg_version() const;

  void set_app_types(const smart_objects::SmartObject& app_types);
  void set_vr_synonyms(const smart_objects::SmartObject& vr_synonyms);
  void set_mobile_app_id(const std::string& policy_app_id);
  void set_tts_name(const smart_objects::SmartObject& tts_name);
  void set_ngn_media_screen_name(const smart_objects::SmartObject& ngn_name);
  void set_language(const mobile_api::Language::eType& language);
  void set_ui_language(const mobile_api::Language::eType& ui_language);
  void set_msg_version(const utils::SemanticVersion& version);

  void set_perform_interaction_layout(
      mobile_api::LayoutMode::eType layout) OVERRIDE;
  mobile_api::LayoutMode::eType perform_interaction_layout() const OVERRIDE;

 protected:
  smart_objects::SmartObject* app_types_;
  smart_objects::SmartObject* vr_synonyms_;
  std::string mobile_app_id_;
  smart_objects::SmartObject* tts_name_;
  smart_objects::SmartObject* ngn_media_screen_name_;
  mobile_api::Language::eType language_;
  mobile_api::Language::eType ui_language_;
  mobile_apis::LayoutMode::eType perform_interaction_layout_;
  utils::SemanticVersion msg_version_;

 private:
  DISALLOW_COPY_AND_ASSIGN(InitialApplicationDataImpl);
};

class DynamicApplicationDataImpl : public virtual Application {
 public:
  typedef std::map<WindowID, smart_objects::SmartObject> AppWindowsTemplates;
  DynamicApplicationDataImpl();
  ~DynamicApplicationDataImpl();
  const smart_objects::SmartObject* help_prompt() const;
  const smart_objects::SmartObject* timeout_prompt() const;
  const smart_objects::SmartObject* vr_help_title() const;
  const smart_objects::SmartObject* vr_help() const;
  const mobile_api::TBTState::eType& tbt_state() const;
  const smart_objects::SmartObject* show_command() const;
  const smart_objects::SmartObject* tbt_show_command() const;
  const smart_objects::SmartObject* keyboard_props() const;
  const smart_objects::SmartObject* menu_title() const;
  const smart_objects::SmartObject* menu_icon() const;
  const smart_objects::SmartObject* menu_layout() const;

  smart_objects::SmartObject day_color_scheme() const OVERRIDE;
  smart_objects::SmartObject night_color_scheme() const OVERRIDE;
  std::string display_layout() const OVERRIDE;
  smart_objects::SmartObjectSPtr display_capabilities() const OVERRIDE;
  smart_objects::SmartObjectSPtr display_capabilities(
      const WindowID window_id) const OVERRIDE;

  void set_window_layout(const WindowID window_id,
                         const std::string& layout) OVERRIDE;

  void set_day_color_scheme(
      const WindowID window_id,
      const smart_objects::SmartObject& color_scheme) OVERRIDE;
  void set_night_color_scheme(
      const WindowID window_id,
      const smart_objects::SmartObject& color_scheme) OVERRIDE;

  std::string window_layout(const WindowID window_id) const OVERRIDE;

  smart_objects::SmartObject day_color_scheme(
      const WindowID window_id) const OVERRIDE;

  smart_objects::SmartObject night_color_scheme(
      const WindowID window_id) const OVERRIDE;

  void load_global_properties(const smart_objects::SmartObject& properties_so);
  void set_help_prompt(const smart_objects::SmartObject& help_prompt);
  void set_timeout_prompt(const smart_objects::SmartObject& timeout_prompt);
  void set_vr_help_title(const smart_objects::SmartObject& vr_help_title);
  void reset_vr_help_title();
  void set_vr_help(const smart_objects::SmartObject& vr_help);
  void reset_vr_help();
  void set_tbt_state(const mobile_api::TBTState::eType& tbt_state);
  void set_show_command(const smart_objects::SmartObject& show_command);
  void set_tbt_show_command(const smart_objects::SmartObject& tbt_show);
  void set_keyboard_props(const smart_objects::SmartObject& keyboard_props);
  void set_menu_title(const smart_objects::SmartObject& menu_title);
  void set_menu_icon(const smart_objects::SmartObject& menu_icon);
  void set_menu_layout(const smart_objects::SmartObject& menu_layout);
  void set_day_color_scheme(const smart_objects::SmartObject& color_scheme);
  void set_night_color_scheme(const smart_objects::SmartObject& color_scheme);
  void set_display_layout(const std::string& layout);
  void set_display_capabilities(
      const smart_objects::SmartObject& display_capabilities) OVERRIDE;
  void remove_window_capability(const WindowID window_id) OVERRIDE;

  /**
   * @brief checks whether a specific menu layout is supported
   * @param menu layout to check
   */
  bool menu_layout_supported(
      const mobile_apis::MenuLayout::eType layout) const OVERRIDE;

  /*
   * @brief Adds a command to the application menu
   * @param[in] internal_id Internal consecutive command id
   * @param[in] command Command to add
   */
  void AddCommand(const uint32_t internal_id,
                  const smart_objects::SmartObject& command);

  /*
   * @brief Deletes all commands from the application menu with the specified
   * command id
   * @param[in] cmd_id Command id
   */
  void RemoveCommand(const uint32_t cmd_id);

  /*
   * @brief Finds command with the specified command id
   * @param[in] cmd_id Command id
   */
  smart_objects::SmartObject FindCommand(const uint32_t cmd_id) OVERRIDE;

  /*
   * @brief Adds a menu to the application
   */
  void AddSubMenu(uint32_t menu_id, const smart_objects::SmartObject& menu);

  /*
   * @brief Deletes menu from the application menu
   */
  void RemoveSubMenu(uint32_t menu_id);

  /*
   * @brief Finds menu with the specified id
   */
  smart_objects::SmartObject FindSubMenu(uint32_t menu_id) const OVERRIDE;

  /*
   * @brief Returns true if sub menu with such name already exist
   */
  bool IsSubMenuNameAlreadyExist(const std::string& name,
                                 const uint32_t parent_id);

  void SetWindowInfo(const WindowID window_id,
                     const smart_objects::SmartObject& window_info) OVERRIDE;

  void RemoveWindowInfo(const WindowID window_id) OVERRIDE;

  /*
   * @brief Adds a interaction choice set to the application
   *
   * @param choice_set_id Unique ID used for this interaction choice set
   * @param choice_set SmartObject that represent choice set
   */
  void AddChoiceSet(uint32_t choice_set_id,
                    const smart_objects::SmartObject& choice_set);

  /*
   * @brief Deletes choice set from the application
   *
   * @param choice_set_id Unique ID of the interaction choice set
   */
  void RemoveChoiceSet(uint32_t choice_set_id);

  /*
   * @brief Finds choice set with the specified choice_set_id id
   *
   * @param choice_set_id Unique ID of the interaction choice set
   */
  smart_objects::SmartObject FindChoiceSet(uint32_t choice_set_id) OVERRIDE;

  /*
   * @brief Adds perform interaction choice set to the application
   *
   * @param correlation_id Unique ID of the request that added this choice set
   * @param choice_set_id Unique ID used for this interaction choice set
   * @param choice_set SmartObject that represents choice set
   */
  void AddPerformInteractionChoiceSet(
      uint32_t correlation_id,
      uint32_t choice_set_id,
      const smart_objects::SmartObject& choice_set);

  /*
   * @brief Deletes entirely perform interaction choice set map
   * @param correlation_id Unique ID of the request that added this choice set
   *
   */
  void DeletePerformInteractionChoiceSet(uint32_t correlation_id);

  /*
   * @brief Retrieves entirely ChoiceSet - VR commands map
   *
   * @return ChoiceSet map that is currently in use
   */
  inline DataAccessor<PerformChoiceSetMap> performinteraction_choice_set_map()
      const;

  /*
   * @brief Retrieve application commands
   */
  inline DataAccessor<CommandsMap> commands_map() const;

  /*
   * @brief Retrieve application sub menus
   */
  inline DataAccessor<SubMenuMap> sub_menu_map() const;

  /*
   * @brief Retrieve application choice set map
   */
  inline DataAccessor<ChoiceSetMap> choice_set_map() const;

  DataAccessor<WindowParamsMap> window_optional_params_map() const;

  DisplayCapabilitiesBuilder& display_capabilities_builder();

  /*
   * @brief Sets perform interaction state
   *
   * @param active Current state of the perform interaction
   */
  void set_perform_interaction_active(uint32_t active);

  /*
   * @brief Retrieves perform interaction state
   *
   * @return TRUE if perform interaction active, otherwise FALSE
   */
  inline uint32_t is_perform_interaction_active() const;

  /*
   * @brief Sets the mode for perform interaction: UI/VR/BOTH
   *
   * @param mode Mode that was selected (MENU; VR; BOTH)
   */
  void set_perform_interaction_mode(int32_t mode);

  /*
   * @brief Retrieve the mode that was PerformInteraction sent in
   *
   * @return mode of PerformInteraction
   */
  inline int32_t perform_interaction_mode() const;

  /*
   * @brief Sets reset global properties state
   *
   * @param active Current state of the reset global properties
   */
  void set_reset_global_properties_active(bool active);

  /*
   * @brief Retrieves reset global properties state
   *
   * @return TRUE if perform interaction active, otherwise FALSE
   */
  inline bool is_reset_global_properties_active() const;

 protected:
  smart_objects::SmartObject* help_prompt_;
  smart_objects::SmartObject* timeout_prompt_;
  smart_objects::SmartObject* vr_help_title_;
  smart_objects::SmartObject* vr_help_;
  mobile_api::TBTState::eType tbt_state_;
  smart_objects::SmartObject* show_command_;
  smart_objects::SmartObject* keyboard_props_;
  smart_objects::SmartObject* menu_title_;
  smart_objects::SmartObject* menu_icon_;
  smart_objects::SmartObject* menu_layout_;
  smart_objects::SmartObject* tbt_show_command_;
  smart_objects::SmartObjectSPtr display_capabilities_;
  AppWindowsTemplates window_templates_;

  CommandsMap commands_;
  mutable std::shared_ptr<sync_primitives::RecursiveLock> commands_lock_ptr_;
  SubMenuMap sub_menu_;
  mutable std::shared_ptr<sync_primitives::RecursiveLock> sub_menu_lock_ptr_;
  ChoiceSetMap choice_set_map_;
  mutable std::shared_ptr<sync_primitives::RecursiveLock>
      choice_set_map_lock_ptr_;
  PerformChoiceSetMap performinteraction_choice_set_map_;
  mutable std::shared_ptr<sync_primitives::RecursiveLock>
      performinteraction_choice_set_lock_ptr_;
  WindowParamsMap window_params_map_;
  mutable std::shared_ptr<sync_primitives::RecursiveLock>
      window_params_map_lock_ptr_;
  uint32_t is_perform_interaction_active_;
  bool is_reset_global_properties_active_;
  int32_t perform_interaction_mode_;
  DisplayCapabilitiesBuilder display_capabilities_builder_;

 private:
  void SetGlobalProperties(
      const smart_objects::SmartObject& param,
      void (DynamicApplicationData::*callback)(
          const ns_smart_device_link::ns_smart_objects::SmartObject&));
  DISALLOW_COPY_AND_ASSIGN(DynamicApplicationDataImpl);
};

DataAccessor<CommandsMap> DynamicApplicationDataImpl::commands_map() const {
  return DataAccessor<CommandsMap>(commands_, commands_lock_ptr_);
}

DataAccessor<SubMenuMap> DynamicApplicationDataImpl::sub_menu_map() const {
  return DataAccessor<SubMenuMap>(sub_menu_, sub_menu_lock_ptr_);
}

DataAccessor<ChoiceSetMap> DynamicApplicationDataImpl::choice_set_map() const {
  return DataAccessor<ChoiceSetMap>(choice_set_map_, choice_set_map_lock_ptr_);
}

DataAccessor<PerformChoiceSetMap>
DynamicApplicationDataImpl::performinteraction_choice_set_map() const {
  return DataAccessor<PerformChoiceSetMap>(
      performinteraction_choice_set_map_,
      performinteraction_choice_set_lock_ptr_);
}

uint32_t DynamicApplicationDataImpl::is_perform_interaction_active() const {
  return is_perform_interaction_active_;
}

bool DynamicApplicationDataImpl::is_reset_global_properties_active() const {
  return is_reset_global_properties_active_;
}

inline int32_t DynamicApplicationDataImpl::perform_interaction_mode() const {
  return perform_interaction_mode_;
}

}  //  namespace application_manager

#endif  // SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_APPLICATION_DATA_IMPL_H_
