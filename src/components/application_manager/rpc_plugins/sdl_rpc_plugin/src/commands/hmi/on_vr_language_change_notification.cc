/*
 * Copyright (c) 2018, Ford Motor Company
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

#include "sdl_rpc_plugin/commands/hmi/on_vr_language_change_notification.h"

#include "application_manager/application_impl.h"
#include "application_manager/message_helper.h"
#include "application_manager/rpc_service.h"
#include "application_manager/state_controller.h"
#include "interfaces/MOBILE_API.h"

namespace sdl_rpc_plugin {
using namespace application_manager;

namespace commands {

SDL_CREATE_LOG_VARIABLE("Commands")

OnVRLanguageChangeNotification::OnVRLanguageChangeNotification(
    const application_manager::commands::MessageSharedPtr& message,
    ApplicationManager& application_manager,
    rpc_service::RPCService& rpc_service,
    HMICapabilities& hmi_capabilities,
    policy::PolicyHandlerInterface& policy_handle)
    : NotificationFromHMI(message,
                          application_manager,
                          rpc_service,
                          hmi_capabilities,
                          policy_handle) {}

OnVRLanguageChangeNotification::~OnVRLanguageChangeNotification() {}

void OnVRLanguageChangeNotification::Run() {
  SDL_LOG_AUTO_TRACE();

  hmi_capabilities_.set_active_vr_language(
      static_cast<hmi_apis::Common_Language::eType>(
          (*message_)[strings::msg_params][strings::language].asInt()));

  std::vector<std::string> sections_to_update{hmi_response::language};
  if (!hmi_capabilities_.SaveCachedCapabilitiesToFile(
          hmi_interface::vr, sections_to_update, message_->getSchema())) {
    SDL_LOG_ERROR("Failed to save VR.OnLanguageChange response to cache");
  }

  (*message_)[strings::msg_params][strings::hmi_display_language] =
      hmi_capabilities_.active_ui_language();

  (*message_)[strings::params][strings::function_id] =
      static_cast<int32_t>(mobile_apis::FunctionID::OnLanguageChangeID);

  const auto applications = application_manager_.applications().GetData();
  for (auto app : applications) {
    if (!app->IsRegistered()) {
      SDL_LOG_DEBUG("Skipping app "
                    << app->app_id()
                    << " which has not finished the registration process");
      continue;
    }

    (*message_)[strings::params][strings::connection_key] = app->app_id();
    SendNotificationToMobile(message_);

    if (static_cast<int32_t>(app->language()) !=
        (*message_)[strings::msg_params][strings::language].asInt()) {
      application_manager_.state_controller().SetRegularState(
          app,
          mobile_apis::PredefinedWindows::DEFAULT_WINDOW,
          mobile_apis::HMILevel::HMI_NONE,
          false);

      rpc_service_.ManageMobileCommand(
          MessageHelper::GetOnAppInterfaceUnregisteredNotificationToMobile(
              app->app_id(),
              mobile_api::AppInterfaceUnregisteredReason::LANGUAGE_CHANGE),
          SOURCE_SDL);
      application_manager_.UnregisterApplication(
          app->app_id(), mobile_apis::Result::SUCCESS, false);
    }
  }
}

}  // namespace commands

}  // namespace sdl_rpc_plugin
