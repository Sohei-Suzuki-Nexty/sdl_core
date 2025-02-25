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

#include "sdl_rpc_plugin/commands/hmi/sdl_activate_app_request.h"
#include "application_manager/message_helper.h"
#include "application_manager/policies/policy_handler_interface.h"
#include "application_manager/state_controller.h"

namespace sdl_rpc_plugin {
using namespace application_manager;

namespace commands {

namespace {
struct ProtoV4AppsOnDevice : std::unary_function<ApplicationSharedPtr, bool> {
  connection_handler::DeviceHandle handle_;
  explicit ProtoV4AppsOnDevice(const connection_handler::DeviceHandle handle)
      : handle_(handle) {}
  bool operator()(const ApplicationSharedPtr app) const {
    return app ? handle_ == app->device() &&
                     Message::is_sufficient_version(
                         protocol_handler::MajorProtocolVersion::
                             PROTOCOL_VERSION_4,
                         app->protocol_version())
               : false;
  }
};

struct ForegroundApp
    : std::unary_function<SDLActivateAppRequest::V4ProtoApps::value_type,
                          bool> {
  bool operator()(
      const SDLActivateAppRequest::V4ProtoApps::value_type ptr) const {
    return ptr ? ptr->is_foreground() : false;
  }
};

struct SendLaunchApp
    : std::unary_function<SDLActivateAppRequest::V4ProtoApps::value_type,
                          void> {
  ApplicationConstSharedPtr app_to_launch_;
  ApplicationManager& application_manager_;
  SendLaunchApp(ApplicationConstSharedPtr app_to_launch, ApplicationManager& am)
      : app_to_launch_(app_to_launch), application_manager_(am) {}
  void operator()(
      const SDLActivateAppRequest::V4ProtoApps::value_type ptr) const {
    MessageHelper::SendLaunchApp((*ptr).app_id(),
                                 app_to_launch_->SchemaUrl(),
                                 app_to_launch_->PackageName(),
                                 application_manager_);
    return;
  }
};
}  // namespace

SDL_CREATE_LOG_VARIABLE("Commands")

SDLActivateAppRequest::SDLActivateAppRequest(
    const application_manager::commands::MessageSharedPtr& message,
    ApplicationManager& application_manager,
    rpc_service::RPCService& rpc_service,
    HMICapabilities& hmi_capabilities,
    policy::PolicyHandlerInterface& policy_handle)
    : RequestFromHMI(message,
                     application_manager,
                     rpc_service,
                     hmi_capabilities,
                     policy_handle) {}

SDLActivateAppRequest::~SDLActivateAppRequest() {}

uint32_t SDLActivateAppRequest::app_id() const {
  using namespace strings;
  if (!(*message_).keyExists(msg_params)) {
    SDL_LOG_DEBUG(msg_params << " section is absent in the message.");
    return 0;
  }
  if (!(*message_)[msg_params].keyExists(strings::app_id)) {
    SDL_LOG_DEBUG(strings::app_id << " section is absent in the message.");
    return 0;
  }
  return (*message_)[msg_params][strings::app_id].asUInt();
}

#ifdef EXTERNAL_PROPRIETARY_MODE
void SDLActivateAppRequest::Run() {
  SDL_LOG_AUTO_TRACE();
  using namespace hmi_apis::FunctionID;

  ApplicationConstSharedPtr app =
      application_manager_.WaitingApplicationByID(app_id());

  if (!app) {
    app = application_manager_.application(app_id());
    if (!app) {
      SDL_LOG_WARN("Can't find application within waiting apps: " << app_id());
      return;
    }
  }

  if (application_manager_.state_controller().IsStateActive(
          HmiState::STATE_ID_DEACTIVATE_HMI)) {
    SDL_LOG_DEBUG(
        "DeactivateHmi state is active. "
        "Sends response with result code REJECTED");
    SendErrorResponse(correlation_id(),
                      static_cast<eType>(function_id()),
                      hmi_apis::Common_Result::REJECTED,
                      "HMIDeactivate is active");
    return;
  }

  if (app->app_id() > 0 || app->IsRegistered()) {
    auto main_state =
        app->CurrentHmiState(mobile_apis::PredefinedWindows::DEFAULT_WINDOW);
    if (mobile_apis::HMILevel::INVALID_ENUM == main_state->hmi_level()) {
      SDL_LOG_DEBUG(
          "Application registration is not completed, HMI level hasn't set "
          "yet, postpone activation");
      auto& postponed_activation_ctrl = application_manager_.state_controller()
                                            .GetPostponedActivationController();
      postponed_activation_ctrl.AddAppToActivate(app->app_id(),
                                                 correlation_id());
      return;
    }
  }

  const uint32_t application_id = app_id();
  if (app->IsRegistered()) {
    SDL_LOG_DEBUG("Application is registered. Activating.");
    policy_handler_.OnActivateApp(application_id, correlation_id());
    return;
  }

  if (app->is_cloud_app()) {
    SDL_LOG_DEBUG("Starting cloud application.");
    const ApplicationManagerSettings& settings =
        application_manager_.get_settings();
    uint32_t total_retry_timeout = (settings.cloud_app_retry_timeout() *
                                    settings.cloud_app_max_retry_attempts());
    application_manager_.UpdateRequestTimeout(
        0, correlation_id(), default_timeout_ + total_retry_timeout);
    subscribe_on_event(BasicCommunication_OnAppRegistered);
    application_manager_.connection_handler().ConnectToDevice(app->device());
    return;
  }

  connection_handler::DeviceHandle device_handle = app->device();
  SDL_LOG_ERROR(
      "Can't find regular foreground app with the same connection id: "
      << device_handle);
  SendErrorResponse(correlation_id(),
                    SDL_ActivateApp,
                    hmi_apis::Common_Result::NO_APPS_REGISTERED,
                    "");
}

#else  // EXTERNAL_PROPRIETARY_MODE
void SDLActivateAppRequest::Run() {
  SDL_LOG_AUTO_TRACE();
  using namespace hmi_apis::FunctionID;
  using namespace hmi_apis::Common_Result;

  const uint32_t application_id = app_id();

  ApplicationConstSharedPtr app_to_activate =
      application_manager_.application(application_id);

  if (!app_to_activate) {
    // Here is the hack - in fact SDL gets hmi_app_id in appID field and
    // replaces it with connection_key only for normally registered apps, but
    // for apps_to_be_registered (waiting) it keeps original value (hmi_app_id)
    // so method does lookup for hmi_app_id
    app_to_activate =
        application_manager_.WaitingApplicationByID(application_id);

    if (!app_to_activate) {
      SDL_LOG_WARN(
          "Can't find application within waiting apps: " << application_id);
      return;
    }
  }

  SDL_LOG_DEBUG("Found application to activate. Application id is "
                << app_to_activate->app_id());

  if (application_manager_.state_controller().IsStateActive(
          HmiState::StateID::STATE_ID_DEACTIVATE_HMI)) {
    SDL_LOG_WARN(
        "DeactivateHmi state is active. "
        "Sends response with result code REJECTED");
    SendErrorResponse(correlation_id(),
                      static_cast<hmi_apis::FunctionID::eType>(function_id()),
                      hmi_apis::Common_Result::REJECTED,
                      "HMIDeactivate is active");
    return;
  }

  if (app_to_activate->app_id() > 0 || app_to_activate->IsRegistered()) {
    auto main_state = app_to_activate->CurrentHmiState(
        mobile_apis::PredefinedWindows::DEFAULT_WINDOW);
    if (mobile_apis::HMILevel::INVALID_ENUM == main_state->hmi_level()) {
      SDL_LOG_DEBUG(
          "Application registration is not completed, HMI level hasn't set "
          "yet, postpone activation");
      auto& postponed_activation_ctrl = application_manager_.state_controller()
                                            .GetPostponedActivationController();
      postponed_activation_ctrl.AddAppToActivate(app_to_activate->app_id(),
                                                 correlation_id());
      return;
    }
  }

  if (app_to_activate->IsRegistered()) {
    SDL_LOG_DEBUG("Application is registered. Activating.");
    policy_handler_.OnActivateApp(application_id, correlation_id());
    return;
  }

  if (app_to_activate->is_cloud_app()) {
    SDL_LOG_DEBUG("Starting cloud application.");
    const ApplicationManagerSettings& settings =
        application_manager_.get_settings();
    uint32_t total_retry_timeout = (settings.cloud_app_retry_timeout() *
                                    settings.cloud_app_max_retry_attempts());
    application_manager_.UpdateRequestTimeout(
        0, correlation_id(), default_timeout_ + total_retry_timeout);
    subscribe_on_event(BasicCommunication_OnAppRegistered);
    application_manager_.connection_handler().ConnectToDevice(
        app_to_activate->device());
    return;
  }

  connection_handler::DeviceHandle device_handle = app_to_activate->device();
  ApplicationSharedPtr foreground_v4_app = get_foreground_app(device_handle);
  V4ProtoApps v4_proto_apps = get_v4_proto_apps(device_handle);

  if (!foreground_v4_app && v4_proto_apps.empty()) {
    SDL_LOG_ERROR(
        "Can't find regular foreground app with the same "
        "connection id: "
        << device_handle);
    SendErrorResponse(
        correlation_id(), SDL_ActivateApp, NO_APPS_REGISTERED, "");
    return;
  }

  SDL_LOG_DEBUG(
      "Application is not registered yet. "
      "Sending launch request.");

  if (foreground_v4_app) {
    SDL_LOG_DEBUG("Sending request to foreground application.");
    MessageHelper::SendLaunchApp(foreground_v4_app->app_id(),
                                 app_to_activate->SchemaUrl(),
                                 app_to_activate->PackageName(),
                                 application_manager_);
  } else {
    SDL_LOG_DEBUG(
        "No preffered (foreground) application is found. "
        "Sending request to all v4 applications.");
    std::for_each(v4_proto_apps.begin(),
                  v4_proto_apps.end(),
                  SendLaunchApp(app_to_activate, application_manager_));
  }

  subscribe_on_event(BasicCommunication_OnAppRegistered);
}

#endif  // EXTERNAL_PROPRIETARY_MODE
void SDLActivateAppRequest::OnTimeOut() {
  using namespace hmi_apis::FunctionID;
  using namespace hmi_apis::Common_Result;
  using namespace application_manager;
  unsubscribe_from_event(BasicCommunication_OnAppRegistered);
  SendErrorResponse(correlation_id(),
                    SDL_ActivateApp,
                    APPLICATION_NOT_REGISTERED,
                    "App registration timed out");
}

void SDLActivateAppRequest::on_event(const event_engine::Event& event) {
  using namespace hmi_apis::FunctionID;
  if (event.id() != BasicCommunication_OnAppRegistered) {
    return;
  }
  unsubscribe_from_event(BasicCommunication_OnAppRegistered);

  // Have to use HMI app id from event, since HMI app id from original request
  // message will be changed after app, initially requested for launch via
  // SDL.ActivateApp, will be registered
  const uint32_t hmi_application_id = hmi_app_id(event.smart_object());

  ApplicationSharedPtr app =
      application_manager_.application_by_hmi_app(hmi_application_id);
  if (!app) {
    SDL_LOG_ERROR(
        "Application not found by HMI app id: " << hmi_application_id);
    return;
  }

  auto main_state =
      app->CurrentHmiState(mobile_apis::PredefinedWindows::DEFAULT_WINDOW);
  if (mobile_apis::HMILevel::INVALID_ENUM == main_state->hmi_level()) {
    SDL_LOG_DEBUG(
        "Application registration is not completed, HMI level hasn't set "
        "yet, postpone activation");
    auto& postponed_activation_ctrl = application_manager_.state_controller()
                                          .GetPostponedActivationController();
    postponed_activation_ctrl.AddAppToActivate(app->app_id(), correlation_id());
    return;
  }

  policy_handler_.OnActivateApp(app->app_id(), correlation_id());
}

uint32_t SDLActivateAppRequest::hmi_app_id(
    const smart_objects::SmartObject& so) const {
  using namespace strings;
  if (!so.keyExists(params)) {
    SDL_LOG_DEBUG(params << " section is absent in the message.");
    return 0;
  }
  if (!so[msg_params].keyExists(application)) {
    SDL_LOG_DEBUG(application << " section is absent in the message.");
    return 0;
  }
  if (!so[msg_params][application].keyExists(strings::app_id)) {
    SDL_LOG_DEBUG(strings::app_id << " section is absent in the message.");
    return 0;
  }
  return so[msg_params][application][strings::app_id].asUInt();
}

SDLActivateAppRequest::V4ProtoApps SDLActivateAppRequest::get_v4_proto_apps(
    const connection_handler::DeviceHandle handle) const {
  const ApplicationSet app_list = application_manager_.applications().GetData();
  V4ProtoApps v4_proto_apps;
  std::copy_if(app_list.begin(),
               app_list.end(),
               std::back_inserter(v4_proto_apps),
               ProtoV4AppsOnDevice(handle));
  return v4_proto_apps;
}

ApplicationSharedPtr SDLActivateAppRequest::get_foreground_app(
    const connection_handler::DeviceHandle handle) const {
  V4ProtoApps v4_proto_apps = get_v4_proto_apps(handle);
  V4ProtoApps::iterator foreground_app =
      std::find_if(v4_proto_apps.begin(), v4_proto_apps.end(), ForegroundApp());
  return foreground_app != v4_proto_apps.end() ? *foreground_app
                                               : ApplicationSharedPtr();
}

}  // namespace commands
}  // namespace sdl_rpc_plugin
