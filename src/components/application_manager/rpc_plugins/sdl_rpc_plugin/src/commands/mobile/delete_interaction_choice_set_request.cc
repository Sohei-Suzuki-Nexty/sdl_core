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

#include "sdl_rpc_plugin/commands/mobile/delete_interaction_choice_set_request.h"

#include "application_manager/application_impl.h"
#include "application_manager/message_helper.h"
#include "interfaces/HMI_API.h"
#include "interfaces/MOBILE_API.h"

namespace sdl_rpc_plugin {
using namespace application_manager;

namespace commands {

SDL_CREATE_LOG_VARIABLE("Commands")

DeleteInteractionChoiceSetRequest::DeleteInteractionChoiceSetRequest(
    const application_manager::commands::MessageSharedPtr& message,
    ApplicationManager& application_manager,
    rpc_service::RPCService& rpc_service,
    HMICapabilities& hmi_capabilities,
    policy::PolicyHandlerInterface& policy_handler)
    : RequestFromMobileImpl(message,
                            application_manager,
                            rpc_service,
                            hmi_capabilities,
                            policy_handler)
    , response_result_codes_() {}

DeleteInteractionChoiceSetRequest::~DeleteInteractionChoiceSetRequest() {}

void DeleteInteractionChoiceSetRequest::Run() {
  SDL_LOG_AUTO_TRACE();

  ApplicationSharedPtr app = application_manager_.application(connection_key());

  if (!app) {
    SDL_LOG_ERROR("No application associated with connection key "
                  << connection_key());
    SendResponse(false, mobile_apis::Result::APPLICATION_NOT_REGISTERED);
    return;
  }

  const int32_t choice_set_id =
      (*message_)[strings::msg_params][strings::interaction_choice_set_id]
          .asInt();

  const auto choice_set = app->FindChoiceSet(choice_set_id);

  if (smart_objects::SmartType_Null == choice_set.getType()) {
    SDL_LOG_ERROR("Choice set with id " << choice_set_id << " is not found.");
    SendResponse(false, mobile_apis::Result::INVALID_ID);
    return;
  }

  if (ChoiceSetInUse(app)) {
    SDL_LOG_ERROR("Choice set currently in use.");
    SendResponse(false, mobile_apis::Result::IN_USE);
    return;
  }
  SendVrDeleteCommand(app);
}

bool DeleteInteractionChoiceSetRequest::Init() {
  hash_update_mode_ = HashUpdateMode::kDoHashUpdate;
  return true;
}

void DeleteInteractionChoiceSetRequest::on_event(
    const event_engine::Event& event) {
  using namespace helpers;
  SDL_LOG_AUTO_TRACE();

  if (event.id() == hmi_apis::FunctionID::VR_DeleteCommand) {
    const smart_objects::SmartObject& message = event.smart_object();
    const auto result_code = static_cast<hmi_apis::Common_Result::eType>(
        message[strings::params][hmi_response::code].asInt());
    response_result_codes_.push_back(result_code);
    const std::uint32_t correlation_id = static_cast<uint32_t>(
        message[strings::params][strings::correlation_id].asUInt());

    bool should_send_response = false;
    {
      sync_primitives::AutoLock auto_lock(requests_lock_);
      auto found_request = sent_requests_.find(correlation_id);
      if (sent_requests_.end() == found_request) {
        SDL_LOG_WARN("Request with " << correlation_id
                                     << " correlation_id is not found.");
        return;
      }

      sent_requests_.erase(found_request);
      should_send_response = sent_requests_.empty();
    }

    if (should_send_response) {
      SendDeleteInteractionChoiceSetResponse();
    }
  }
}

bool DeleteInteractionChoiceSetRequest::ChoiceSetInUse(
    ApplicationConstSharedPtr app) {
  SDL_LOG_AUTO_TRACE();
  if (!app->is_perform_interaction_active()) {
    return false;
  }
  const DataAccessor<PerformChoiceSetMap> accessor =
      app->performinteraction_choice_set_map();
  const PerformChoiceSetMap& choice_set_map = accessor.GetData();

  const uint32_t choice_set_id =
      (*message_)[strings::msg_params][strings::interaction_choice_set_id]
          .asUInt();

  PerformChoiceSetMap::const_iterator it = choice_set_map.begin();
  for (; choice_set_map.end() != it; ++it) {
    const PerformChoice& choice = it->second;
    PerformChoice::const_iterator choice_it = choice.begin();
    for (; choice.end() != choice_it; ++choice_it) {
      if (choice_it->first == choice_set_id) {
        SDL_LOG_ERROR("Choice set with id " << choice_set_id << " is in use.");
        return true;
      }
    }
  }
  return true;
}

void DeleteInteractionChoiceSetRequest::SendVrDeleteCommand(
    application_manager::ApplicationSharedPtr app) {
  SDL_LOG_AUTO_TRACE();

  const uint32_t choice_set_id =
      (*message_)[strings::msg_params][strings::interaction_choice_set_id]
          .asUInt();

  smart_objects::SmartObject choice_set = app->FindChoiceSet(choice_set_id);

  if (smart_objects::SmartType_Null == choice_set.getType()) {
    SDL_LOG_ERROR("Choice set with id " << choice_set_id << " is not found.");
    return;
  }

  smart_objects::SmartObject msg_params =
      smart_objects::SmartObject(smart_objects::SmartType_Map);
  msg_params[strings::app_id] = app->app_id();
  msg_params[strings::type] = hmi_apis::Common_VRCommandType::Choice;
  msg_params[strings::grammar_id] = choice_set[strings::grammar_id];
  choice_set = choice_set[strings::choice_set];

  sync_primitives::AutoLock auto_lock(requests_lock_);
  for (uint32_t i = 0; i < choice_set.length(); ++i) {
    msg_params[strings::cmd_id] = choice_set[i][strings::choice_id];
    const uint32_t delte_cmd_hmi_corr_id = SendHMIRequest(
        hmi_apis::FunctionID::VR_DeleteCommand, &msg_params, true);
    sent_requests_.insert(delte_cmd_hmi_corr_id);
  }
}

void DeleteInteractionChoiceSetRequest::
    SendDeleteInteractionChoiceSetResponse() {
  hmi_apis::Common_Result::eType result_code =
      hmi_apis::Common_Result::INVALID_ENUM;
  for (const auto& code : response_result_codes_) {
    if (result_code == hmi_apis::Common_Result::INVALID_ENUM) {
      result_code = code;
      continue;
    }

    if (!application_manager::commands::IsHMIResultSuccess(code)) {
      result_code = code;
    }
  }

  const bool response_result = PrepareResultForMobileResponse(
      result_code, HmiInterfaces::InterfaceID::HMI_INTERFACE_VR);

  if (response_result) {
    ApplicationSharedPtr app =
        application_manager_.application(connection_key());
    if (!app) {
      SDL_LOG_ERROR("Application with connection key " << connection_key()
                                                       << " did not find.");
      return;
    }
    const uint32_t choice_set_id =
        (*message_)[strings::msg_params][strings::interaction_choice_set_id]
            .asUInt();
    app->RemoveChoiceSet(choice_set_id);
  }

  SDL_LOG_DEBUG("Response sent. Result code: " << result_code
                                               << " sussess: " << std::boolalpha
                                               << result_code);
  SendResponse(response_result, MessageHelper::HMIToMobileResult(result_code));
}

}  // namespace commands

}  // namespace sdl_rpc_plugin
