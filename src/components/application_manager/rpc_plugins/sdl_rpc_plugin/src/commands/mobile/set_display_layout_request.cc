/*

 Copyright (c) 2018, Ford Motor Company
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following
 disclaimer in the documentation and/or other materials provided with the
 distribution.

 Neither the name of the Ford Motor Company nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include "sdl_rpc_plugin/commands/mobile/set_display_layout_request.h"

#include "application_manager/application_impl.h"
#include "application_manager/message_helper.h"

namespace sdl_rpc_plugin {
using namespace application_manager;

namespace commands {

SDL_CREATE_LOG_VARIABLE("Commands")

SetDisplayLayoutRequest::SetDisplayLayoutRequest(
    const application_manager::commands::MessageSharedPtr& message,
    ApplicationManager& application_manager,
    app_mngr::rpc_service::RPCService& rpc_service,
    app_mngr::HMICapabilities& hmi_capabilities,
    policy::PolicyHandlerInterface& policy_handler)
    : RequestFromMobileImpl(message,
                            application_manager,
                            rpc_service,
                            hmi_capabilities,
                            policy_handler) {}

SetDisplayLayoutRequest::~SetDisplayLayoutRequest() {}

void SetDisplayLayoutRequest::Run() {
  SDL_LOG_AUTO_TRACE();
  ApplicationSharedPtr app = application_manager_.application(connection_key());

  if (!app) {
    SDL_LOG_ERROR("Application is not registered");
    SendResponse(false, mobile_apis::Result::APPLICATION_NOT_REGISTERED);
    return;
  }

  const smart_objects::SmartObject& msg_params =
      (*message_)[strings::msg_params];

  std::string old_layout = app->display_layout();
  std::string new_layout = "";

  if (msg_params.keyExists(strings::display_layout)) {
    new_layout = msg_params[strings::display_layout].asString();
  }

  if (new_layout != old_layout && !new_layout.empty()) {
    // Template switched, hence allow any color change
    SDL_LOG_DEBUG("SetDisplayLayoutRequest New Layout: " << new_layout);
    app->set_display_layout(new_layout);
  } else {
    SDL_LOG_DEBUG("SetDisplayLayoutRequest No Layout Change");
    // Template layout is the same as previous layout
    // Reject message if colors are set
    if (msg_params.keyExists(strings::day_color_scheme) &&
        app->day_color_scheme().getType() != smart_objects::SmartType_Null &&
        msg_params[strings::day_color_scheme] != app->day_color_scheme()) {
      // Color scheme param exists and has been previously set,
      // hence do not allow color change
      SDL_LOG_DEBUG("Reject Day Color Scheme Change");
      SendResponse(false, mobile_apis::Result::REJECTED);
      return;
    }

    if (msg_params.keyExists(strings::night_color_scheme) &&
        app->night_color_scheme().getType() != smart_objects::SmartType_Null &&
        msg_params[strings::night_color_scheme] != app->night_color_scheme()) {
      // Color scheme param exists and has been previously set,
      // hence do not allow color change
      SDL_LOG_DEBUG("Reject Night Color Scheme Change");
      SendResponse(false, mobile_apis::Result::REJECTED);
      return;
    }
  }

  if (msg_params.keyExists(strings::day_color_scheme)) {
    SDL_LOG_DEBUG("Allow Day Color Scheme Change");
    app->set_day_color_scheme(msg_params[strings::day_color_scheme]);
  }

  if (msg_params.keyExists(strings::night_color_scheme)) {
    SDL_LOG_DEBUG("Allow Night Color Scheme Change");
    app->set_night_color_scheme(msg_params[strings::night_color_scheme]);
  }

  (*message_)[strings::msg_params][strings::app_id] = app->app_id();
  StartAwaitForInterface(HmiInterfaces::HMI_INTERFACE_UI);
  SendHMIRequest(hmi_apis::FunctionID::UI_SetDisplayLayout,
                 &((*message_)[strings::msg_params]),
                 true);
}

void SetDisplayLayoutRequest::on_event(const event_engine::Event& event) {
  SDL_LOG_AUTO_TRACE();

  ApplicationSharedPtr app = application_manager_.application(connection_key());

  if (!app) {
    SDL_LOG_ERROR("Application is not registered");
    SendResponse(false, mobile_apis::Result::APPLICATION_NOT_REGISTERED);
    return;
  }

  const smart_objects::SmartObject& message = event.smart_object();
  switch (event.id()) {
    case hmi_apis::FunctionID::UI_SetDisplayLayout: {
      SDL_LOG_INFO("Received UI_SetDisplayLayout event");
      EndAwaitForInterface(HmiInterfaces::HMI_INTERFACE_UI);
      hmi_apis::Common_Result::eType result_code =
          static_cast<hmi_apis::Common_Result::eType>(
              message[strings::params][hmi_response::code].asInt());
      const bool response_success = PrepareResultForMobileResponse(
          result_code, HmiInterfaces::HMI_INTERFACE_UI);
      std::string info;
      GetInfo(message, info);
      smart_objects::SmartObject msg_params = message[strings::msg_params];
      if (response_success) {
        HMICapabilities& hmi_capabilities = hmi_capabilities_;

        // In case templates_available is empty copy from hmi capabilities
        if (msg_params.keyExists(hmi_response::display_capabilities)) {
          if (0 == msg_params[hmi_response::display_capabilities]
                             [hmi_response::templates_available]
                                 .length()) {
            auto display_capabilities = hmi_capabilities.display_capabilities();
            if (display_capabilities) {
              msg_params[hmi_response::display_capabilities]
                        [hmi_response::templates_available] =
                            display_capabilities->getElement(
                                hmi_response::templates_available);
            }
          }
        }
        const Version& app_version = app->version();
        if (app_version.max_supported_api_version >= APIVersion::kAPIV6) {
          // In case of successful response warn user that this RPC is
          // deprecated from 6.0 and higher API versions
          result_code = hmi_apis::Common_Result::WARNINGS;
          info =
              "The RPC is deprecated and will be removed in a future version. "
              "The requested display layout is set to the main window. Please "
              "use `Show.templateConfiguration` instead.";
        }
      }

      SendResponse(response_success,
                   MessageHelper::HMIToMobileResult(result_code),
                   info.empty() ? nullptr : info.c_str(),
                   &msg_params);
      break;
    }
    default: {
      SDL_LOG_ERROR("Received unknown event " << event.id());
      return;
    }
  }
}

}  // namespace commands

}  // namespace sdl_rpc_plugin
