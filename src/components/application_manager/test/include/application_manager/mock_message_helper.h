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

#ifndef SRC_COMPONENTS_APPLICATION_MANAGER_TEST_INCLUDE_APPLICATION_MANAGER_MOCK_MESSAGE_HELPER_H_
#define SRC_COMPONENTS_APPLICATION_MANAGER_TEST_INCLUDE_APPLICATION_MANAGER_MOCK_MESSAGE_HELPER_H_
#include "application_manager/application.h"
#include "application_manager/application_manager.h"
#include "application_manager/hmi_capabilities.h"
#include "application_manager/message_helper.h"
#include "application_manager/policies/policy_handler_interface.h"
#include "gmock/gmock.h"
#include "interfaces/HMI_API.h"
#include "policy/policy_types.h"
#include "smart_objects/smart_object.h"
#include "transport_manager/common.h"

namespace application_manager {

class MockMessageHelper {
 public:
  MOCK_METHOD2(CreateNotification,
               smart_objects::SmartObjectSPtr(mobile_apis::FunctionID::eType,
                                              uint32_t));
  MOCK_METHOD1(CreateHMINotification,
               smart_objects::SmartObjectSPtr(hmi_apis::FunctionID::eType));

  MOCK_METHOD1(GetHashUpdateNotification,
               smart_objects::SmartObjectSPtr(const uint32_t app_id));
  MOCK_METHOD2(SendHashUpdateNotification,
               void(const uint32_t app_id, ApplicationManager& app_mngr));
  MOCK_METHOD1(HMIToMobileResult,
               mobile_api::Result::eType(
                   const hmi_apis::Common_Result::eType hmi_result));
  MOCK_METHOD1(MobileToHMIResult,
               hmi_apis::Common_Result::eType(
                   const mobile_api::Result::eType mobile_result));
  MOCK_METHOD3(CreateDeviceListSO,
               smart_objects::SmartObjectSPtr(
                   const connection_handler::DeviceMap& devices,
                   const policy::PolicyHandlerInterface& policy_handler,
                   ApplicationManager& app_mngr));
  MOCK_METHOD3(SendNaviSetVideoConfig,
               void(int32_t app_id,
                    ApplicationManager& app_mngr,
                    const smart_objects::SmartObject& video_params));
  MOCK_METHOD2(SendNaviStartStream,
               void(int32_t connection_key, ApplicationManager& app_mngr));
  MOCK_METHOD2(SendNaviStopStream,
               void(int32_t connection_key, ApplicationManager& app_mngr));
  MOCK_METHOD3(SendOnAppPermissionsChangedNotification,
               void(uint32_t connection_key,
                    const policy::AppPermissions& permissions,
                    ApplicationManager& app_mngr));
  MOCK_METHOD2(SendAudioStartStream,
               void(int32_t connection_key, ApplicationManager& app_mngr));
  MOCK_METHOD2(SendAudioStopStream,
               void(int32_t connection_key, ApplicationManager& app_mngr));
  MOCK_METHOD3(SendOnDataStreaming,
               void(protocol_handler::ServiceType service,
                    bool available,
                    ApplicationManager& app_mngr));
  MOCK_METHOD3(CreateGetVehicleDataRequest,
               void(uint32_t correlation_id,
                    const std::vector<std::string>& params,
                    ApplicationManager& app_mngr));
  MOCK_METHOD3(SendUpdateSDLResponse,
               void(const std::string& result,
                    uint32_t correlation_id,
                    ApplicationManager& app_mngr));
  MOCK_METHOD3(SendGetUserFriendlyMessageResponse,
               void(const std::vector<policy::UserFriendlyMessage>& msg,
                    uint32_t correlation_id,
                    ApplicationManager& app_mngr));
  MOCK_METHOD3(SendGetStatusUpdateResponse,
               void(const std::string& status,
                    uint32_t correlation_id,
                    ApplicationManager& app_mngr));
  MOCK_METHOD2(SendOnStatusUpdate,
               void(const std::string& status, ApplicationManager& app_mngr));
  MOCK_METHOD3(SendSDLActivateAppResponse,
               void(policy::AppPermissions& permissions,
                    uint32_t correlation_id,
                    ApplicationManager& app_mngr));
  MOCK_METHOD1(SendGetSystemInfoRequest, void(ApplicationManager& app_mngr));
  MOCK_METHOD4(SendActivateAppToHMI,
               void(uint32_t const app_id,
                    ApplicationManager& application_manager,
                    hmi_apis::Common_HMILevel::eType level,
                    bool send_policy_priority));
  MOCK_METHOD2(CreateMessageForHMI,
               smart_objects::SmartObjectSPtr(hmi_apis::messageType::eType,
                                              const uint32_t));
  MOCK_METHOD2(CreateMessageForHMI,
               smart_objects::SmartObjectSPtr(hmi_apis::FunctionID::eType,
                                              const uint32_t));
  MOCK_METHOD2(CreateHMIStatusNotification,
               smart_objects::SmartObjectSPtr(ApplicationSharedPtr application,
                                              const WindowID window_id));
  MOCK_METHOD4(CreateButtonSubscriptionsHandlingRequestsList,
               smart_objects::SmartObjectList(
                   ApplicationConstSharedPtr application,
                   const ButtonSubscriptions& button_subscriptions,
                   const hmi_apis::FunctionID::eType function_id,
                   ApplicationManager& app_mngr));
  MOCK_METHOD4(SendPolicyUpdate,
               void(const std::string& file_path,
                    const uint32_t timeout,
                    const std::vector<int>& retries,
                    ApplicationManager& app_mngr));
  MOCK_METHOD2(SendDecryptCertificateToHMI,
               void(const std::string& file_name,
                    ApplicationManager& app_mngr));
#ifdef EXTERNAL_PROPRIETARY_MODE
  MOCK_METHOD5(
      SendGetListOfPermissionsResponse,
      void(const std::vector<policy::FunctionalGroupPermission>& permissions,
           const policy::ExternalConsentStatus& external_consent_status,
           uint32_t correlation_id,
           ApplicationManager& app_mngr,
           const bool success_flag));
#else
  MOCK_METHOD4(
      SendGetListOfPermissionsResponse,
      void(const std::vector<policy::FunctionalGroupPermission>& permissions,
           uint32_t correlation_id,
           ApplicationManager& app_mngr,
           const bool success_flag));
#endif  // #ifdef EXTERNAL_PROPRIETARY_MODE
  MOCK_METHOD4(SendOnPermissionsChangeNotification,
               void(uint32_t connection_key,
                    const policy::Permissions& permissions,
                    ApplicationManager& app_mngr,
                    const bool require_encryption));
  MOCK_METHOD4(SendPolicySnapshotNotification,
               void(uint32_t connection_key,
                    const std::string& snapshot_file_path,
                    const std::string& url,
                    ApplicationManager& app_mngr));
  MOCK_METHOD4(SendPolicySnapshotNotification,
               void(uint32_t connection_key,
                    const std::vector<uint8_t>& policy_data,
                    const std::string& url,
                    ApplicationManager& app_mngr));
  MOCK_METHOD2(CreateModuleInfoSO,
               smart_objects::SmartObjectSPtr(uint32_t function_id,
                                              ApplicationManager& app_mngr));
  MOCK_METHOD2(CreateUIResetGlobalPropertiesRequest,
               smart_objects::SmartObjectSPtr(
                   const ResetGlobalPropertiesResult& reset_result,
                   ApplicationSharedPtr application));

