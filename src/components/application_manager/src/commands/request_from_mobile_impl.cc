/*
 Copyright (c) 2020, Ford Motor Company
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

#include "application_manager/commands/request_from_mobile_impl.h"

#include <algorithm>
#include <numeric>
#include <string>

#include "application_manager/app_service_manager.h"
#include "application_manager/application_manager.h"
#include "application_manager/message_helper.h"
#include "application_manager/rpc_service.h"
#include "smart_objects/smart_object.h"
#include "utils/helpers.h"
#include "utils/macro.h"

namespace application_manager {

namespace commands {

SDL_CREATE_LOG_VARIABLE("Commands")

namespace smart_objects = ns_smart_device_link::ns_smart_objects;

std::string MergeInfos(const ResponseInfo& first_info,
                       const std::string& first_str,
                       const ResponseInfo& second_info,
                       const std::string& second_str) {
  if ((first_info.interface_state == HmiInterfaces::STATE_NOT_AVAILABLE) &&
      (second_info.interface_state != HmiInterfaces::STATE_NOT_AVAILABLE) &&
      !second_str.empty()) {
    return second_str;
  }

  if ((second_info.interface_state == HmiInterfaces::STATE_NOT_AVAILABLE) &&
      (first_info.interface_state != HmiInterfaces::STATE_NOT_AVAILABLE) &&
      !first_str.empty()) {
    return first_str;
  }

  return MergeInfos(first_str, second_str);
}

std::string MergeInfos(const std::string& first, const std::string& second) {
  return first + ((!first.empty() && !second.empty()) ? ", " : "") + second;
}

std::string MergeInfos(const std::string& first,
                       const std::string& second,
                       const std::string& third) {
  std::string result = MergeInfos(first, second);
  return MergeInfos(result, third);
}

const std::string CreateInfoForUnsupportedResult(
    HmiInterfaces::InterfaceID interface) {
  switch (interface) {
    case (HmiInterfaces::InterfaceID::HMI_INTERFACE_VR): {
      return "VR is not supported by system";
    }
    case (HmiInterfaces::InterfaceID::HMI_INTERFACE_TTS): {
      return "TTS is not supported by system";
    }
    case (HmiInterfaces::InterfaceID::HMI_INTERFACE_UI): {
      return "UI is not supported by system";
    }
    case (HmiInterfaces::InterfaceID::HMI_INTERFACE_Navigation): {
      return "Navigation is not supported by system";
    }
    case (HmiInterfaces::InterfaceID::HMI_INTERFACE_VehicleInfo): {
      return "VehicleInfo is not supported by system";
    }
    case (HmiInterfaces::InterfaceID::HMI_INTERFACE_RC): {
      return "Remote control is not supported by system";
    }
    default:
      SDL_LOG_WARN(
          "Could not create info because"
          " interface isn't valid. Interface is:"
          << static_cast<int32_t>(interface));
      return "";
  }
}

bool RequestFromMobileImpl::CheckResultCode(const ResponseInfo& first,
                                            const ResponseInfo& second) const {
  if (first.is_ok && second.is_unsupported_resource) {
    return true;
  }
  if (first.is_ok && second.is_not_used) {
    return true;
  }
  if (first.is_ok && second.is_ok) {
    return true;
  }
  return false;
}

bool IsResultCodeWarning(const ResponseInfo& first,
                         const ResponseInfo& second) {
  const bool first_is_ok_second_is_warn =
      (first.is_ok || first.is_not_used) &&
      hmi_apis::Common_Result::WARNINGS == second.result_code;

  const bool both_warnings =
      hmi_apis::Common_Result::WARNINGS == first.result_code &&
      hmi_apis::Common_Result::WARNINGS == second.result_code;

  return first_is_ok_second_is_warn || both_warnings;
}

ResponseInfo::ResponseInfo()
    : result_code(hmi_apis::Common_Result::INVALID_ENUM)
    , interface(HmiInterfaces::HMI_INTERFACE_INVALID_ENUM)
    , interface_state(HmiInterfaces::STATE_NOT_RESPONSE)
    , is_ok(false)
    , is_unsupported_resource(false)
    , is_not_used(false) {}

ResponseInfo::ResponseInfo(const hmi_apis::Common_Result::eType result,
                           const HmiInterfaces::InterfaceID hmi_interface,
                           ApplicationManager& application_manager)
    : result_code(result)
    , interface(hmi_interface)
    , interface_state(HmiInterfaces::STATE_NOT_RESPONSE)
    , is_ok(false)
    , is_unsupported_resource(false)
    , is_not_used(false) {
  using namespace helpers;

  interface_state =
      application_manager.hmi_interfaces().GetInterfaceState(hmi_interface);

  is_ok = IsHMIResultSuccess(result_code);

  is_not_used = hmi_apis::Common_Result::INVALID_ENUM == result_code;

  is_unsupported_resource =
      hmi_apis::Common_Result::UNSUPPORTED_RESOURCE == result_code;
}

RequestFromMobileImpl::RequestFromMobileImpl(
    const MessageSharedPtr& message,
    ApplicationManager& application_manager,
    rpc_service::RPCService& rpc_service,
    HMICapabilities& hmi_capabilities,
    policy::PolicyHandlerInterface& policy_handler)
    : CommandRequestImpl(message,
                         application_manager,
                         rpc_service,
                         hmi_capabilities,
                         policy_handler)
    , hash_update_mode_(kSkipHashUpdate)
    , is_success_result_(false) {}

RequestFromMobileImpl::~RequestFromMobileImpl() {
  UpdateHash();
}

bool RequestFromMobileImpl::Init() {
  return true;
}

bool RequestFromMobileImpl::CheckPermissions() {
  return CheckAllowedParameters(Command::CommandSource::SOURCE_MOBILE);
}

bool RequestFromMobileImpl::CleanUp() {
  unsubscribe_from_all_mobile_events();
  unsubscribe_from_all_hmi_events();  // To prevent on_event calls

  // Cleanup for mobile requests can be done only if OnEvent/OnTimeout events
  // are not processed at that moment
  return kAwaitingResponse == current_state();
}

void RequestFromMobileImpl::Run() {}

void RequestFromMobileImpl::OnTimeOut() {
  SDL_LOG_AUTO_TRACE();

  unsubscribe_from_all_mobile_events();
  unsubscribe_from_all_hmi_events();

  smart_objects::SmartObjectSPtr response =
      MessageHelper::CreateNegativeResponse(connection_key(),
                                            function_id(),
                                            correlation_id(),
                                            mobile_api::Result::GENERIC_ERROR);

  AddTimeOutComponentInfoToMessage(*response);

  rpc_service_.ManageMobileCommand(response, SOURCE_SDL);
}

void RequestFromMobileImpl::on_event(const event_engine::Event& event) {}

void RequestFromMobileImpl::on_event(const event_engine::MobileEvent& event) {}

void RequestFromMobileImpl::SendResponse(
    const bool success,
    const mobile_apis::Result::eType& result_code,
    const char* info,
    const smart_objects::SmartObject* response_params,
    const std::vector<uint8_t> binary_data) {
  SDL_LOG_AUTO_TRACE();

  smart_objects::SmartObjectSPtr result =
      std::make_shared<smart_objects::SmartObject>();

  smart_objects::SmartObject& response = *result;

  response[strings::params][strings::message_type] = MessageType::kResponse;
  response[strings::params][strings::correlation_id] = correlation_id();
  response[strings::params][strings::protocol_type] =
      CommandImpl::mobile_protocol_type_;
  response[strings::params][strings::protocol_version] =
      CommandImpl::protocol_version_;
  response[strings::params][strings::connection_key] = connection_key();
  response[strings::params][strings::function_id] = function_id();
  if (!binary_data.empty()) {
    response[strings::params][strings::binary_data] = binary_data;
  }
  if (response_params) {
    response[strings::msg_params] = *response_params;
  }

  if (info && *info != '\0') {
    response[strings::msg_params][strings::info] = std::string(info);
  }

  // Add disallowed parameters and info from request back to response with
  // appropriate reasons (VehicleData result codes)
  if (result_code != mobile_apis::Result::APPLICATION_NOT_REGISTERED &&
      result_code != mobile_apis::Result::INVALID_DATA) {
    FormatResponse(response);
  }

  response[strings::msg_params][strings::success] = success;
  if ((result_code == mobile_apis::Result::SUCCESS ||
       result_code == mobile_apis::Result::WARNINGS) &&
      !warning_info().empty()) {
    response[strings::msg_params][strings::info] =
        (info && *info != '\0') ? std::string(info) + "\n" + warning_info()
                                : warning_info();
    response[strings::msg_params][strings::result_code] =
        mobile_apis::Result::WARNINGS;
  } else if (mobile_apis::Result::INVALID_ENUM == result_code) {
    response[strings::msg_params][strings::result_code] =
        mobile_apis::Result::GENERIC_ERROR;
    response[strings::msg_params][strings::info] =
        "Invalid result received from vehicle";
  } else {
    response[strings::msg_params][strings::result_code] = result_code;
  }

  is_success_result_ = success;

  rpc_service_.ManageMobileCommand(result, SOURCE_SDL);
}

smart_objects::SmartObject CreateUnsupportedResourceResponse(
    const hmi_apis::FunctionID::eType function_id,
    const uint32_t hmi_correlation_id,
    HmiInterfaces::InterfaceID interface) {
  smart_objects::SmartObject response(smart_objects::SmartType_Map);
  smart_objects::SmartObject& params = response[strings::params];
  params[strings::message_type] = MessageType::kResponse;
  params[strings::correlation_id] = hmi_correlation_id;
  params[strings::protocol_type] = CommandImpl::hmi_protocol_type_;
  params[strings::protocol_version] = CommandImpl::protocol_version_;
  params[strings::function_id] = function_id;
  params[hmi_response::code] = hmi_apis::Common_Result::UNSUPPORTED_RESOURCE;
  smart_objects::SmartObject& msg_params = response[strings::msg_params];
  msg_params[strings::info] = CreateInfoForUnsupportedResult(interface);
  return response;
}

bool RequestFromMobileImpl::ProcessHMIInterfacesAvailability(
    const uint32_t hmi_correlation_id,
    const hmi_apis::FunctionID::eType& function_id) {
  SDL_LOG_AUTO_TRACE();
  HmiInterfaces& hmi_interfaces = application_manager_.hmi_interfaces();
  HmiInterfaces::InterfaceID interface =
      hmi_interfaces.GetInterfaceFromFunction(function_id);
  DCHECK(interface != HmiInterfaces::HMI_INTERFACE_INVALID_ENUM);
  const HmiInterfaces::InterfaceState state =
      hmi_interfaces.GetInterfaceState(interface);
  if (HmiInterfaces::STATE_NOT_AVAILABLE == state) {
    event_engine::Event event(function_id);
    event.set_smart_object(CreateUnsupportedResourceResponse(
        function_id, hmi_correlation_id, interface));
    event.raise(application_manager_.event_dispatcher());
    return false;
  }
  return true;
}

void RequestFromMobileImpl::UpdateHash() {
  SDL_LOG_AUTO_TRACE();
  if (hash_update_mode_ == kSkipHashUpdate) {
    SDL_LOG_DEBUG("Hash update is disabled for " << function_id());
    return;
  }

  if (HmiInterfaces::InterfaceState::STATE_NOT_RESPONSE ==
      application_manager_.hmi_interfaces().GetInterfaceState(
          HmiInterfaces::InterfaceID::HMI_INTERFACE_UI)) {
    SDL_LOG_ERROR("UI interface has not responded. Hash won't be updated.");
    return;
  }

  if (!is_success_result_) {
    SDL_LOG_WARN("Command is not succeeded. Hash won't be updated.");
    return;
  }

  ApplicationSharedPtr application =
      application_manager_.application(connection_key());
  if (!application) {
    SDL_LOG_ERROR("Application with connection key "
                  << connection_key()
                  << " not found. Not able to update hash.");
    return;
  }

  SDL_LOG_DEBUG(
      "Updating hash for application with connection key "
      << connection_key() << " while processing function id "
      << MessageHelper::StringifiedFunctionID(
             static_cast<mobile_api::FunctionID::eType>(function_id())));

  application->UpdateHash();
}

uint32_t RequestFromMobileImpl::SendHMIRequest(
    const hmi_apis::FunctionID::eType& function_id,
    const smart_objects::SmartObject* msg_params,
    bool use_events) {
  smart_objects::SmartObjectSPtr result =
      std::make_shared<smart_objects::SmartObject>();

  const uint32_t hmi_correlation_id =
      application_manager_.GetNextHMICorrelationID();

  smart_objects::SmartObject& request = *result;
  request[strings::params][strings::message_type] = MessageType::kRequest;
  request[strings::params][strings::function_id] = function_id;
  request[strings::params][strings::correlation_id] = hmi_correlation_id;
  request[strings::params][strings::protocol_version] =
      CommandImpl::protocol_version_;
  request[strings::params][strings::protocol_type] =
      CommandImpl::hmi_protocol_type_;

  if (msg_params) {
    request[strings::msg_params] = *msg_params;
  }

  if (use_events) {
    SDL_LOG_DEBUG("SendHMIRequest subscribe_on_event " << function_id << " "
                                                       << hmi_correlation_id);
    subscribe_on_event(function_id, hmi_correlation_id);
  }
  if (ProcessHMIInterfacesAvailability(hmi_correlation_id, function_id)) {
    if (rpc_service_.ManageHMICommand(result, SOURCE_SDL_TO_HMI)) {
      AddRequestToTimeoutHandler(request);
    } else {
      SDL_LOG_ERROR("Unable to send request");
      SendResponse(false, mobile_apis::Result::OUT_OF_MEMORY);
    }
  } else {
    SDL_LOG_DEBUG("Interface is not available");
  }
  return hmi_correlation_id;
}

void RequestFromMobileImpl::CreateHMINotification(
    const hmi_apis::FunctionID::eType& function_id,
    const ns_smart_device_link::ns_smart_objects::SmartObject& msg_params)
    const {
  smart_objects::SmartObjectSPtr result =
      std::make_shared<smart_objects::SmartObject>();
  if (!result) {
    SDL_LOG_ERROR("Memory allocation failed.");
    return;
  }
  smart_objects::SmartObject& notify = *result;

  notify[strings::params][strings::message_type] =
      static_cast<int32_t>(application_manager::MessageType::kNotification);
  notify[strings::params][strings::protocol_version] =
      CommandImpl::protocol_version_;
  notify[strings::params][strings::protocol_type] =
      CommandImpl::hmi_protocol_type_;
  notify[strings::params][strings::function_id] = function_id;
  notify[strings::msg_params] = msg_params;

  if (!rpc_service_.ManageHMICommand(result, SOURCE_SDL_TO_HMI)) {
    SDL_LOG_ERROR("Unable to send HMI notification");
  }
}

mobile_apis::Result::eType RequestFromMobileImpl::GetMobileResultCode(
    const hmi_apis::Common_Result::eType& hmi_code) const {
  mobile_apis::Result::eType mobile_result = mobile_apis::Result::GENERIC_ERROR;
  switch (hmi_code) {
    case hmi_apis::Common_Result::SUCCESS: {
      mobile_result = mobile_apis::Result::SUCCESS;
      break;
    }
    case hmi_apis::Common_Result::UNSUPPORTED_REQUEST: {
      mobile_result = mobile_apis::Result::UNSUPPORTED_REQUEST;
      break;
    }
    case hmi_apis::Common_Result::UNSUPPORTED_RESOURCE: {
      mobile_result = mobile_apis::Result::UNSUPPORTED_RESOURCE;
      break;
    }
    case hmi_apis::Common_Result::DISALLOWED: {
      mobile_result = mobile_apis::Result::DISALLOWED;
      break;
    }
    case hmi_apis::Common_Result::REJECTED: {
      mobile_result = mobile_apis::Result::REJECTED;
      break;
    }
    case hmi_apis::Common_Result::ABORTED: {
      mobile_result = mobile_apis::Result::ABORTED;
      break;
    }
    case hmi_apis::Common_Result::IGNORED: {
      mobile_result = mobile_apis::Result::IGNORED;
      break;
    }
    case hmi_apis::Common_Result::RETRY: {
      mobile_result = mobile_apis::Result::RETRY;
      break;
    }
    case hmi_apis::Common_Result::IN_USE: {
      mobile_result = mobile_apis::Result::IN_USE;
      break;
    }
    case hmi_apis::Common_Result::DATA_NOT_AVAILABLE: {
      mobile_result = mobile_apis::Result::VEHICLE_DATA_NOT_AVAILABLE;
      break;
    }
    case hmi_apis::Common_Result::TIMED_OUT: {
      mobile_result = mobile_apis::Result::TIMED_OUT;
      break;
    }
    case hmi_apis::Common_Result::INVALID_DATA: {
      mobile_result = mobile_apis::Result::INVALID_DATA;
      break;
    }
    case hmi_apis::Common_Result::CHAR_LIMIT_EXCEEDED: {
      mobile_result = mobile_apis::Result::CHAR_LIMIT_EXCEEDED;
      break;
    }
    case hmi_apis::Common_Result::INVALID_ID: {
      mobile_result = mobile_apis::Result::INVALID_ID;
      break;
    }
    case hmi_apis::Common_Result::DUPLICATE_NAME: {
      mobile_result = mobile_apis::Result::DUPLICATE_NAME;
      break;
    }
    case hmi_apis::Common_Result::APPLICATION_NOT_REGISTERED: {
      mobile_result = mobile_apis::Result::APPLICATION_NOT_REGISTERED;
      break;
    }
    case hmi_apis::Common_Result::WRONG_LANGUAGE: {
      mobile_result = mobile_apis::Result::WRONG_LANGUAGE;
      break;
    }
    case hmi_apis::Common_Result::OUT_OF_MEMORY: {
      mobile_result = mobile_apis::Result::OUT_OF_MEMORY;
      break;
    }
    case hmi_apis::Common_Result::TOO_MANY_PENDING_REQUESTS: {
      mobile_result = mobile_apis::Result::TOO_MANY_PENDING_REQUESTS;
      break;
    }
    case hmi_apis::Common_Result::NO_APPS_REGISTERED: {
      mobile_result = mobile_apis::Result::APPLICATION_NOT_REGISTERED;
      break;
    }
    case hmi_apis::Common_Result::NO_DEVICES_CONNECTED: {
      mobile_result = mobile_apis::Result::APPLICATION_NOT_REGISTERED;
      break;
    }
    case hmi_apis::Common_Result::WARNINGS: {
      mobile_result = mobile_apis::Result::WARNINGS;
      break;
    }
    case hmi_apis::Common_Result::GENERIC_ERROR: {
      mobile_result = mobile_apis::Result::GENERIC_ERROR;
      break;
    }
    case hmi_apis::Common_Result::USER_DISALLOWED: {
      mobile_result = mobile_apis::Result::USER_DISALLOWED;
      break;
    }
    case hmi_apis::Common_Result::SAVED: {
      mobile_result = mobile_apis::Result::SAVED;
      break;
    }
    case hmi_apis::Common_Result::READ_ONLY: {
      mobile_result = mobile_apis::Result::READ_ONLY;
      break;
    }
    default: {
      SDL_LOG_ERROR("Unknown HMI result code " << hmi_code);
      break;
    }
  }

  return mobile_result;
}

bool RequestFromMobileImpl::CheckHMICapabilities(
    const mobile_apis::ButtonName::eType button) const {
  SDL_LOG_AUTO_TRACE();

  using namespace smart_objects;
  using namespace mobile_apis;

  if (!hmi_capabilities_.is_ui_cooperating()) {
    SDL_LOG_ERROR("UI is not supported by HMI");
    return false;
  }

  const auto button_capabilities_so = hmi_capabilities_.button_capabilities();
  if (!button_capabilities_so) {
    SDL_LOG_ERROR("Invalid button capabilities object");
    return false;
  }

  const SmartObject& button_capabilities = *button_capabilities_so;
  for (size_t i = 0; i < button_capabilities.length(); ++i) {
    const SmartObject& capabilities = button_capabilities[i];
    const ButtonName::eType current_button = static_cast<ButtonName::eType>(
        capabilities.getElement(hmi_response::button_name).asInt());
    if (current_button == button) {
      SDL_LOG_DEBUG("Button capabilities for " << button << " was found");
      return true;
    }
  }

  SDL_LOG_DEBUG("Button capabilities for " << button << " was not found");
  return false;
}

void RequestFromMobileImpl::RemoveDisallowedParameters() {
  SDL_LOG_AUTO_TRACE();

  smart_objects::SmartObject& params = (*message_)[strings::msg_params];

  // Remove from request all disallowed parameters
  RPCParams::const_iterator it_disallowed =
      parameters_permissions_.disallowed_params.begin();
  RPCParams::const_iterator it_disallowed_end =
      parameters_permissions_.disallowed_params.end();
  for (; it_disallowed != it_disallowed_end; ++it_disallowed) {
    if (params.keyExists(*it_disallowed)) {
      const std::string key = *it_disallowed;
      params.erase(key);
      removed_parameters_permissions_.disallowed_params.insert(key);
      SDL_LOG_INFO("Following parameter is disallowed by user: " << key);
    }
  }

  // Remove from request all undefined yet parameters
  RPCParams::const_iterator it_undefined =
      parameters_permissions_.undefined_params.begin();
  RPCParams::const_iterator it_undefined_end =
      parameters_permissions_.undefined_params.end();
  for (; it_undefined != it_undefined_end; ++it_undefined) {
    if (params.keyExists(*it_undefined)) {
      const std::string key = *it_undefined;
      params.erase(key);
      removed_parameters_permissions_.undefined_params.insert(key);
      SDL_LOG_INFO("Following parameter is disallowed by policy: " << key);
    }
  }

  // Remove from request all parameters missed in allowed
  const VehicleData& vehicle_data =
      application_manager::MessageHelper::vehicle_data();

  VehicleData::const_iterator it_vehicle_data = vehicle_data.begin();
  VehicleData::const_iterator it_vehicle_data_end = vehicle_data.end();
  for (; it_vehicle_data != it_vehicle_data_end; ++it_vehicle_data) {
    const std::string key = it_vehicle_data->first;
    if (params.keyExists(key) &&
        parameters_permissions_.allowed_params.end() ==
            std::find(parameters_permissions_.allowed_params.begin(),
                      parameters_permissions_.allowed_params.end(),
                      key)) {
      params.erase(key);
      removed_parameters_permissions_.undefined_params.insert(key);
      SDL_LOG_INFO("Following parameter is not found among allowed parameters '"
                   << key << "' and will be treated as disallowed.");
    }
  }
}

bool RequestFromMobileImpl::HasDisallowedParams() const {
  return ((!removed_parameters_permissions_.disallowed_params.empty()) ||
          (!removed_parameters_permissions_.undefined_params.empty()));
}

bool RequestFromMobileImpl::PrepareResultForMobileResponse(
    hmi_apis::Common_Result::eType result_code,
    HmiInterfaces::InterfaceID interface) const {
  SDL_LOG_AUTO_TRACE();

  return IsHMIResultSuccess(result_code, interface);
}

bool RequestFromMobileImpl::PrepareResultForMobileResponse(
    ResponseInfo& out_first, ResponseInfo& out_second) const {
  SDL_LOG_AUTO_TRACE();
  bool result = CheckResultCode(out_first, out_second) ||
                CheckResultCode(out_second, out_first);
  return result;
}

bool RequestFromMobileImpl::PrepareResultForMobileResponse(
    ResponseInfo& out_first,
    ResponseInfo& out_second,
    ResponseInfo& out_third) const {
  SDL_LOG_AUTO_TRACE();
  bool result = (PrepareResultForMobileResponse(out_first, out_second) ||
                 PrepareResultForMobileResponse(out_second, out_third)) &&
                PrepareResultForMobileResponse(out_first, out_third);
  return result;
}

void RequestFromMobileImpl::GetInfo(
    const smart_objects::SmartObject& response_from_hmi,
    std::string& out_info) const {
  if (response_from_hmi[strings::msg_params].keyExists(strings::info)) {
    if (!response_from_hmi[strings::msg_params][strings::info].empty()) {
      out_info =
          response_from_hmi[strings::msg_params][strings::info].asString();
    }
  }
}

mobile_apis::Result::eType RequestFromMobileImpl::PrepareResultCodeForResponse(
    const ResponseInfo& first, const ResponseInfo& second) {
  SDL_LOG_AUTO_TRACE();
  if (IsResultCodeUnsupported(first, second) ||
      IsResultCodeUnsupported(second, first)) {
    return mobile_apis::Result::UNSUPPORTED_RESOURCE;
  }
  if (IsResultCodeWarning(first, second) ||
      IsResultCodeWarning(second, first)) {
    return mobile_apis::Result::WARNINGS;
  }
  // If response contains erroneous result code SDL need return erroneus
  // result code.
  hmi_apis::Common_Result::eType first_result =
      hmi_apis::Common_Result::INVALID_ENUM;
  hmi_apis::Common_Result::eType second_result =
      hmi_apis::Common_Result::INVALID_ENUM;
  if (!first.is_unsupported_resource) {
    first_result = first.result_code;
  }
  if (!second.is_unsupported_resource) {
    second_result = second.result_code;
  }
  mobile_apis::Result::eType result_code =
      MessageHelper::HMIToMobileResult(std::max(first_result, second_result));
  return result_code;
}

mobile_apis::Result::eType RequestFromMobileImpl::PrepareResultCodeForResponse(
    const ResponseInfo& first,
    const ResponseInfo& second,
    const ResponseInfo& third) {
  SDL_LOG_AUTO_TRACE();

  const auto first_comparison = PrepareResultCodeForResponse(first, second);
  const auto second_comparison = PrepareResultCodeForResponse(second, third);
  const auto third_comparison = PrepareResultCodeForResponse(first, third);

  return std::max({first_comparison, second_comparison, third_comparison});
}

const CommandParametersPermissions&
RequestFromMobileImpl::parameters_permissions() const {
  return parameters_permissions_;
}

void RequestFromMobileImpl::SendProviderRequest(
    const mobile_apis::FunctionID::eType& mobile_function_id,
    const hmi_apis::FunctionID::eType& hmi_function_id,
    const smart_objects::SmartObject* msg,
    bool use_events) {
  SDL_LOG_AUTO_TRACE();
  bool hmi_destination = false;
  ApplicationSharedPtr app;
  // Default error code and error message
  std::string error_msg = "No app service provider available";
  mobile_apis::Result::eType error_code =
      mobile_apis::Result::DATA_NOT_AVAILABLE;

  if ((*msg)[strings::msg_params].keyExists(strings::service_type)) {
    std::string service_type =
        (*msg)[strings::msg_params][strings::service_type].asString();
    application_manager_.GetAppServiceManager().GetProviderByType(
        service_type, true, app, hmi_destination);
    error_msg = "No app service provider with serviceType: " + service_type +
                " is available";
    error_code = mobile_apis::Result::DATA_NOT_AVAILABLE;
  } else if ((*msg)[strings::msg_params].keyExists(strings::service_id)) {
    std::string service_id =
        (*msg)[strings::msg_params][strings::service_id].asString();
    application_manager_.GetAppServiceManager().GetProviderByID(
        service_id, true, app, hmi_destination);
    error_msg = "No app service provider with serviceId: " + service_id +
                " is available";
    error_code = mobile_apis::Result::INVALID_ID;
  }

  if (hmi_destination) {
    SDL_LOG_DEBUG("Sending Request to HMI Provider");
    application_manager_.IncreaseForwardedRequestTimeout(connection_key(),
                                                         correlation_id());
    SendHMIRequest(hmi_function_id, &(*msg)[strings::msg_params], use_events);
    return;
  }

  if (!app) {
    SDL_LOG_DEBUG("Invalid App Provider pointer");
    SendResponse(false, error_code, error_msg.c_str());
    return;
  }

  if (connection_key() == app->app_id()) {
    SendResponse(false,
                 mobile_apis::Result::IGNORED,
                 "Consumer app is same as producer app");
    return;
  }

  smart_objects::SmartObjectSPtr new_msg =
      std::make_shared<smart_objects::SmartObject>();
  smart_objects::SmartObject& request = *new_msg;

  request[strings::params] = (*msg)[strings::params];
  request[strings::msg_params] = (*msg)[strings::msg_params];
  request[strings::params][strings::connection_key] = app->app_id();

  application_manager_.IncreaseForwardedRequestTimeout(connection_key(),
                                                       correlation_id());
  SendMobileRequest(mobile_function_id, new_msg, use_events);
}

bool RequestFromMobileImpl::IsResultCodeUnsupported(
    const ResponseInfo& first, const ResponseInfo& second) const {
  const bool first_ok_second_unsupported =
      (first.is_ok || first.is_not_used) && second.is_unsupported_resource;
  const bool both_unsupported =
      first.is_unsupported_resource && second.is_unsupported_resource;
  return first_ok_second_unsupported || both_unsupported;
}

std::string GetComponentNameFromInterface(
    const HmiInterfaces::InterfaceID& interface) {
  switch (interface) {
    case HmiInterfaces::HMI_INTERFACE_Buttons:
      return hmi_interface::buttons;
    case HmiInterfaces::HMI_INTERFACE_BasicCommunication:
      return hmi_interface::basic_communication;
    case HmiInterfaces::HMI_INTERFACE_VR:
      return hmi_interface::vr;
    case HmiInterfaces::HMI_INTERFACE_TTS:
      return hmi_interface::tts;
    case HmiInterfaces::HMI_INTERFACE_UI:
      return hmi_interface::ui;
    case HmiInterfaces::HMI_INTERFACE_Navigation:
      return hmi_interface::navigation;
    case HmiInterfaces::HMI_INTERFACE_VehicleInfo:
      return hmi_interface::vehicle_info;
    case HmiInterfaces::HMI_INTERFACE_SDL:
      return hmi_interface::sdl;
    case HmiInterfaces::HMI_INTERFACE_RC:
      return hmi_interface::rc;
    case HmiInterfaces::HMI_INTERFACE_AppService:
      return hmi_interface::app_service;
    default:
      return "Unknown type";
  }
}

const std::string InfoInterfaceSeparator(
    const std::string& sum, const HmiInterfaces::InterfaceID container_value) {
  return sum.empty()
             ? GetComponentNameFromInterface(container_value)
             : sum + ", " + GetComponentNameFromInterface(container_value);
}

void RequestFromMobileImpl::AddTimeOutComponentInfoToMessage(
    smart_objects::SmartObject& response) const {
  using ns_smart_device_link::ns_smart_objects::SmartObject;
  SDL_LOG_AUTO_TRACE();
  sync_primitives::AutoLock lock(awaiting_response_interfaces_lock_);
  if (awaiting_response_interfaces_.empty()) {
    SDL_LOG_ERROR("No interfaces awaiting, info param is empty");
    return;
  }

  const std::string not_responding_interfaces_string =
      std::accumulate(awaiting_response_interfaces_.begin(),
                      awaiting_response_interfaces_.end(),
                      std::string(""),
                      InfoInterfaceSeparator);
  SDL_LOG_DEBUG(
      "Not responding interfaces string: " << not_responding_interfaces_string);
  if (!not_responding_interfaces_string.empty()) {
    const std::string component_info =
        not_responding_interfaces_string + " component does not respond";
    response[strings::msg_params][strings::info] = component_info;
  }
}

void RequestFromMobileImpl::AddRequestToTimeoutHandler(
    const smart_objects::SmartObject& request_to_hmi) const {
  auto function_id = static_cast<hmi_apis::FunctionID::eType>(
      request_to_hmi[strings::params][strings::function_id].asUInt());
  // SDL must not apply "default timeout for RPCs processing" for
  // BasicCommunication.DialNumber RPC (that is, SDL must always wait for HMI
  // response to BC.DialNumber as long as it takes and not return GENERIC_ERROR
  // to mobile app), so the OnResetTimeout logic is not applicable for
  // DialNumber RPC
  if (helpers::Compare<hmi_apis::FunctionID::eType, helpers::EQ, helpers::ONE>(
          function_id,
          hmi_apis::FunctionID::BasicCommunication_DialNumber,
          hmi_apis::FunctionID::INVALID_ENUM)) {
    SDL_LOG_DEBUG(
        "Current RPC is DialNumber or Invalid, OnResetTimeout "
        "logic is not applicable in this case");
    return;
  }

  // If soft buttons are present in Alert or SubtleAlert RPC, SDL will not use
  // timeout tracking for response, so the OnResetTimeout logic is not
  // applicable in this case
  if (helpers::Compare<hmi_apis::FunctionID::eType, helpers::EQ, helpers::ONE>(
          function_id,
          hmi_apis::FunctionID::UI_Alert,
          hmi_apis::FunctionID::UI_SubtleAlert)) {
    if (request_to_hmi.keyExists(strings::msg_params)) {
      if (request_to_hmi[strings::msg_params].keyExists(
              strings::soft_buttons)) {
        SDL_LOG_DEBUG("Soft buttons are present in "
                      << EnumToString(function_id)
                      << " RPC, OnResetTimeout "
                         "logic is not applicable in this case");
        return;
      }
    }
  }

  const application_manager::request_controller::Request request{
      correlation_id(), connection_key(), static_cast<uint32_t>(function_id)};
  application_manager_.get_request_timeout_handler().AddRequest(
      request_to_hmi[strings::params][strings::correlation_id].asUInt(),
      request);
}

}  // namespace commands

}  // namespace application_manager
