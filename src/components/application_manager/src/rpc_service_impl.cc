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

#include "application_manager/rpc_service_impl.h"
#include "application_manager/rpc_protection_manager_impl.h"

#include "application_manager/app_service_manager.h"
#include "application_manager/command_factory.h"
#include "application_manager/commands/command.h"
#include "application_manager/commands/command_impl.h"
#include "application_manager/plugin_manager/plugin_keys.h"

namespace application_manager {
namespace rpc_service {

SDL_CREATE_LOG_VARIABLE("RPCServiceImpl")
namespace formatters = ns_smart_device_link::ns_json_handler::formatters;
namespace jhs = ns_smart_device_link::ns_json_handler::strings;
namespace plugin_names = application_manager::plugin_manager::plugin_names;

RPCServiceImpl::RPCServiceImpl(
    ApplicationManager& app_manager,
    request_controller::RequestController& request_ctrl,
    protocol_handler::ProtocolHandler* protocol_handler,
    hmi_message_handler::HMIMessageHandler* hmi_handler,
    CommandHolder& commands_holder,
    RPCProtectionManagerSPtr rpc_protection_manager,
    hmi_apis::HMI_API& hmi_so_factory,
    mobile_apis::MOBILE_API& mobile_so_factory)
    : app_manager_(app_manager)
    , request_ctrl_(request_ctrl)
    , protocol_handler_(protocol_handler)
    , hmi_handler_(hmi_handler)
    , rpc_protection_manager_(rpc_protection_manager)
    , commands_holder_(commands_holder)
    , messages_to_mobile_("AM ToMobile", this)
    , messages_to_hmi_("AM ToHMI", this)
    , hmi_so_factory_(hmi_so_factory)
    , mobile_so_factory_(mobile_so_factory) {}

RPCServiceImpl::~RPCServiceImpl() {}

void RPCServiceImpl::Stop() {
  SDL_LOG_AUTO_TRACE();

  messages_to_mobile_.Shutdown();
  messages_to_hmi_.Shutdown();
}

EncryptionFlagCheckResult RPCServiceImpl::IsEncryptionRequired(
    const smart_objects::SmartObject& message,
    std::shared_ptr<Application> app,
    const bool is_rpc_service_secure) const {
  SDL_LOG_AUTO_TRACE();
  const auto function_id =
      message[strings::params][strings::function_id].asUInt();
  const auto correlation_id =
      message[strings::params][strings::correlation_id].asUInt();

  const auto message_type =
      message[strings::params][strings::message_type].asUInt();
  const bool policy_encryption_flag =
      rpc_protection_manager_->CheckPolicyEncryptionFlag(
          function_id, app, is_rpc_service_secure);
  if (MessageType::kRequest == message_type) {
    const bool message_protected =
        message[strings::params][strings::protection].asBool();

    if (!message_protected) {
      return policy_encryption_flag
                 ? EncryptionFlagCheckResult::kError_EncryptionNeeded
                 : EncryptionFlagCheckResult::kSuccess_NotProtected;
    }

    //    request is encrypted, so we need to encrypt response regardless of
    //    policy flag
    const auto connection_key =
        message[strings::params][strings::connection_key].asUInt();
    rpc_protection_manager_->AddToEncryptionNeededCache(connection_key,
                                                        correlation_id);
    return EncryptionFlagCheckResult::kSuccess_Protected;
  }
  return policy_encryption_flag
             ? EncryptionFlagCheckResult::kSuccess_Protected
             : EncryptionFlagCheckResult::kSuccess_NotProtected;
}
bool RPCServiceImpl::ManageMobileCommand(
    const commands::MessageSharedPtr message,
    commands::Command::CommandSource source) {
  return ManageMobileCommand(message, source, std::string());
}

bool RPCServiceImpl::ManageMobileCommand(
    const commands::MessageSharedPtr message,
    commands::Command::CommandSource source,
    const std::string warning_info) {
  SDL_LOG_AUTO_TRACE();

  if (!message) {
    SDL_LOG_WARN("Null-pointer message received.");
    return false;
  }

  if (app_manager_.IsLowVoltage()) {
    SDL_LOG_WARN("Low Voltage is active");
    return false;
  }

  MessageHelper::PrintSmartObject(*message);

  const uint32_t connection_key = static_cast<uint32_t>(
      (*message)[strings::params][strings::connection_key].asUInt());
  const WindowID window_id = MessageHelper::ExtractWindowIdFromSmartObject(
      (*message)[strings::msg_params]);
  int32_t message_type =
      (*message)[strings::params][strings::message_type].asInt();

  auto app_ptr = app_manager_.application(connection_key);
  if (app_ptr &&
      (app_manager_.IsAppInReconnectMode(app_ptr->device(),
                                         app_ptr->policy_app_id()) ||
       (mobile_apis::PredefinedWindows::DEFAULT_WINDOW == window_id &&
        (!app_ptr->WindowIdExists(window_id) ||
         mobile_apis::HMILevel::INVALID_ENUM ==
             app_ptr->hmi_level(window_id)) &&
        mobile_apis::messageType::notification == message_type))) {
    commands_holder_.Suspend(
        app_ptr, CommandHolder::CommandType::kMobileCommand, source, message);
    return true;
  }
  mobile_apis::FunctionID::eType function_id =
      static_cast<mobile_apis::FunctionID::eType>(
          (*message)[strings::params][strings::function_id].asInt());

  // Notifications from HMI have no such parameter
  uint32_t correlation_id =
      (*message)[strings::params].keyExists(strings::correlation_id)
          ? (*message)[strings::params][strings::correlation_id].asUInt()
          : 0;

  int32_t protocol_type =
      (*message)[strings::params][strings::protocol_type].asInt();

  ApplicationSharedPtr app;
  if (((mobile_apis::FunctionID::RegisterAppInterfaceID != function_id) &&
       (protocol_type == commands::CommandImpl::mobile_protocol_type_)) &&
      (mobile_apis::FunctionID::UnregisterAppInterfaceID != function_id)) {
    app = app_manager_.application(connection_key);
    if (!app) {
      SDL_LOG_ERROR("RET APPLICATION_NOT_REGISTERED");
      smart_objects::SmartObjectSPtr response =
          MessageHelper::CreateNegativeResponse(
              connection_key,
              static_cast<int32_t>(function_id),
              correlation_id,
              static_cast<int32_t>(
                  mobile_apis::Result::APPLICATION_NOT_REGISTERED));

      SendMessageToMobile(response);
      return false;
    }
#ifdef ENABLE_SECURITY
    if (EncryptionFlagCheckResult::kError_EncryptionNeeded ==
        IsEncryptionRequired(
            *message,
            app,
            protocol_handler_->IsRPCServiceSecure(connection_key))) {
      const auto response = MessageHelper::CreateNegativeResponse(
          connection_key,
          function_id,
          correlation_id,
          static_cast<int32_t>(mobile_apis::Result::ENCRYPTION_NEEDED));
      SendMessageToMobile(response);
      return false;
    }
#endif  // ENABLE_SECURITY
  }

  auto plugin =
      app_manager_.GetPluginManager().FindPluginToProcess(function_id, source);
  if (!plugin) {
    SDL_LOG_WARN("Failed to find plugin : " << plugin.error());
    CheckSourceForUnsupportedRequest(message, source);
    return false;
  }
  application_manager::CommandFactory& factory = (*plugin).GetCommandFactory();
  auto command = factory.CreateCommand(message, source);

  if (!command) {
    SDL_LOG_WARN("Failed to create mobile command from smart object");
    return false;
  }

  if (message_type == mobile_apis::messageType::response) {
    if (command->Init() && command->CheckPermissions()) {
      command->Run();
      command->CleanUp();
    }
    return true;
  }
  if (message_type == mobile_apis::messageType::notification) {
    request_ctrl_.AddNotification(command);
    if (command->Init() && command->CheckPermissions()) {
      command->Run();
      if (command->CleanUp()) {
        request_ctrl_.RemoveNotification(command.get());
      }
      // If CleanUp returned false notification should remove it self.
    }
    return true;
  }

  if (message_type == mobile_apis::messageType::request &&
      source == commands::Command::CommandSource::SOURCE_SDL) {
    if (command->Init() && command->CheckPermissions()) {
      command->Run();
      command->CleanUp();
      return true;
    } else {
      return false;
    }
  }

  if (message_type == mobile_apis::messageType::request &&
      source == commands::Command::CommandSource::SOURCE_MOBILE) {
    mobile_apis::HMILevel::eType app_hmi_level =
        mobile_apis::HMILevel::INVALID_ENUM;
    if (app) {
      app_hmi_level =
          app->hmi_level(mobile_apis::PredefinedWindows::DEFAULT_WINDOW);
    }

    command->set_warning_info(warning_info);

    // commands will be launched from request_ctrl
    const request_controller::RequestController::TResult result =
        request_ctrl_.AddMobileRequest(command, app_hmi_level);

    if (result == request_controller::RequestController::TResult::SUCCESS) {
      SDL_LOG_DEBUG("Perform request");
    } else if (result == request_controller::RequestController::TResult::
                             TOO_MANY_PENDING_REQUESTS) {
      SDL_LOG_ERROR("RET  Unable top perform request: "
                    << "TOO_MANY_PENDING_REQUESTS");

      smart_objects::SmartObjectSPtr response =
          MessageHelper::CreateNegativeResponse(
              connection_key,
              static_cast<int32_t>(function_id),
              correlation_id,
              static_cast<int32_t>(
                  mobile_apis::Result::TOO_MANY_PENDING_REQUESTS));
      ApplicationSharedPtr app_ptr = app_manager_.application(connection_key);
      if (app_ptr) {
        app_ptr->usage_report().RecordRemovalsForBadBehavior();
      }

      SendMessageToMobile(response);
      return false;
    } else if (result == request_controller::RequestController::TResult::
                             TOO_MANY_REQUESTS) {
      SDL_LOG_ERROR("RET  Unable to perform request: "
                    << "TOO_MANY_REQUESTS");

      ManageMobileCommand(
          MessageHelper::GetOnAppInterfaceUnregisteredNotificationToMobile(
              connection_key,
              mobile_api::AppInterfaceUnregisteredReason::TOO_MANY_REQUESTS),
          commands::Command::SOURCE_SDL);

      app_manager_.UnregisterApplication(
          connection_key,
          mobile_apis::Result::TOO_MANY_PENDING_REQUESTS,
          false);
      ApplicationSharedPtr app_ptr = app_manager_.application(connection_key);
      if (app_ptr) {
        app_ptr->usage_report().RecordRemovalsForBadBehavior();
      }
      return false;
    } else if (result == request_controller::RequestController::TResult::
                             NONE_HMI_LEVEL_MANY_REQUESTS) {
      SDL_LOG_ERROR("RET  Unable to perform request: "
                    << "REQUEST_WHILE_IN_NONE_HMI_LEVEL");

      ManageMobileCommand(
          MessageHelper::GetOnAppInterfaceUnregisteredNotificationToMobile(
              connection_key,
              mobile_api::AppInterfaceUnregisteredReason::
                  REQUEST_WHILE_IN_NONE_HMI_LEVEL),
          commands::Command::SOURCE_SDL);

      ApplicationSharedPtr app_ptr = app_manager_.application(connection_key);
      if (app_ptr) {
        app_ptr->usage_report().RecordRemovalsForBadBehavior();
      }
      app_manager_.UnregisterApplication(
          connection_key, mobile_apis::Result::INVALID_ENUM, false);
      return false;
    } else {
      SDL_LOG_ERROR("RET  Unable to perform request: Unknown case");
      return false;
    }
    return true;
  }

  SDL_LOG_ERROR("RET  UNKNOWN MESSAGE TYPE " << message_type);
  return false;
}

bool RPCServiceImpl::ManageHMICommand(const commands::MessageSharedPtr message,
                                      commands::Command::CommandSource source) {
  return ManageHMICommand(message, source, std::string());
}

bool RPCServiceImpl::ManageHMICommand(const commands::MessageSharedPtr message,
                                      commands::Command::CommandSource source,
                                      const std::string warning_info) {
  SDL_LOG_AUTO_TRACE();

  if (!message) {
    SDL_LOG_WARN("Null-pointer message received.");
    return false;
  }

  if (app_manager_.IsLowVoltage()) {
    SDL_LOG_WARN("Low Voltage is active");
    return false;
  }

  MessageHelper::PrintSmartObject(*message);
  const int32_t function_id =
      (*(message.get()))[strings::params][strings::function_id].asInt();
  auto plugin =
      app_manager_.GetPluginManager().FindPluginToProcess(function_id, source);
  if (!plugin) {
    SDL_LOG_WARN("Filed to find plugin : " << plugin.error());
    return false;
  }

  application_manager::CommandFactory& factory = (*plugin).GetCommandFactory();
  auto command = factory.CreateCommand(message, source);

  if (!command) {
    SDL_LOG_WARN("Failed to create command from smart object");
    return false;
  }

  if ((*message).keyExists(strings::msg_params) &&
      (*message)[strings::msg_params].keyExists(strings::app_id)) {
    const auto connection_key =
        (*message)[strings::msg_params][strings::app_id].asUInt();

    auto app = app_manager_.application(static_cast<uint32_t>(connection_key));
    if (app && app_manager_.IsAppInReconnectMode(app->device(),
                                                 app->policy_app_id())) {
      commands_holder_.Suspend(
          app, CommandHolder::CommandType::kHmiCommand, source, message);
      return true;
    }
  }

  int32_t message_type =
      (*(message.get()))[strings::params][strings::message_type].asInt();

  if (kRequest == message_type) {
    SDL_LOG_DEBUG("ManageHMICommand");
    command->set_warning_info(warning_info);
    if (!app_manager_.IsStopping()) {
      request_ctrl_.AddHMIRequest(command);
    }
  }

  if (command->Init()) {
    command->Run();
    if (helpers::Compare<int32_t, helpers::EQ, helpers::ONE>(
            message_type, kResponse, kErrorResponse) &&
        message->getElement(strings::params)
            .keyExists(strings::correlation_id)) {
      const uint32_t correlation_id =
          (*(message.get()))[strings::params][strings::correlation_id].asUInt();
      const int32_t function_id =
          (*(message.get()))[strings::params][strings::function_id].asInt();
      request_ctrl_.OnHMIResponse(correlation_id, function_id);
    }
    return true;
  }
  return false;
}

void RPCServiceImpl::Handle(const impl::MessageToHmi message) {
  SDL_LOG_AUTO_TRACE();
  if (!hmi_handler_) {
    SDL_LOG_ERROR("Observer is not set for HMIMessageHandler");
    return;
  }

  hmi_handler_->SendMessageToHMI(message);
  SDL_LOG_INFO("Message for HMI given away");
}

void RPCServiceImpl::Handle(const impl::MessageToMobile message) {
  SDL_LOG_AUTO_TRACE();
  if (!protocol_handler_) {
    SDL_LOG_WARN("Protocol Handler is not set; cannot send message to mobile.");
    return;
  }

  std::shared_ptr<protocol_handler::RawMessage> rawMessage(
      MobileMessageHandler::HandleOutgoingMessageProtocol(message));

  if (!rawMessage) {
    SDL_LOG_ERROR("Failed to create raw message.");
    return;
  }

  bool is_final = message.is_final;
  bool close_session = false;
  if (is_final) {
    if (1 < app_manager_.connection_handler().GetConnectionSessionsCount(
                message->connection_key())) {
      is_final = false;
      close_session = true;
    }
  }

  const auto correlation_id = message->correlation_id();
  const auto app_id = message->connection_key();

  const bool is_service_secure =
      protocol_handler_->IsRPCServiceSecure(message->connection_key());

  const bool needs_encryption =
      MessageType::kNotification == message->type()
          ? rpc_protection_manager_->CheckPolicyEncryptionFlag(
                message->function_id(),
                app_manager_.application(app_id),
                is_service_secure)
          : rpc_protection_manager_->IsInEncryptionNeededCache(app_id,
                                                               correlation_id);

  if (needs_encryption && !is_service_secure) {
    SDL_LOG_WARN(
        "Unable to send rpc that requires encryption without secure "
        "rpc service");
    return;
  };

  protocol_handler_->SendMessageToMobileApp(
      rawMessage, needs_encryption, is_final);
  rpc_protection_manager_->RemoveFromEncryptionNeededCache(app_id,
                                                           correlation_id);
  SDL_LOG_INFO("Message for mobile given away");

  if (close_session) {
    app_manager_.connection_handler().CloseSession(
        message->connection_key(), connection_handler::kFinalMessage);
  }
}

void RPCServiceImpl::SendMessageToMobile(
    const application_manager::commands::MessageSharedPtr message,
    bool final_message) {
  SDL_LOG_AUTO_TRACE();

  if (!message) {
    SDL_LOG_ERROR("Null-pointer message received.");
    NOTREACHED();
    return;
  }

  if (!protocol_handler_) {
    SDL_LOG_WARN("No Protocol Handler set");
    return;
  }

  ApplicationSharedPtr app = app_manager_.application(
      (*message)[strings::params][strings::connection_key].asUInt());

  const bool is_result_code_exists =
      (*message)[strings::msg_params].keyExists(strings::result_code);
  bool allow_unknown_parameters = false;

  if (!app) {
    SDL_LOG_ERROR("No application associated with connection key");
    if (is_result_code_exists &&
        ((*message)[strings::msg_params][strings::result_code] ==
         ns_smart_device_link_rpc::V1::Result::UNSUPPORTED_VERSION)) {
      (*message)[strings::params][strings::protocol_version] =
          protocol_handler::MajorProtocolVersion::PROTOCOL_VERSION_1;
    } else {
      (*message)[strings::params][strings::protocol_version] =
          app_manager_.SupportedSDLVersion();
    }
  } else {
    (*message)[strings::params][strings::protocol_version] =
        app->protocol_version();
  }

  if (app && is_result_code_exists &&
      (*message)[strings::msg_params][strings::result_code] ==
          mobile_apis::Result::OUT_OF_MEMORY) {
    app->usage_report().RecordRejectionsSyncOutOfMemory();
  }

  // Messages to mobile are not yet prioritized so use default priority value
  std::shared_ptr<Message> message_to_send(
      new Message(protocol_handler::MessagePriority::kDefault));

  int32_t function_id = (*message)[jhs::S_PARAMS][jhs::S_FUNCTION_ID].asInt();
  bool rpc_passing = app_manager_.GetAppServiceManager()
                         .GetRPCPassingHandler()
                         .CanHandleFunctionID(function_id);
  if (IsAppServiceRPC(function_id,
                      commands::Command::CommandSource::SOURCE_SDL) ||
      rpc_passing) {
    SDL_LOG_DEBUG("Allowing unknown parameters for response function "
                  << function_id);
    allow_unknown_parameters = true;
  }

  if (rpc_passing &&
      !app_manager_.GetAppServiceManager()
           .GetRPCPassingHandler()
           .IsPassThroughMessage(
               (*message)[jhs::S_PARAMS][jhs::S_CORRELATION_ID].asUInt(),
               commands::Command::CommandSource::SOURCE_SDL,
               (*message)[jhs::S_PARAMS][jhs::S_MESSAGE_TYPE].asInt())) {
    allow_unknown_parameters = false;
  }

  const auto api_function_id = static_cast<mobile_apis::FunctionID::eType>(
      (*message)[strings::params][strings::function_id].asUInt());

  mobile_so_factory().attachSchema(*message, false);
  SDL_LOG_DEBUG(
      "Attached schema to message, result if valid: " << message->isValid());

  if (!ConvertSOtoMessage(
          (*message), (*message_to_send), allow_unknown_parameters)) {
    SDL_LOG_WARN("Can't send msg to Mobile: failed to create string");
    return;
  }

  smart_objects::SmartObject& msg_to_mobile = *message;
  // If correlation_id is not present, it is from-HMI message which should be
  // checked against policy permissions
  if (msg_to_mobile[strings::params].keyExists(strings::correlation_id)) {
    request_ctrl_.OnMobileResponse(
        msg_to_mobile[strings::params][strings::correlation_id].asUInt(),
        msg_to_mobile[strings::params][strings::connection_key].asUInt(),
        msg_to_mobile[strings::params][strings::function_id].asInt());
  } else if (app) {
    RPCParams params;

    const smart_objects::SmartObject& s_map = (*message)[strings::msg_params];
    const WindowID window_id =
        MessageHelper::ExtractWindowIdFromSmartObject(s_map);
    if (smart_objects::SmartType_Map == s_map.getType()) {
      smart_objects::SmartMap::iterator iter = s_map.map_begin();
      smart_objects::SmartMap::iterator iter_end = s_map.map_end();

      for (; iter != iter_end; ++iter) {
        if (true == iter->second.asBool()) {
          SDL_LOG_INFO("Request's param: " << iter->first);
          params.insert(iter->first);
        }
      }
    }
    const std::string string_functionID =
        MessageHelper::StringifiedFunctionID(api_function_id);
    const mobile_apis::Result::eType check_result =
        app_manager_.CheckPolicyPermissions(
            app, window_id, string_functionID, params);
    if (mobile_apis::Result::SUCCESS != check_result) {
      SDL_LOG_WARN("Function \"" << string_functionID << "\" (#"
                                 << api_function_id
                                 << ") not allowed by policy");
      return;
    }

    if (api_function_id == mobile_apis::FunctionID::OnSystemRequestID) {
      mobile_apis::RequestType::eType request_type =
          static_cast<mobile_apis::RequestType::eType>(
              (*message)[strings::msg_params][strings::request_type].asUInt());
      if (mobile_apis::RequestType::PROPRIETARY == request_type ||
          mobile_apis::RequestType::HTTP == request_type) {
        app_manager_.GetPolicyHandler().OnUpdateRequestSentToMobile();
      }
    }
  }

  if (message_to_send->binary_data()) {
    SDL_LOG_DEBUG(
        "Binary data size: " << message_to_send->binary_data()->size());
  }
  messages_to_mobile_.PostMessage(
      impl::MessageToMobile(message_to_send, final_message));
}

void RPCServiceImpl::SendMessageToHMI(
    const commands::MessageSharedPtr message) {
  SDL_LOG_AUTO_TRACE();
  if (!message) {
    SDL_LOG_WARN("Null-pointer message received.");
    NOTREACHED();
    return;
  }

  if (!hmi_handler_) {
    SDL_LOG_WARN("No HMI Handler set");
    return;
  }

  bool allow_unknown_parameters = false;
  // SmartObject |message| has no way to declare priority for now
  std::shared_ptr<Message> message_to_send(
      new Message(protocol_handler::MessagePriority::kDefault));
  if (!message_to_send) {
    SDL_LOG_ERROR("Null pointer");
    return;
  }

  hmi_so_factory().attachSchema(*message, false);
  SDL_LOG_INFO(
      "Attached schema to message, result if valid: " << message->isValid());

  if (IsAppServiceRPC((*message)[jhs::S_PARAMS][jhs::S_FUNCTION_ID].asInt(),
                      commands::Command::CommandSource::SOURCE_SDL_TO_HMI)) {
    SDL_LOG_DEBUG("Allowing unknown parameters for response function "
                  << (*message)[jhs::S_PARAMS][jhs::S_FUNCTION_ID].asInt());

    allow_unknown_parameters = true;
  }

  if (!ConvertSOtoMessage(
          *message, *message_to_send, allow_unknown_parameters)) {
    SDL_LOG_WARN("Cannot send message to HMI: failed to create string");
    return;
  }
  messages_to_hmi_.PostMessage(impl::MessageToHmi(message_to_send));
}

bool RPCServiceImpl::IsAppServiceRPC(int32_t function_id,
                                     commands::Command::CommandSource source) {
  // General RPCs related to App Services
  if ((source == commands::Command::CommandSource::SOURCE_MOBILE) ||
      (source ==
       commands::Command::CommandSource::SOURCE_SDL)) {  // MOBILE COMMANDS
    switch (function_id) {
      case mobile_apis::FunctionID::GetSystemCapabilityID:
      case mobile_apis::FunctionID::OnSystemCapabilityUpdatedID:
        return true;
        break;
    }
  } else if ((source == commands::Command::CommandSource::SOURCE_HMI) ||
             (source == commands::Command::CommandSource::
                            SOURCE_SDL_TO_HMI)) {  // HMI COMMANDS
    switch (function_id) {
      case hmi_apis::FunctionID::BasicCommunication_OnSystemCapabilityUpdated:
        return true;
        break;
    }
  }

  // RPCs handled by app services plugin
  auto plugin =
      app_manager_.GetPluginManager().FindPluginToProcess(function_id, source);
  if (!plugin) {
    return false;
  }
  if ((*plugin).PluginName() != plugin_names::app_service_rpc_plugin) {
    return false;
  }
  application_manager::CommandFactory& factory = (*plugin).GetCommandFactory();

  return factory.IsAbleToProcess(function_id, source);
}

void RPCServiceImpl::set_protocol_handler(
    protocol_handler::ProtocolHandler* handler) {
  protocol_handler_ = handler;
}

void RPCServiceImpl::set_hmi_message_handler(
    hmi_message_handler::HMIMessageHandler* handler) {
  hmi_handler_ = handler;
}

bool RPCServiceImpl::ConvertSOtoMessage(
    const ns_smart_device_link::ns_smart_objects::SmartObject& message,
    Message& output,
    const bool allow_unknown_parameters) {
  SDL_LOG_AUTO_TRACE();

  if (smart_objects::SmartType_Null == message.getType() ||
      smart_objects::SmartType_Invalid == message.getType()) {
    SDL_LOG_WARN("Invalid smart object received.");
    return false;
  }

  SDL_LOG_DEBUG(
      "Message with protocol: " << message.getElement(jhs::S_PARAMS)
                                       .getElement(jhs::S_PROTOCOL_TYPE)
                                       .asInt());

  std::string output_string;
  const int64_t protocol_type = message.getElement(jhs::S_PARAMS)
                                    .getElement(jhs::S_PROTOCOL_TYPE)
                                    .asInt();
  const int64_t protocol_version = message.getElement(jhs::S_PARAMS)
                                       .getElement(jhs::S_PROTOCOL_VERSION)
                                       .asInt();
  switch (protocol_type) {
    case 0: {
      if (protocol_version == 1) {
        if (!formatters::CFormatterJsonSDLRPCv1::toString(
                message, output_string, !allow_unknown_parameters)) {
          SDL_LOG_WARN("Failed to serialize smart object");
          return false;
        }
        output.set_protocol_version(
            protocol_handler::MajorProtocolVersion::PROTOCOL_VERSION_1);
      } else {
        if (!formatters::CFormatterJsonSDLRPCv2::toString(
                message, output_string, !allow_unknown_parameters)) {
          SDL_LOG_WARN("Failed to serialize smart object");
          return false;
        }
        output.set_protocol_version(
            static_cast<protocol_handler::MajorProtocolVersion>(
                protocol_version));
      }

      break;
    }
    case 1: {
      if (!formatters::FormatterJsonRpc::ToString(
              message, output_string, !allow_unknown_parameters)) {
        SDL_LOG_WARN("Failed to serialize smart object");
        return false;
      }
      output.set_protocol_version(
          protocol_handler::MajorProtocolVersion::PROTOCOL_VERSION_HMI);
      break;
    }
    default:
      NOTREACHED();
      return false;
  }

  SDL_LOG_DEBUG("Conversion result: " << output_string);

  output.set_connection_key(message.getElement(jhs::S_PARAMS)
                                .getElement(strings::connection_key)
                                .asInt());

  output.set_function_id(
      message.getElement(jhs::S_PARAMS).getElement(jhs::S_FUNCTION_ID).asInt());

  output.set_correlation_id(message.getElement(jhs::S_PARAMS)
                                .getElement(jhs::S_CORRELATION_ID)
                                .asInt());
  output.set_message_type(
      static_cast<MessageType>(message.getElement(jhs::S_PARAMS)
                                   .getElement(jhs::S_MESSAGE_TYPE)
                                   .asInt()));

  // Currently formatter creates JSON = 3 bytes for empty SmartObject.
  // workaround for notification. JSON must be empty
  if (mobile_apis::FunctionID::OnAudioPassThruID !=
      message.getElement(jhs::S_PARAMS)
          .getElement(strings::function_id)
          .asInt()) {
    output.set_json_message(output_string);
  }

  if (message.getElement(jhs::S_PARAMS).keyExists(strings::binary_data)) {
    const application_manager::BinaryData binaryData(
        message.getElement(jhs::S_PARAMS)
            .getElement(strings::binary_data)
            .asBinary());

    output.set_binary_data(&binaryData);
  }

  SDL_LOG_DEBUG("Successfully parsed smart object into message");
  return true;
}

void RPCServiceImpl::UpdateMobileRPCParams(
    const mobile_apis::FunctionID::eType& function_id,
    const mobile_apis::messageType::eType& message_type,
    const std::map<std::string, SMember>& members) {
  mobile_so_factory().ResetFunctionSchema(function_id, message_type);
  for (const auto& item : members) {
    mobile_so_factory().AddCustomMember(
        function_id, message_type, item.first, item.second);
  }
}

void RPCServiceImpl::UpdateHMIRPCParams(
    const hmi_apis::FunctionID::eType& function_id,
    const hmi_apis::messageType::eType& message_type,
    const std::map<std::string, SMember>& members) {
  hmi_so_factory().ResetFunctionSchema(function_id, message_type);
  for (const auto& item : members) {
    hmi_so_factory().AddCustomMember(
        function_id, message_type, item.first, item.second);
  }
}

hmi_apis::HMI_API& RPCServiceImpl::hmi_so_factory() {
  return hmi_so_factory_;
}

mobile_apis::MOBILE_API& RPCServiceImpl::mobile_so_factory() {
  return mobile_so_factory_;
}

void RPCServiceImpl::CheckSourceForUnsupportedRequest(
    const commands::MessageSharedPtr message,
    commands::Command::CommandSource source) {
  int32_t message_type =
      (*(message.get()))[strings::params][strings::message_type].asInt();
  uint32_t correlation_id =
      (*message)[strings::params].keyExists(strings::correlation_id)
          ? (*message)[strings::params][strings::correlation_id].asUInt()
          : 0;
  const uint32_t connection_key = static_cast<uint32_t>(
      (*message)[strings::params][strings::connection_key].asUInt());
  mobile_apis::FunctionID::eType function_id =
      static_cast<mobile_apis::FunctionID::eType>(
          (*message)[strings::params][strings::function_id].asInt());
  if ((source == commands::Command::CommandSource::SOURCE_MOBILE &&
       kRequest == message_type) ||
      (source == commands::Command::CommandSource::SOURCE_SDL &&
       kResponse == message_type)) {
    smart_objects::SmartObjectSPtr response =
        MessageHelper::CreateNegativeResponse(connection_key,
                                              static_cast<int32_t>(function_id),
                                              correlation_id,
                                              0);

    // Since we are dealing with an unknown RPC, there is no schema attached
    // to the message, so we have to convert the result to string directly
    std::string result_code;
    smart_objects::EnumConversionHelper<mobile_apis::Result::eType>::
        EnumToString(mobile_apis::Result::UNSUPPORTED_REQUEST, &result_code);
    (*response)[strings::msg_params][strings::result_code] = result_code;
    (*response)[strings::msg_params][strings::info] =
        "Module does not recognize this function id";

    SendMessageToMobile(response);
  }
}

}  // namespace rpc_service

}  // namespace application_manager