  MOCK_METHOD2(CreateTTSResetGlobalPropertiesRequest,
               smart_objects::SmartObjectSPtr(
                   const ResetGlobalPropertiesResult& reset_result,
                   ApplicationSharedPtr application));

  MOCK_METHOD2(CreateRCResetGlobalPropertiesRequest,
               smart_objects::SmartObjectSPtr(
                   const ResetGlobalPropertiesResult& reset_result,
                   ApplicationSharedPtr application));

  MOCK_METHOD2(CreateGlobalPropertiesRequestsToHMI,
               smart_objects::SmartObjectList(ApplicationConstSharedPtr app,
                                              ApplicationManager& app_mngr));
  MOCK_METHOD1(CreateSubscribeWayPointsMessageToHMI,
               smart_objects::SmartObjectSPtr(const uint32_t correlation_id));

  MOCK_METHOD4(CreateButtonSubscriptionHandlingRequestToHmi,
               smart_objects::SmartObjectSPtr(
                   const uint32_t app_id,
                   const hmi_apis::Common_ButtonName::eType button_name,
                   const hmi_apis::FunctionID::eType function_id,
                   application_manager::ApplicationManager& app_mngr));

  MOCK_METHOD2(CreateButtonNotificationToMobile,
               smart_objects::SmartObjectSPtr(
                   ApplicationSharedPtr app,
                   const smart_objects::SmartObject& source_message));

