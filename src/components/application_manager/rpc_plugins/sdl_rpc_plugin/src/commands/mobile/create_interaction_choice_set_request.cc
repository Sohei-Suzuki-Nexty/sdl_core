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

#include "sdl_rpc_plugin/commands/mobile/create_interaction_choice_set_request.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "application_manager/application_impl.h"
#include "application_manager/message_helper.h"
#include "application_manager/resumption/resume_ctrl.h"
#include "utils/gen_hash.h"
#include "utils/helpers.h"

const char* kInvalidImageWarningInfo = "Requested image(s) not found.";

namespace sdl_rpc_plugin {
namespace commands {

SDL_CREATE_LOG_VARIABLE("Commands")

using namespace application_manager;

CreateInteractionChoiceSetRequest::CreateInteractionChoiceSetRequest(
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
    , choice_set_id_(0)
    , expected_chs_count_(0)
    , received_chs_count_(0)
    , should_send_warnings_(false)
    , error_from_hmi_(false) {}

CreateInteractionChoiceSetRequest::~CreateInteractionChoiceSetRequest() {
  SDL_LOG_AUTO_TRACE();
}

void CreateInteractionChoiceSetRequest::Run() {
  SDL_LOG_AUTO_TRACE();
  using namespace mobile_apis;
  ApplicationSharedPtr app = application_manager_.application(connection_key());

  if (!app) {
    SDL_LOG_ERROR("NULL pointer");
    SendResponse(false, mobile_apis::Result::APPLICATION_NOT_REGISTERED);
    return;
  }

  for (uint32_t i = 0;
       i < (*message_)[strings::msg_params][strings::choice_set].length();
       ++i) {
    Result::eType verification_result_image = Result::SUCCESS;
    Result::eType verification_result_secondary_image = Result::SUCCESS;
    if ((*message_)[strings::msg_params][strings::choice_set][i].keyExists(
            strings::image)) {
      verification_result_image = MessageHelper::VerifyImage(
          (*message_)[strings::msg_params][strings::choice_set][i]
                     [strings::image],
          app,
          application_manager_);
    }
    if ((*message_)[strings::msg_params][strings::choice_set][i].keyExists(
            strings::secondary_image)) {
      verification_result_secondary_image = MessageHelper::VerifyImage(
          (*message_)[strings::msg_params][strings::choice_set][i]
                     [strings::secondary_image],
          app,
          application_manager_);
    }
    if (verification_result_image == Result::INVALID_DATA ||
        verification_result_secondary_image == Result::INVALID_DATA) {
      SDL_LOG_ERROR("Image verification failed.");
      SendResponse(false, Result::INVALID_DATA);
      return;
    } else if (verification_result_image == Result::WARNINGS ||
               verification_result_secondary_image == Result::WARNINGS) {
      should_send_warnings_ = true;
      break;
    }
  }

  choice_set_id_ =
      (*message_)[strings::msg_params][strings::interaction_choice_set_id]
          .asInt();

  const auto choice_set = app->FindChoiceSet(choice_set_id_);

  if (smart_objects::SmartType_Null != choice_set.getType()) {
    SDL_LOG_ERROR("Choice set with id " << choice_set_id_ << " is not found.");
    SendResponse(false, Result::INVALID_ID);
    return;
  }

  Result::eType result = CheckChoiceSet(app);
  if (Result::SUCCESS != result) {
    SendResponse(false, result);
    return;
  }
  auto vr_status = MessageHelper::CheckChoiceSetVRCommands(
      (*message_)[strings::msg_params][strings::choice_set]);
  if (vr_status == MessageHelper::ChoiceSetVRCommandsStatus::MIXED) {
    // this is an error
    SendResponse(false,
                 Result::INVALID_DATA,
                 "Some choices don't contain VR commands. Either all or none "
                 "must have voice commands.");
    return;  // exit now, this is a bad set
  } else if (vr_status == MessageHelper::ChoiceSetVRCommandsStatus::ALL) {
    // everyone had a vr command, setup the grammar
    uint32_t grammar_id = application_manager_.GenerateGrammarID();
    (*message_)[strings::msg_params][strings::grammar_id] = grammar_id;
  }
  // continue on as usual
  app->AddChoiceSet(choice_set_id_, (*message_)[strings::msg_params]);

  if (vr_status == MessageHelper::ChoiceSetVRCommandsStatus::ALL) {
    // we have VR commands
    SendVRAddCommandRequests(app);
  } else {
    // we have none, just return with success
    SendResponse(true, Result::SUCCESS);
  }
}

mobile_apis::Result::eType CreateInteractionChoiceSetRequest::CheckChoiceSet(
    ApplicationConstSharedPtr app) {
  using namespace smart_objects;
  SDL_LOG_AUTO_TRACE();

  std::set<uint32_t> choice_id_set;

  const SmartArray* choice_set =
      (*message_)[strings::msg_params][strings::choice_set].asArray();

  SmartArray::const_iterator current_choice_set_it = choice_set->begin();
  SmartArray::const_iterator next_choice_set_it;

  for (; choice_set->end() != current_choice_set_it; ++current_choice_set_it) {
    std::pair<std::set<uint32_t>::iterator, bool> ins_res =
        choice_id_set.insert(
            (*current_choice_set_it)[strings::choice_id].asInt());
    if (!ins_res.second) {
      SDL_LOG_ERROR("Choice with ID "
                    << (*current_choice_set_it)[strings::choice_id].asInt()
                    << " already exists");
      return mobile_apis::Result::INVALID_ID;
    }

    if (IsWhiteSpaceExist(*current_choice_set_it)) {
      SDL_LOG_ERROR("Incoming choice set has contains \t\n \\t \\n");
      return mobile_apis::Result::INVALID_DATA;
    }
    for (next_choice_set_it = current_choice_set_it + 1;
         choice_set->end() != next_choice_set_it;
         ++next_choice_set_it) {
      if (compareSynonyms(*current_choice_set_it, *next_choice_set_it)) {
        return mobile_apis::Result::DUPLICATE_NAME;
      }
    }
  }
  return mobile_apis::Result::SUCCESS;
}

bool CreateInteractionChoiceSetRequest::compareSynonyms(
    const ns_smart_device_link::ns_smart_objects::SmartObject& choice1,
    const ns_smart_device_link::ns_smart_objects::SmartObject& choice2) {
  // only compare if they both have vr commands
  if (!(choice1.keyExists(strings::vr_commands) &&
        choice2.keyExists(strings::vr_commands))) {
    return false;  // clearly there isn't a duplicate if one of them is null
  }
  smart_objects::SmartArray* vr_cmds_1 =
      choice1[strings::vr_commands].asArray();
  smart_objects::SmartArray* vr_cmds_2 =
      choice2[strings::vr_commands].asArray();

  smart_objects::SmartArray::iterator it;
  it = std::find_first_of(vr_cmds_1->begin(),
                          vr_cmds_1->end(),
                          vr_cmds_2->begin(),
                          vr_cmds_2->end(),
                          CreateInteractionChoiceSetRequest::compareStr);

  if (it != vr_cmds_1->end()) {
    SDL_LOG_INFO("Incoming choice set has duplicated VR synonyms "
                 << it->asString());
    return true;
  }

  return false;
}

bool CreateInteractionChoiceSetRequest::compareStr(
    const ns_smart_device_link::ns_smart_objects::SmartObject& str1,
    const ns_smart_device_link::ns_smart_objects::SmartObject& str2) {
  return 0 == strcasecmp(str1.asCharArray(), str2.asCharArray());
}

bool CreateInteractionChoiceSetRequest::IsWhiteSpaceExist(
    const smart_objects::SmartObject& choice_set) {
  SDL_LOG_AUTO_TRACE();

  const char* str = choice_set[strings::menu_name].asCharArray();
  if (!CheckSyntax(str)) {
    SDL_LOG_ERROR("Invalid menu_name syntax check failed");
    return true;
  }

  if (choice_set.keyExists(strings::secondary_text)) {
    str = choice_set[strings::secondary_text].asCharArray();
    if (!CheckSyntax(str)) {
      SDL_LOG_ERROR("Invalid secondary_text syntax check failed");
      return true;
    }
  }

  if (choice_set.keyExists(strings::tertiary_text)) {
    str = choice_set[strings::tertiary_text].asCharArray();
    if (!CheckSyntax(str)) {
      SDL_LOG_ERROR("Invalid tertiary_text syntax check failed");
      return true;
    }
  }

  if (choice_set.keyExists(strings::vr_commands)) {
    const size_t len = choice_set[strings::vr_commands].length();

    for (size_t i = 0; i < len; ++i) {
      str = choice_set[strings::vr_commands][i].asCharArray();
      if (!CheckSyntax(str)) {
        SDL_LOG_ERROR("Invalid vr_commands syntax check failed");
        return true;
      }
    }
  }

  if (choice_set.keyExists(strings::image)) {
    str = choice_set[strings::image][strings::value].asCharArray();
    if (!CheckSyntax(str)) {
      SDL_LOG_ERROR("Invalid image value syntax check failed");
      return true;
    }
  }

  if (choice_set.keyExists(strings::secondary_image)) {
    str = choice_set[strings::secondary_image][strings::value].asCharArray();
    if (!CheckSyntax(str)) {
      SDL_LOG_ERROR(
          "Invalid secondary_image value. "
          "Syntax check failed");
      return true;
    }
  }
  return false;
}

void CreateInteractionChoiceSetRequest::SendVRAddCommandRequests(
    application_manager::ApplicationSharedPtr const app) {
  SDL_LOG_AUTO_TRACE();

  smart_objects::SmartObject& choice_set = (*message_)[strings::msg_params];
  smart_objects::SmartObject msg_params =
      smart_objects::SmartObject(smart_objects::SmartType_Map);
  msg_params[strings::type] = hmi_apis::Common_VRCommandType::Choice;
  msg_params[strings::app_id] = app->app_id();
  msg_params[strings::grammar_id] = choice_set[strings::grammar_id];
  const uint32_t choice_count = choice_set[strings::choice_set].length();
  SetAllowedToTerminate(false);

  expected_chs_count_ = choice_count;
  size_t chs_num = 0;
  for (; chs_num < choice_count; ++chs_num) {
    {
      sync_primitives::AutoLock error_lock(error_from_hmi_lock_);
      if (error_from_hmi_) {
        SDL_LOG_WARN("Error from HMI received. Stop sending VRCommands");
        break;
      }
    }

    msg_params[strings::cmd_id] =
        choice_set[strings::choice_set][chs_num][strings::choice_id];
    msg_params[strings::vr_commands] =
        choice_set[strings::choice_set][chs_num][strings::vr_commands];

    sync_primitives::AutoLock commands_lock(vr_commands_lock_);
    const uint32_t vr_cmd_id = msg_params[strings::cmd_id].asUInt();
    StartAwaitForInterface(HmiInterfaces::HMI_INTERFACE_VR);
    const uint32_t vr_corr_id =
        SendHMIRequest(hmi_apis::FunctionID::VR_AddCommand, &msg_params, true);

    VRCommandInfo vr_command(vr_cmd_id);
    sent_commands_map_[vr_corr_id] = vr_command;
    SDL_LOG_DEBUG("VR_command sent corr_id " << vr_corr_id << " cmd_id "
                                             << vr_corr_id);
  }
  expected_chs_count_ = chs_num;
  SDL_LOG_DEBUG("expected_chs_count_ = " << expected_chs_count_);
}

void CreateInteractionChoiceSetRequest::ProcessHmiError(
    const hmi_apis::Common_Result::eType vr_result) {
  SDL_LOG_DEBUG("Hmi response is not Success: "
                << vr_result << ". Stop sending VRAddCommand requests");
  if (!error_from_hmi_) {
    error_from_hmi_ = true;
    std::string info =
        vr_result == hmi_apis::Common_Result::UNSUPPORTED_RESOURCE
            ? "VR is not supported by system"
            : "";
    SendResponse(false, GetMobileResultCode(vr_result), info.c_str());
  }
}

bool CreateInteractionChoiceSetRequest::ProcessSuccesfulHMIResponse(
    const uint32_t corr_id) {
  SentCommandsMap::iterator it = sent_commands_map_.find(corr_id);
  if (sent_commands_map_.end() == it) {
    SDL_LOG_WARN("HMI response for unknown VR command received");
    return false;
  }
  VRCommandInfo& vr_command = it->second;
  vr_command.succesful_response_received_ = true;
  return true;
}

void CreateInteractionChoiceSetRequest::CountReceivedVRResponses() {
  ++received_chs_count_;
  SDL_LOG_DEBUG("Got VR.AddCommand response, there are "
                << expected_chs_count_ - received_chs_count_
                << " more to wait.");
  if (received_chs_count_ < expected_chs_count_) {
    application_manager_.UpdateRequestTimeout(
        connection_key(), correlation_id(), default_timeout());
    SDL_LOG_DEBUG("Timeout for request was updated");
  } else {
    OnAllHMIResponsesReceived();
  }
}

void CreateInteractionChoiceSetRequest::on_event(
    const event_engine::Event& event) {
  using namespace hmi_apis;
  using namespace helpers;
  SDL_LOG_AUTO_TRACE();

  const smart_objects::SmartObject& message = event.smart_object();
  const Common_Result::eType result = static_cast<Common_Result::eType>(
      message[strings::params][hmi_response::code].asInt());
  const bool is_no_error = Compare<Common_Result::eType, EQ, ONE>(
      result, Common_Result::SUCCESS, Common_Result::WARNINGS);
  uint32_t corr_id = static_cast<uint32_t>(
      message[strings::params][strings::correlation_id].asUInt());
  if (event.id() == hmi_apis::FunctionID::VR_AddCommand) {
    EndAwaitForInterface(HmiInterfaces::HMI_INTERFACE_VR);
    {
      sync_primitives::AutoLock commands_lock(vr_commands_lock_);
      if (is_no_error) {
        if (!ProcessSuccesfulHMIResponse(corr_id)) {
          return;
        }
      } else {
        ProcessHmiError(result);
      }
    }
    CountReceivedVRResponses();
  }
}

void CreateInteractionChoiceSetRequest::OnTimeOut() {
  SDL_LOG_AUTO_TRACE();

  if (!error_from_hmi_) {
    RequestFromMobileImpl::OnTimeOut();
  }

  DeleteChoices();

  auto& resume_ctrl = application_manager_.resume_controller();

  resume_ctrl.HandleOnTimeOut(
      correlation_id(),
      static_cast<hmi_apis::FunctionID::eType>(function_id()));

  application_manager_.TerminateRequest(
      connection_key(), correlation_id(), function_id());
}

bool CreateInteractionChoiceSetRequest::Init() {
  hash_update_mode_ = HashUpdateMode::kDoHashUpdate;
  return true;
}

void CreateInteractionChoiceSetRequest::DeleteChoices() {
  SDL_LOG_AUTO_TRACE();

  ApplicationSharedPtr application =
      application_manager_.application(connection_key());
  if (!application) {
    SDL_LOG_ERROR("NULL pointer");
    return;
  }
  application->RemoveChoiceSet(choice_set_id_);

  smart_objects::SmartObject msg_param(smart_objects::SmartType_Map);
  msg_param[strings::app_id] = application->app_id();

  sync_primitives::AutoLock commands_lock(vr_commands_lock_);
  SentCommandsMap::const_iterator it = sent_commands_map_.begin();
  for (; it != sent_commands_map_.end(); ++it) {
    const VRCommandInfo& vr_command_info = it->second;
    if (vr_command_info.succesful_response_received_) {
      msg_param[strings::cmd_id] = vr_command_info.cmd_id_;
      SendHMIRequest(hmi_apis::FunctionID::VR_DeleteCommand, &msg_param);
    } else {
      SDL_LOG_WARN("succesful response has not been received for cmd_id =  "
                   << vr_command_info.cmd_id_);
    }
  }
  sent_commands_map_.clear();
}

void CreateInteractionChoiceSetRequest::OnAllHMIResponsesReceived() {
  SDL_LOG_AUTO_TRACE();

  if (!error_from_hmi_ && should_send_warnings_) {
    SendResponse(true, mobile_apis::Result::WARNINGS, kInvalidImageWarningInfo);
  } else if (!error_from_hmi_) {
    SendResponse(true, mobile_apis::Result::SUCCESS);
  } else {
    DeleteChoices();
  }

  application_manager_.TerminateRequest(
      connection_key(), correlation_id(), function_id());
}

}  // namespace commands

}  // namespace sdl_rpc_plugin
