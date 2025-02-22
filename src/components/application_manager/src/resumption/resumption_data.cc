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

#include "application_manager/resumption/resumption_data.h"
#include "application_manager/application_manager_settings.h"
#include "application_manager/smart_object_keys.h"
#include "utils/logger.h"

namespace resumption {

SDL_CREATE_LOG_VARIABLE("Resumption")

ResumptionData::ResumptionData(
    const application_manager::ApplicationManager& application_manager)
    : application_manager_(application_manager) {}

smart_objects::SmartObject ResumptionData::GetApplicationCommands(
    app_mngr::ApplicationConstSharedPtr application) const {
  using namespace app_mngr;
  SDL_LOG_AUTO_TRACE();
  smart_objects::SmartObject commands_array(smart_objects::SmartType_Array);
  DCHECK_OR_RETURN(application, commands_array);
  if (!application) {
    SDL_LOG_ERROR("NULL Pointer App");
    return commands_array;
  }
  const DataAccessor<CommandsMap> accessor = application->commands_map();
  const CommandsMap& commands = accessor.GetData();
  CommandsMap::const_iterator it = commands.begin();
  for (int i = 0; it != commands.end(); ++it, ++i) {
    commands_array[i] = *(it->second);
  }
  return commands_array;
}

smart_objects::SmartObject ResumptionData::GetApplicationSubMenus(
    app_mngr::ApplicationConstSharedPtr application) const {
  using namespace app_mngr;
  SDL_LOG_AUTO_TRACE();

  DCHECK(application.get());
  smart_objects::SmartObject submenues_array =
      smart_objects::SmartObject(smart_objects::SmartType_Array);

  if (!application) {
    SDL_LOG_ERROR("NULL Pointer App");
    return submenues_array;
  }
  const DataAccessor<SubMenuMap> accessor = application->sub_menu_map();
  const SubMenuMap& sub_menus = accessor.GetData();
  SubMenuMap::const_iterator it = sub_menus.begin();
  for (int i = 0; it != sub_menus.end(); ++it, ++i) {
    submenues_array[i] = *(it->second);
  }
  return submenues_array;
}

smart_objects::SmartObject ResumptionData::GetApplicationInteractionChoiseSets(
    app_mngr::ApplicationConstSharedPtr application) const {
  using namespace app_mngr;
  SDL_LOG_AUTO_TRACE();

  DCHECK(application.get());
  smart_objects::SmartObject interaction_choice_set_array =
      smart_objects::SmartObject(smart_objects::SmartType_Array);
  if (!application) {
    SDL_LOG_ERROR("NULL Pointer App");
    return interaction_choice_set_array;
  }
  const DataAccessor<ChoiceSetMap> accessor = application->choice_set_map();
  const ChoiceSetMap& choices = accessor.GetData();
  ChoiceSetMap::const_iterator it = choices.begin();
  for (int i = 0; it != choices.end(); ++it, ++i) {
    interaction_choice_set_array[i] = *(it->second);
  }
  return interaction_choice_set_array;
}

smart_objects::SmartObject ResumptionData::GetApplicationGlobalProperties(
    app_mngr::ApplicationConstSharedPtr application) const {
  using namespace app_mngr;
  SDL_LOG_AUTO_TRACE();

  DCHECK(application.get());
  smart_objects::SmartObject global_properties =
      smart_objects::SmartObject(smart_objects::SmartType_Map);
  if (!application) {
    SDL_LOG_ERROR("NULL Pointer App");
    return global_properties;
  }

  global_properties[strings::help_prompt] =
      PointerToSmartObj(application->help_prompt());
  global_properties[strings::timeout_prompt] =
      PointerToSmartObj(application->timeout_prompt());
  global_properties[strings::vr_help] =
      PointerToSmartObj(application->vr_help());
  global_properties[strings::vr_help_title] =
      PointerToSmartObj(application->vr_help_title());
  global_properties[strings::keyboard_properties] =
      PointerToSmartObj(application->keyboard_props());
  global_properties[strings::menu_title] =
      PointerToSmartObj(application->menu_title());
  global_properties[strings::menu_icon] =
      PointerToSmartObj(application->menu_icon());
  global_properties[strings::menu_layout] =
      PointerToSmartObj(application->menu_layout());
  return global_properties;
}

smart_objects::SmartObject ResumptionData::GetApplicationSubscriptions(
    app_mngr::ApplicationConstSharedPtr application) const {
  using namespace app_mngr;
  SDL_LOG_AUTO_TRACE();
  DCHECK(application.get());
  smart_objects::SmartObject subscriptions =
      smart_objects::SmartObject(smart_objects::SmartType_Map);
  if (!application) {
    SDL_LOG_ERROR("NULL Pointer App");
    return subscriptions;
  }
  SDL_LOG_DEBUG("app_id:" << application->app_id());

  {
    DataAccessor<ButtonSubscriptions> button_accessor =
        application->SubscribedButtons();

    const ButtonSubscriptions& button_subscriptions = button_accessor.GetData();

    SDL_LOG_DEBUG("SubscribedButtons:" << button_subscriptions.size());
    Append(button_subscriptions.begin(),
           button_subscriptions.end(),
           strings::application_buttons,
           subscriptions);
  }

  for (auto extension : application->Extensions()) {
    extension->SaveResumptionData(subscriptions);
  }

  return subscriptions;
}

smart_objects::SmartObject ResumptionData::GetApplicationFiles(
    app_mngr::ApplicationConstSharedPtr application) const {
  using namespace app_mngr;
  SDL_LOG_AUTO_TRACE();
  DCHECK(application.get());
  SDL_LOG_TRACE("ENTER app_id:" << application->app_id());

  smart_objects::SmartObject files =
      smart_objects::SmartObject(smart_objects::SmartType_Array);
  if (!application) {
    SDL_LOG_ERROR("NULL Pointer App");
    return files;
  }

  const AppFilesMap& app_files = application->getAppFiles();
  int i = 0;
  for (AppFilesMap::const_iterator file_it = app_files.begin();
       file_it != app_files.end();
       ++file_it) {
    const AppFile& file = file_it->second;
    if (file.is_persistent) {
      smart_objects::SmartObject file_data =
          smart_objects::SmartObject(smart_objects::SmartType_Map);
      file_data[strings::persistent_file] = file.is_persistent;
      file_data[strings::is_download_complete] = file.is_download_complete;
      file_data[strings::sync_file_name] = file.file_name;
      file_data[strings::file_type] = file.file_type;
      files[i++] = file_data;
    }
  }
  return files;
}

smart_objects::SmartObject ResumptionData::GetApplicationWidgetsInfo(
    app_mngr::ApplicationConstSharedPtr application) const {
  using namespace app_mngr;
  SDL_LOG_AUTO_TRACE();
  smart_objects::SmartObject windows_info =
      smart_objects::SmartObject(smart_objects::SmartType_Array);
  DCHECK_OR_RETURN(application, windows_info);
  const auto window_ids = application->GetWindowIds();
  const auto& window_optional_params_map =
      application->window_optional_params_map().GetData();
  for (const auto& window_id : window_ids) {
    const HmiStatePtr hmi_state = application->CurrentHmiState(window_id);
    if (mobile_apis::WindowType::WIDGET != hmi_state->window_type()) {
      continue;
    }
    auto info = CreateWindowInfoSO(
        window_id, hmi_state->window_type(), window_optional_params_map);

    windows_info[windows_info.length()] = info;
  }
  return windows_info;
}

smart_objects::SmartObject ResumptionData::CreateWindowInfoSO(
    const app_mngr::WindowID window_id,
    const mobile_apis::WindowType::eType window_type,
    const app_mngr::WindowParamsMap& window_optional_params_map) const {
  using namespace app_mngr;
  SDL_LOG_AUTO_TRACE();
  auto window_info = smart_objects::SmartObject(smart_objects::SmartType_Map);

  window_info[strings::window_id] = window_id;
  window_info[strings::window_type] = window_type;

  const auto& it_info = window_optional_params_map.find(window_id);
  if (window_optional_params_map.end() != it_info) {
    const auto keys = it_info->second->enumerate();

    for (const auto& key : keys) {
      window_info[key] = (*it_info->second)[key];
    }
  }

  return window_info;
}

smart_objects::SmartObject ResumptionData::PointerToSmartObj(
    const smart_objects::SmartObject* ptr) const {
  SDL_LOG_AUTO_TRACE();
  smart_objects::SmartObject temp;
  if (ptr != NULL) {
    temp = *ptr;
  }
  return temp;
}
}  // namespace resumption