  MOCK_METHOD2(SendOnResumeAudioSourceToHMI,
               void(uint32_t app_id, ApplicationManager& app_mngr));
  MOCK_METHOD2(CreateAddSubMenuRequestsToHMI,
               smart_objects::SmartObjectList(ApplicationConstSharedPtr app,
                                              ApplicationManager& app_mngr));
  MOCK_METHOD2(CreateAddCommandRequestToHMI,
               smart_objects::SmartObjectList(ApplicationConstSharedPtr app,
                                              ApplicationManager& app_mngr));
  MOCK_METHOD1(CreateAddVRCommandRequestFromChoiceToHMI,
               smart_objects::SmartObjectList(ApplicationConstSharedPtr app));
  MOCK_METHOD2(SendGlobalPropertiesToHMI,
               void(ApplicationConstSharedPtr app,
                    ApplicationManager& app_mngr));
  MOCK_METHOD3(VerifyTtsFiles,
               mobile_apis::Result::eType(smart_objects::SmartObject& message,
                                          ApplicationConstSharedPtr app,
                                          ApplicationManager& app_mngr));
  MOCK_METHOD3(VerifyImageFiles,
               mobile_apis::Result::eType(smart_objects::SmartObject& message,
                                          ApplicationConstSharedPtr app,
                                          ApplicationManager& app_mngr));
  MOCK_METHOD3(VerifyImage,
               mobile_apis::Result::eType(smart_objects::SmartObject& message,
                                          ApplicationConstSharedPtr app,
                                          ApplicationManager& app_mngr));
  MOCK_METHOD1(CheckChoiceSetVRCommands,
               MessageHelper::ChoiceSetVRCommandsStatus(
                   const smart_objects::SmartObject&));

