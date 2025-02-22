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

#ifndef SRC_COMPONENTS_APPLICATION_MANAGER_RPC_PLUGINS_SDL_RPC_PLUGIN_INCLUDE_SDL_RPC_PLUGIN_COMMANDS_MOBILE_CREATE_INTERACTION_CHOICE_SET_REQUEST_H_
#define SRC_COMPONENTS_APPLICATION_MANAGER_RPC_PLUGINS_SDL_RPC_PLUGIN_INCLUDE_SDL_RPC_PLUGIN_COMMANDS_MOBILE_CREATE_INTERACTION_CHOICE_SET_REQUEST_H_
#include <map>
#include <string>

#include "application_manager/application.h"
#include "application_manager/commands/request_from_mobile_impl.h"
#include "application_manager/event_engine/event_observer.h"
#include "interfaces/MOBILE_API.h"
#include "utils/macro.h"

namespace sdl_rpc_plugin {
namespace app_mngr = application_manager;

class Application;

namespace commands {

/**
 * @brief CreateInteractionChoiceSetRequest command class
 **/
class CreateInteractionChoiceSetRequest
    : public app_mngr::commands::RequestFromMobileImpl {
 public:
  /**
   * @brief CreateInteractionChoiceSetRequest class constructor
   *
   * @param message Incoming SmartObject message
   **/
  CreateInteractionChoiceSetRequest(
      const app_mngr::commands::MessageSharedPtr& message,
      app_mngr::ApplicationManager& application_manager,
      app_mngr::rpc_service::RPCService& rpc_service,
      app_mngr::HMICapabilities& hmi_capabilities,
      policy::PolicyHandlerInterface& policy_handler);

  /**
   * @brief CreateInteractionChoiceSetRequest class destructor
   **/
  ~CreateInteractionChoiceSetRequest();

  /**
   * @brief Execute command
   **/
  void Run() FINAL;

  void on_event(const app_mngr::event_engine::Event& event) FINAL;

  void OnTimeOut() FINAL;

  /**
   * @brief Init sets hash update mode for request
   */
  bool Init() FINAL;

 private:
  /**
   * @brief DeleteChoices allows to walk through the sent commands collection
   * in order to sent appropriate DeleteCommand request.
   */
  void DeleteChoices();

  /**
   * @brief Calls after all responses from HMI were received.
   * Terminates request and sends successful response to mobile
   * if all responses were SUCCESS or calls DeleteChoices in other case.
   */
  void OnAllHMIResponsesReceived();

  /**
   * @brief The VRCommand struct
   * Collect minimum information about sent VR commands, for correctly
   * processing deleting sent commands if error from HMI received
   */
  struct VRCommandInfo {
    VRCommandInfo() : cmd_id_(0), succesful_response_received_(false) {}
    VRCommandInfo(uint32_t cmd_id)
        : cmd_id_(cmd_id), succesful_response_received_(false) {}
    uint32_t cmd_id_;
    bool succesful_response_received_;
  };

  typedef std::map<uint32_t, VRCommandInfo> SentCommandsMap;
  SentCommandsMap sent_commands_map_;

  int32_t choice_set_id_;
  size_t expected_chs_count_;
  size_t received_chs_count_;
  bool should_send_warnings_;

  /**
   * @brief Flag for stop sending VR commands to HMI, in case one of responses
   * failed
   */
  volatile bool error_from_hmi_;
  sync_primitives::Lock error_from_hmi_lock_;

  sync_primitives::RecursiveLock vr_commands_lock_;
  /*
   * @brief Sends VR AddCommand request to HMI
   *
   * @param app_id Application ID
   *
   */
  void SendVRAddCommandRequests(app_mngr::ApplicationSharedPtr const app);

  /*
   * @brief Checks incoming choiseSet params.
   * @param app Registred mobile application
   *
   * @return Mobile result code
   */
  mobile_apis::Result::eType CheckChoiceSet(
      app_mngr::ApplicationConstSharedPtr app);

  /*
   * @brief Checks if incoming choice set doesn't has similar VR synonyms.
   *
   * @param choice1  Choice to compare
   * @param choice2  Choice to compare
   *
   * return Return TRUE if there are similar VR synonyms in choice set,
   * otherwise FALSE
   */
  bool compareSynonyms(
      const ns_smart_device_link::ns_smart_objects::SmartObject& choice1,
      const ns_smart_device_link::ns_smart_objects::SmartObject& choice2);

  /*
   * @brief Checks VR synonyms ignoring differences in case.
   *
   * @param str1 VR synonym to compare
   * @param str2 VR synonym to compare
   *
   * return Return TRUE if there are similar VR synonyms in choice set,
   * otherwise FALSE
   */
  static bool compareStr(
      const ns_smart_device_link::ns_smart_objects::SmartObject& str1,
      const ns_smart_device_link::ns_smart_objects::SmartObject& str2);

  /**
   * @brief Checks choice set params(menuName, tertiaryText, ...)
   * When type is String there is a check on the contents \t\n \\t \\n
   * @param choice_set which must check
   * @return if choice_set contains \t\n \\t \\n return TRUE, FALSE otherwise
   */
  bool IsWhiteSpaceExist(const smart_objects::SmartObject& choice_set);

  /**
   * @brief ProcessHmiError process received error from HMI.
   * This function id not thread safe. It should be protected with
   * vr_commands_lock_
   * @param vr_result ERROR type
   */
  void ProcessHmiError(const hmi_apis::Common_Result::eType vr_result);

  /**
   * @brief ProcessSuccesfulHMIResponse process succesful response from HMI\
   * This function id not thread safe. It should be protected with
   * vr_commands_lock_
   * @param corr_id correlation id of received response
   * @return true if resuest with corr_itd was sent on HMI, false otherwise
   */
  bool ProcessSuccesfulHMIResponse(const uint32_t corr_id);

  /**
   * @brief CountReceivedVRResponses counts received HMI responses. Updated
   * request timeout if not all responses received
   * Send response to mobile if all responses received.
   */
  void CountReceivedVRResponses();

  DISALLOW_COPY_AND_ASSIGN(CreateInteractionChoiceSetRequest);
};

}  // namespace commands
}  // namespace sdl_rpc_plugin

#endif  // SRC_COMPONENTS_APPLICATION_MANAGER_RPC_PLUGINS_SDL_RPC_PLUGIN_INCLUDE_SDL_RPC_PLUGIN_COMMANDS_MOBILE_CREATE_INTERACTION_CHOICE_SET_REQUEST_H_
