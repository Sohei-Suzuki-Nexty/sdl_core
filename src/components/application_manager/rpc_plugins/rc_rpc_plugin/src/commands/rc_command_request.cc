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

#include "rc_rpc_plugin/commands/rc_command_request.h"

#include <sstream>

#include "application_manager/hmi_interfaces.h"
#include "application_manager/message_helper.h"
#include "application_manager/policies/policy_handler_interface.h"
#include "rc_rpc_plugin/commands/rc_command_request.h"
#include "rc_rpc_plugin/interior_data_cache.h"
#include "rc_rpc_plugin/rc_helpers.h"
#include "rc_rpc_plugin/rc_module_constants.h"
#include "smart_objects/enum_schema_item.h"

SDL_CREATE_LOG_VARIABLE("Commands")

namespace rc_rpc_plugin {
namespace commands {

RCCommandRequest::RCCommandRequest(
    const app_mngr::commands::MessageSharedPtr& message,
    const RCCommandParams& params)
    : application_manager::commands::RequestFromMobileImpl(
          message,
          params.application_manager_,
          params.rpc_service_,
          params.hmi_capabilities_,
          params.policy_handler_)
    , is_subscribed(false)
    , auto_allowed_(false)
    , resource_allocation_manager_(params.resource_allocation_manager_)
    , interior_data_cache_(params.interior_data_cache_)
    , interior_data_manager_(params.interior_data_manager_)
    , rc_capabilities_manager_(params.rc_capabilities_manager_)
    , rc_consent_manager_(params.rc_consent_manager_) {}

RCCommandRequest::~RCCommandRequest() {}

bool RCCommandRequest::IsInterfaceAvailable(
    const app_mngr::HmiInterfaces::InterfaceID interface) const {
  app_mngr::HmiInterfaces& hmi_interfaces =
      application_manager_.hmi_interfaces();
  const app_mngr::HmiInterfaces::InterfaceState state =
      hmi_interfaces.GetInterfaceState(interface);
  return app_mngr::HmiInterfaces::STATE_NOT_AVAILABLE != state;
}

void RCCommandRequest::OnTimeOut() {
  SDL_LOG_AUTO_TRACE();
  const std::string module_type = ModuleType();
  SetResourceState(module_type, ResourceState::FREE);

  RequestFromMobileImpl::OnTimeOut();
}

bool RCCommandRequest::CheckDriverConsent() {
  SDL_LOG_AUTO_TRACE();
  app_mngr::ApplicationSharedPtr app =
      application_manager_.application(connection_key());

  const std::string module_type = ModuleType();
  rc_rpc_plugin::TypeAccess access = CheckModule(module_type, app);

  if (rc_rpc_plugin::kAllowed == access) {
    set_auto_allowed(true);
    return true;
  }
  SendDisallowed(access);
  return false;
}

rc_rpc_plugin::TypeAccess RCCommandRequest::CheckModule(
    const std::string& module_type,
    application_manager::ApplicationSharedPtr app) {
  return policy_handler_.CheckModule(app->policy_app_id(), module_type)
             ? rc_rpc_plugin::TypeAccess::kAllowed
             : rc_rpc_plugin::TypeAccess::kDisallowed;
}

bool RCCommandRequest::IsModuleIdProvided(
    const smart_objects::SmartObject& hmi_response) const {
  SDL_LOG_AUTO_TRACE();
  return hmi_response[app_mngr::strings::msg_params]
                     [message_params::kModuleData]
                         .keyExists(message_params::kModuleId);
}

void RCCommandRequest::SendDisallowed(rc_rpc_plugin::TypeAccess access) {
  SDL_LOG_AUTO_TRACE();
  std::string info;
  if (rc_rpc_plugin::kDisallowed == access) {
    info = disallowed_info_.empty()
               ? "The RPC is disallowed by vehicle settings"
               : disallowed_info_;
  } else {
    return;
  }
  SDL_LOG_ERROR(info);
  SendResponse(false, mobile_apis::Result::DISALLOWED, info.c_str());
}

void RCCommandRequest::Run() {
  SDL_LOG_AUTO_TRACE();
  app_mngr::ApplicationSharedPtr app =
      application_manager_.application(connection_key());

  if (!IsInterfaceAvailable(app_mngr::HmiInterfaces::HMI_INTERFACE_RC)) {
    SDL_LOG_WARN("HMI interface RC is not available");
    SendResponse(false,
                 mobile_apis::Result::UNSUPPORTED_RESOURCE,
                 "RC is not supported by system");
    return;
  }
  SDL_LOG_TRACE("RC interface is available!");
  if (!policy_handler_.CheckHMIType(
          app->policy_app_id(),
          mobile_apis::AppHMIType::eType::REMOTE_CONTROL,
          app->app_types())) {
    SDL_LOG_WARN("Application has no remote control functions");
    SendResponse(false, mobile_apis::Result::DISALLOWED, "");
    return;
  }
  if (!resource_allocation_manager_.is_rc_enabled()) {
    SDL_LOG_WARN("Remote control is disabled by user");
    SetResourceState(ModuleType(), ResourceState::FREE);
    SendResponse(false,
                 mobile_apis::Result::USER_DISALLOWED,
                 "Remote control is disabled by user");
    return;
  }
  auto rc_capability = hmi_capabilities_.rc_capability();
  if (!rc_capability || rc_capability->empty()) {
    SDL_LOG_WARN("Accessing not supported module: " << ModuleType());
    SetResourceState(ModuleType(), ResourceState::FREE);
    SendResponse(false,
                 mobile_apis::Result::UNSUPPORTED_RESOURCE,
                 "Accessing not supported module");
    return;
  }

  if (CheckDriverConsent()) {
    if (AcquireResources()) {
      Execute();  // run child's logic
    }
    // If resource is not aqcuired, AcquireResources method will either
    // send response to mobile or
    // send additional request to HMI to ask driver consent
  }
}

bool RCCommandRequest::AcquireResources() {
  SDL_LOG_AUTO_TRACE();
  const std::string module_type = ModuleType();
  const std::string module_id = ModuleId();

  if (!IsResourceFree(module_type, module_id)) {
    SDL_LOG_WARN("Resource is busy.");
    SendResponse(false, mobile_apis::Result::IN_USE, "");
    return false;
  }

  AcquireResult::eType acquire_result = AcquireResource(message_);
  switch (acquire_result) {
    case AcquireResult::ALLOWED: {
      SetResourceState(module_type, ResourceState::BUSY);
      return true;
    }
    case AcquireResult::IN_USE: {
      SendResponse(false, mobile_apis::Result::IN_USE, "");
      return false;
    }
    case AcquireResult::ASK_DRIVER: {
      ProcessAskDriverMode(module_type, module_id);
      return false;
    }
    case AcquireResult::REJECTED: {
      SendResponse(false, mobile_apis::Result::REJECTED, "");
      return false;
    }
  }
  return false;
}

void RCCommandRequest::on_event(const app_mngr::event_engine::Event& event) {
  SDL_LOG_AUTO_TRACE();

  if (event.id() == hmi_apis::FunctionID::RC_GetInteriorVehicleDataConsent) {
    ProcessAccessResponse(event);
  } else {
    const std::string module_type = ModuleType();
    SetResourceState(module_type, ResourceState::FREE);
  }
}

void RCCommandRequest::ProcessAccessResponse(
    const app_mngr::event_engine::Event& event) {
  SDL_LOG_AUTO_TRACE();

  auto app = application_manager_.application(connection_key());
  const std::string module_type = ModuleType();
  const std::string module_id = ModuleId();

  if (!app) {
    SDL_LOG_ERROR("NULL pointer.");
    SendResponse(false, mobile_apis::Result::APPLICATION_NOT_REGISTERED, "");
    return;
  }

  const auto& message = event.smart_object();
  const auto result_code =
      GetMobileResultCode(static_cast<hmi_apis::Common_Result::eType>(
          message[app_mngr::strings::params][app_mngr::hmi_response::code]
              .asUInt()));

  const bool result =
      helpers::Compare<mobile_apis::Result::eType, helpers::EQ, helpers::ONE>(
          result_code,
          mobile_apis::Result::SUCCESS,
          mobile_apis::Result::WARNINGS);

  if (result) {
    bool is_allowed = false;
    if (message[app_mngr::strings::msg_params].keyExists(
            message_params::kAllowed)) {
      is_allowed =
          message[app_mngr::strings::msg_params][message_params::kAllowed][0]
              .asBool();
    }
    std::string policy_app_id = app->policy_app_id();
    const auto mac_address = app->mac_address();
    std::vector<std::string> module_ids{module_id};
    std::vector<bool> module_allowed{is_allowed};
    auto module_consents =
        RCHelpers::FillModuleConsents(module_type, module_ids, module_allowed);
    rc_consent_manager_.SaveModuleConsents(
        policy_app_id, mac_address, module_consents);
    ProcessConsentResult(is_allowed, module_type, module_id, app->app_id());
  } else {
    std::string response_info;
    GetInfo(message, response_info);
    SendResponse(false, result_code, response_info.c_str());
  }
}

void RCCommandRequest::ProcessConsentResult(const bool is_allowed,
                                            const std::string& module_type,
                                            const std::string& module_id,
                                            const uint32_t app_id) {
  SDL_LOG_AUTO_TRACE();
  if (is_allowed) {
    SetResourceState(module_type, ResourceState::BUSY);
    const auto default_timeout =
        application_manager_.get_settings().default_timeout() +
        application_manager_.get_settings().default_timeout_compensation();
    application_manager_.UpdateRequestTimeout(
        connection_key(), correlation_id(), default_timeout);
    Execute();  // run child's logic
  } else {
    resource_allocation_manager_.OnDriverDisallowed(
        module_type, module_id, app_id);

    std::stringstream ss;
    ss << "The resource [" << module_type << ":" << module_id
       << "] is in use and the driver disallows this remote "
          "control RPC";
    SendResponse(false, mobile_apis::Result::REJECTED, ss.str().c_str());
  }
}

void RCCommandRequest::ProcessAskDriverMode(const std::string& module_type,
                                            const std::string& module_id) {
  SDL_LOG_AUTO_TRACE();
  auto app =
      application_manager_.application(CommandRequestImpl::connection_key());
  const std::string policy_app_id = app->policy_app_id();
  const std::string mac_address = app->mac_address();

  auto consent = rc_consent_manager_.GetModuleConsent(
      policy_app_id, mac_address, {module_type, module_id});
  switch (consent) {
    case rc_rpc_types::ModuleConsent::NOT_EXISTS: {
      smart_objects::SmartObject module_ids(
          smart_objects::SmartType::SmartType_Array);
      module_ids[0] = module_id;
      SendGetUserConsent(module_type, module_ids);
      break;
    }
    case rc_rpc_types::ModuleConsent::NOT_CONSENTED:
    case rc_rpc_types::ModuleConsent::CONSENTED: {
      const bool is_allowed = rc_rpc_types::ModuleConsent::CONSENTED == consent;
      ProcessConsentResult(is_allowed, module_type, module_id, app->app_id());
      break;
    }
  };
}

void RCCommandRequest::SendGetUserConsent(
    const std::string& module_type,
    const smart_objects::SmartObject& module_ids) {
  SDL_LOG_AUTO_TRACE();
  app_mngr::ApplicationSharedPtr app =
      application_manager_.application(connection_key());
  DCHECK(app);
  smart_objects::SmartObject msg_params =
      smart_objects::SmartObject(smart_objects::SmartType_Map);
  msg_params[app_mngr::strings::app_id] = app->app_id();
  msg_params[message_params::kModuleType] = module_type;
  msg_params[message_params::kModuleIds] = module_ids;

  SendHMIRequest(hmi_apis::FunctionID::RC_GetInteriorVehicleDataConsent,
                 &msg_params,
                 true);
}
}  // namespace commands
}  // namespace rc_rpc_plugin