  MOCK_METHOD5(GetBCActivateAppRequestToHMI,
               smart_objects::SmartObjectSPtr(
                   ApplicationConstSharedPtr app,
                   const policy::PolicyHandlerInterface& policy_handler,
                   hmi_apis::Common_HMILevel::eType level,
                   bool send_policy_priority,
                   ApplicationManager& app_mngr));
  MOCK_METHOD2(GetBCCloseApplicationRequestToHMI,
               smart_objects::SmartObjectSPtr(ApplicationConstSharedPtr app,
                                              ApplicationManager& app_mngr));
  MOCK_METHOD2(GetOnAppInterfaceUnregisteredNotificationToMobile,
               ns_smart_device_link::ns_smart_objects::SmartObjectSPtr(
                   int32_t connection_key,
                   mobile_apis::AppInterfaceUnregisteredReason::eType reason));
  MOCK_METHOD4(ProcessSoftButtons,
               mobile_apis::Result::eType(
                   smart_objects::SmartObject& message_params,
                   ApplicationConstSharedPtr app,
                   const policy::PolicyHandlerInterface& policy_handler,
                   ApplicationManager& app_mngr));
  MOCK_METHOD4(SubscribeApplicationToSoftButton,
               void(smart_objects::SmartObject& message_params,
                    ApplicationSharedPtr app,
                    int32_t function_id,
                    const application_manager::WindowID window_id));
  MOCK_METHOD3(SubscribeApplicationToSoftButton,
               void(smart_objects::SmartObject& message_params,
                    ApplicationSharedPtr app,
                    int32_t function_id));
  MOCK_METHOD4(CreateNegativeResponse,
               smart_objects::SmartObjectSPtr(uint32_t connection_key,
                                              int32_t function_id,
                                              const uint32_t correlation_id,
                                              int32_t result_code));
  MOCK_METHOD4(
      CreateBlockedByPoliciesResponse,
      smart_objects::SmartObjectSPtr(mobile_apis::FunctionID::eType function_id,
                                     mobile_apis::Result::eType result,
                                     const uint32_t correlation_id,
                                     uint32_t connection_key));
  MOCK_METHOD0(vehicle_data, const VehicleData&());
  MOCK_METHOD1(SendStopAudioPathThru, bool(ApplicationManager& app_mngr));
  MOCK_METHOD1(StringifiedFunctionID,
               std::string(const mobile_apis::FunctionID::eType function_id));
  MOCK_METHOD2(SendUIChangeRegistrationRequestToHMI,
               void(ApplicationConstSharedPtr app,
                    ApplicationManager& app_mngr));
  MOCK_METHOD5(CreateDeviceInfo,
               bool(connection_handler::DeviceHandle device_handle,
                    const protocol_handler::SessionObserver& session_observer,
                    const policy::PolicyHandlerInterface& policy_handler,
                    ApplicationManager& app_mngr,
                    smart_objects::SmartObject* output));
  MOCK_METHOD5(CreateHMIApplicationStruct,
               bool(ApplicationConstSharedPtr app,
                    const protocol_handler::SessionObserver& session_observer,
                    const policy::PolicyHandlerInterface& policy_handler,
                    smart_objects::SmartObject* output,
                    ApplicationManager& app_mngr));
  MOCK_METHOD3(SendOnAppUnregNotificationToHMI,
               void(ApplicationConstSharedPtr app,
                    const bool is_unexpected_disconnect,
                    ApplicationManager& app_mngr));
  MOCK_METHOD2(CreateOnAppPropertiesChangeNotification,
               smart_objects::SmartObjectSPtr(const std::string& policy_app_id,
                                              ApplicationManager& app_mngr));
  MOCK_METHOD4(SendLaunchApp,
               void(const uint32_t connection_key,
                    const std::string& urlSchema,
                    const std::string& packageName,
                    ApplicationManager& app_man));
  MOCK_METHOD1(CreateUnsubscribeWayPointsRequest,
               smart_objects::SmartObjectSPtr(const uint32_t correlation_id));

  MOCK_METHOD2(SendQueryApps,
               void(const uint32_t connection_key,
                    ApplicationManager& app_man));
  MOCK_METHOD1(CreateAppVrHelp,
               smart_objects::SmartObjectSPtr(ApplicationConstSharedPtr app));
  MOCK_METHOD3(VerifyImageVrHelpItems,
               mobile_apis::Result::eType(smart_objects::SmartObject& message,
                                          ApplicationConstSharedPtr app,
                                          ApplicationManager& app_mngr));

  MOCK_METHOD1(GetPriorityCode, const uint32_t(const std::string& priority));

  MOCK_METHOD1(PrintSmartObject,
               bool(const smart_objects::SmartObject& object));

  MOCK_METHOD1(ExtractWindowIdFromSmartObject,
               WindowID(const smart_objects::SmartObject& s_map));

  MOCK_METHOD3(SendTTSGlobalProperties,
               void(ApplicationSharedPtr app,
                    const bool default_help_prompt,
                    ApplicationManager& app_man));
  MOCK_METHOD3(SendSetAppIcon,
               void(const uint32_t app_id,
                    const std::string& icon_path,
                    ApplicationManager& application_manager));
  MOCK_METHOD2(GetDeviceMacAddressForHandle,
               std::string(const transport_manager::DeviceHandle device_handle,
                           const ApplicationManager& app_mngr));
  MOCK_METHOD3(CreateDeleteUICommandRequest,
               smart_objects::SmartObjectSPtr(smart_objects::SmartObject* cmd,
                                              const uint32_t app_id,
                                              const uint32_t corr_id));
  MOCK_METHOD3(CreateDeleteVRCommandRequest,
               smart_objects::SmartObjectSPtr(smart_objects::SmartObject* cmd,
                                              ApplicationSharedPtr application,
                                              const uint32_t corr_id));
  MOCK_METHOD3(SendDeleteSubmenuRequest,
               void(smart_objects::SmartObject* cmd,
                    ApplicationSharedPtr application,
                    ApplicationManager& app_mngr));
  MOCK_METHOD3(SendDeleteChoiceSetRequest,
               void(smart_objects::SmartObject* cmd,
                    ApplicationSharedPtr application,
                    ApplicationManager& app_mngr));
  MOCK_METHOD2(SendResetPropertiesRequest,
               void(ApplicationSharedPtr application,
                    ApplicationManager& app_mngr));
  MOCK_METHOD1(CreateAppServiceCapabilities,
               smart_objects::SmartObject(
                   std::vector<smart_objects::SmartObject>& all_services));
  MOCK_METHOD2(BroadcastCapabilityUpdate,
               void(smart_objects::SmartObject& msg_params,
                    ApplicationManager& app_mngr));
  MOCK_METHOD3(CreateUICreateWindowRequestsToHMI,
               smart_objects::SmartObjectList(
                   ApplicationSharedPtr application,
                   ApplicationManager& app_manager,
                   const smart_objects::SmartObject& windows_info));
  MOCK_METHOD2(
      CreateDisplayCapabilityUpdateToMobile,
      smart_objects::SmartObjectSPtr(const smart_objects::SmartObject&,
                                     application_manager::Application&));
  MOCK_METHOD4(CreateOnServiceUpdateNotification,
               smart_objects::SmartObjectSPtr(
                   const hmi_apis::Common_ServiceType::eType service_type,
                   const hmi_apis::Common_ServiceEvent::eType service_event,
                   const hmi_apis::Common_ServiceStatusUpdateReason::eType
                       service_update_reason,
                   const uint32_t app_id));
  MOCK_METHOD4(CreateNegativeResponseFromHmi,
               smart_objects::SmartObjectSPtr(const int32_t function_id,
                                              const uint32_t correlation_id,
                                              const int32_t result_code,
                                              const std::string& info));

  MOCK_METHOD3(CreateResponseMessageFromHmi,
               smart_objects::SmartObjectSPtr(const int32_t function_id,
                                              const uint32_t correlation_id,
                                              const int32_t result_code));
  MOCK_METHOD3(CreateUIDeleteWindowRequestToHMI,
               smart_objects::SmartObjectSPtr(
                   application_manager::ApplicationSharedPtr application,
                   application_manager::ApplicationManager& app_mngr,
                   const application_manager::WindowID window_id));
  MOCK_METHOD1(RemoveEmptyMessageParams,
               uint16_t(const smart_objects::SmartObject&));

  MOCK_METHOD2(AddDefaultParamsToTireStatus,
               void(application_manager::ApplicationSharedPtr application,
                    smart_objects::SmartObject& response_from_hmi));

  static MockMessageHelper* message_helper_mock();
};

}  // namespace application_manager
#endif  // SRC_COMPONENTS_APPLICATION_MANAGER_TEST_INCLUDE_APPLICATION_MANAGER_MOCK_MESSAGE_HELPER_H_
