/*
 * Copyright (c) 2016, Ford Motor Company
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

#ifndef SRC_COMPONENTS_INCLUDE_APPLICATION_MANAGER_APPLICATION_MANAGER_H_
#define SRC_COMPONENTS_INCLUDE_APPLICATION_MANAGER_APPLICATION_MANAGER_H_

#include <ctime>
#include <set>
#include <string>
#include <vector>
#include "application_manager/application.h"
#include "application_manager/commands/command.h"
#include "connection_handler/connection_handler.h"
#include "utils/data_accessor.h"

#include "interfaces/v4_protocol_v1_2_no_extra.h"
#include "interfaces/v4_protocol_v1_2_no_extra_schema.h"

#include "application_manager/application_manager_settings.h"
#include "application_manager/hmi_interfaces.h"
#include "application_manager/plugin_manager/rpc_plugin_manager.h"
#include "application_manager/request_controller.h"
#include "application_manager/request_timeout_handler.h"
#include "application_manager/state_controller.h"
#include "policy/policy_types.h"

namespace app_launch {
class AppLaunchCtrl;
}  // namespace app_launch
namespace media_manager {
class MediaManager;
}

// Other compomnents class declaration
namespace hmi_message_handler {
class HMIMessageHandler;
}
namespace protocol_handler {
class ProtocolHandler;
}
namespace connection_handler {
class ConnectionHandler;
}
namespace resumption {
class ResumeCtrl;
}

namespace policy {
class PolicyHandlerInterface;
}

namespace application_manager {

namespace event_engine {
class EventDispatcher;
}
namespace rpc_service {
class RPCService;
}
namespace rpc_handler {
class RPCHandler;
}
namespace request_controller {
class RequestTimeoutHandler;
class RequestController;
}  // namespace request_controller
class Application;
class AppServiceManager;
class StateControllerImpl;
struct CommandParametersPermissions;
struct ResetGlobalPropertiesResult;
using policy::RPCParams;
typedef std::vector<ApplicationSharedPtr> AppSharedPtrs;
struct ApplicationsSorter {
  bool operator()(const ApplicationSharedPtr lhs,
                  const ApplicationSharedPtr rhs) const {
    if (lhs->app_id() == rhs->app_id()) {
      return lhs->device() < rhs->device();
    }
    return lhs->app_id() < rhs->app_id();
  }
};
struct ApplicationsPolicyAppIdSorter {
  bool operator()(const ApplicationSharedPtr lhs,
                  const ApplicationSharedPtr rhs) {
    if (lhs->policy_app_id() == rhs->policy_app_id()) {
      return lhs->device() < rhs->device();
    }
    return lhs->policy_app_id() < rhs->policy_app_id();
  }
};

struct ExpiredButtonRequestData {
  uint32_t app_id_;
  hmi_apis::Common_ButtonName::eType button_name_;
};

typedef std::set<ApplicationSharedPtr, ApplicationsSorter> ApplicationSet;

typedef std::set<ApplicationSharedPtr, ApplicationsPolicyAppIdSorter>
    AppsWaitRegistrationSet;

/**
 * @brief ReregisterWaitList is list of applications expected to be
 * re-registered after transport switching is complete
 */
typedef std::vector<ApplicationSharedPtr> ReregisterWaitList;

// typedef for Applications list iterator
typedef ApplicationSet::iterator ApplicationSetIt;

// typedef for Applications list const iterator
typedef ApplicationSet::const_iterator ApplicationSetConstIt;

class ApplicationManager {
 public:
  virtual ~ApplicationManager() {}

  /**
   * Inits application manager
   */
  virtual bool Init(resumption::LastStateWrapperPtr last_state_wrapper,
                    media_manager::MediaManager* media_manager) = 0;

  /**
   * @brief Stop work.
   *
   * @return TRUE on success otherwise FALSE.
   **/
  virtual bool Stop() = 0;

  virtual void set_hmi_message_handler(
      hmi_message_handler::HMIMessageHandler* handler) = 0;

  /**
   * @brief set_protocol_handler
   * @param handler
   * set protocol handler
   */
  virtual void set_protocol_handler(
      protocol_handler::ProtocolHandler* handler) = 0;

  virtual void set_connection_handler(
      connection_handler::ConnectionHandler* handler) = 0;

  virtual DataAccessor<ApplicationSet> applications() const = 0;
  virtual DataAccessor<AppsWaitRegistrationSet> pending_applications()
      const = 0;

  /**
   * @brief CreatePendingApplication Add applicaiton to pending state
   * All info mandatory for application will be fetched from policy database.
   * Application will be stored to internal pending applicaitons list.
   * UpdateAppList will not be trigerred
   * Application will be created if app exists in policy database and
   * nicknames are not empty
   * @param policy_app_id app id to store
   */
  virtual void CreatePendingLocalApplication(
      const std::string& policy_app_id) = 0;

  /**
   * @brief RemovePendingApplication Remove applicaiton from pending state
   * Application will be removed from the internal pending applicaitons list.
   * UpdateAppList will not be trigerred
   * @param policy_app_id app id to remove
   */
  virtual void RemovePendingApplication(const std::string& policy_app_id) = 0;

  virtual DataAccessor<ReregisterWaitList> reregister_applications() const = 0;

  virtual ApplicationSharedPtr application(uint32_t app_id) const = 0;
  virtual ApplicationSharedPtr active_application() const = 0;
  virtual void OnQueryAppsRequest(
      const connection_handler::DeviceHandle device) = 0;

  virtual ApplicationSharedPtr get_full_or_limited_application() const = 0;

  /**
   * Function used only by HMI request/response/notification base classes
   * to change HMI app id to Mobile app id and vice versa.
   * Don't use it inside Core
   */
  virtual ApplicationSharedPtr application_by_hmi_app(
      uint32_t hmi_app_id) const = 0;

  virtual ApplicationSharedPtr application_by_policy_id(
      const std::string& policy_app_id) const = 0;

  virtual ApplicationSharedPtr pending_application_by_policy_id(
      const std::string& policy_app_id) const = 0;

  virtual ApplicationSharedPtr reregister_application_by_policy_id(
      const std::string& policy_app_id) const = 0;

  virtual AppSharedPtrs applications_by_name(
      const std::string& app_name) const = 0;
  virtual AppSharedPtrs applications_by_button(uint32_t button) = 0;
  virtual AppSharedPtrs applications_with_navi() = 0;

  /**
   * @brief application find application by device and policy identifier
   * @param device_id device id
   * @param policy_app_id poilcy identifier
   * @return pointer to application in case if application exist, in other case
   * return empty shared pointer
   */
  virtual ApplicationSharedPtr application(
      const std::string& device_id, const std::string& policy_app_id) const = 0;

  virtual void ChangeAppsHMILevel(uint32_t app_id,
                                  mobile_apis::HMILevel::eType level) = 0;

  virtual std::vector<std::string> devices(
      const std::string& policy_app_id) const = 0;

  virtual plugin_manager::RPCPluginManager& GetPluginManager() = 0;

  virtual AppServiceManager& GetAppServiceManager() = 0;

#ifdef BUILD_TESTS
  virtual void SetPluginManager(
      std::unique_ptr<plugin_manager::RPCPluginManager>& plugin_manager) = 0;
#endif

  virtual std::vector<ApplicationSharedPtr>
  applications_with_mobile_projection() = 0;
  /**
   * @brief Returns media application with LIMITED HMI Level if exists
   *
   * @return Shared pointer to application if application does not
   * exist returns empty shared pointer.
   */
  virtual ApplicationSharedPtr get_limited_media_application() const = 0;

  /**
   * @brief Returns navigation application with LIMITED HMI Level if exists
   *
   * @return Shared pointer to application if application does not
   * exist returns empty shared pointer
   */
  virtual ApplicationSharedPtr get_limited_navi_application() const = 0;

  /**
   * @brief Returns voice communication application with
   * LIMITED HMI Level if exists
   *
   * @return Shared pointer to application if application does not
   * exist returns empty shared pointer
   */
  virtual ApplicationSharedPtr get_limited_voice_application() const = 0;

  virtual ApplicationSharedPtr get_limited_mobile_projection_application()
      const = 0;

  /**
   * @brief Retrieves application id associated with correlation id
   *
   * @param correlation_id Correlation ID of the HMI request
   *
   * @return application id associated with correlation id
   */
  virtual uint32_t application_id(const int32_t correlation_id) = 0;

  /**
   * @brief Sets application id correlation id
   *
   * @param correlation_id Correlation ID of the HMI request
   * @param app_id Application ID
   */
  virtual void set_application_id(const int32_t correlation_id,
                                  const uint32_t app_id) = 0;
  /**
   * @brief get_current_audio_source
   * @return current audio source
   */
  virtual uint32_t get_current_audio_source() const = 0;

  /**
   * @brief set_current_audio_source
   * @param source
   * set current audio source
   */
  virtual void set_current_audio_source(const uint32_t source) = 0;

  /**
   * @brief OnHMIStateChanged the callback that allows SDL to react when
   * application's HMI level has been changed.
   *
   * @param app_id application identifier for which HMIState has been chaned.
   *
   * @param from previous HMIState.
   * @param to current HMIState.
   */
  virtual void OnHMIStateChanged(const uint32_t app_id,
                                 const HmiStatePtr from,
                                 const HmiStatePtr to) = 0;

  /**
   * @brief Updates streaming service status for specified session and notifies
   * HMI via notification if required
   * @param service_type Id of service which status should be updated
   * @param app_id Id of session which status should be updated
   * @param streaming_data_available Availability of streaming data for
   * specified session
   */
  virtual void ProcessOnDataStreamingNotification(
      const protocol_handler::ServiceType service_type,
      const uint32_t app_id,
      const bool streaming_data_available) = 0;

  /**
   * @brief Checks if driver distraction state is valid, creates message
   * which is sent to the application if allowed, otherwise it is added
   * to a list of postponed messages.
   * @param application contains registered application.
   */
  virtual void SendDriverDistractionState(ApplicationSharedPtr application) = 0;

  virtual void SendGetIconUrlNotifications(
      const uint32_t connection_key, ApplicationSharedPtr application) = 0;

  /**
   * @brief Checks if Application is subscribed for way points
   * @param app_id Application id
   * @return true if Application is subscribed for way points
   * otherwise false
   */
  virtual bool IsAppSubscribedForWayPoints(uint32_t app_id) const = 0;

  /**
   * @brief Checks if Application is subscribed for way points
   * @param app Application reference
   * @return true if Application is subscribed for way points
   * otherwise false
   */
  virtual bool IsAppSubscribedForWayPoints(Application& app) const = 0;

  /**
   * @brief Subscribe Application for way points
   * @param app_id Application id
   * @param response_from_hmi True if a successful HMI response was received
   * when subscribing
   */
  virtual void SubscribeAppForWayPoints(uint32_t app_id,
                                        bool response_from_hmi = true) = 0;

  /**
   * @brief Subscribe Application for way points
   * @param app Application pointer
   * @param response_from_hmi True if a successful HMI response was received
   * when subscribing
   */
  virtual void SubscribeAppForWayPoints(ApplicationSharedPtr app,
                                        bool response_from_hmi = true) = 0;

  /**
   * @brief Unsubscribe Application for way points
   * @param app_id Application id
   * @param response_from_hmi True if a successful HMI response was received
   * when unsubscribing
   */
  virtual void UnsubscribeAppFromWayPoints(uint32_t app_id,
                                           bool response_from_hmi = true) = 0;

  /**
   * @brief Unsubscribe Application for way points
   * @param app Application pointer
   * @param response_from_hmi True if a successful HMI response was received
   * when unsubscribing
   */
  virtual void UnsubscribeAppFromWayPoints(ApplicationSharedPtr app,
                                           bool response_from_hmi = true) = 0;

  /**
   * @brief Is SDL Core subscribed to HMI waypoints
   * @return true if SDL Core is subscribed to HMI waypoints, otherwise false
   */
  virtual bool IsSubscribedToHMIWayPoints() const = 0;

  /**
   * @brief Is Any Application is subscribed for way points
   * @return true if some app is subscribed otherwise false
   */
  virtual bool IsAnyAppSubscribedForWayPoints() const = 0;

  /**
   * @brief Save message after OnWayPointsChangeNotification reception
   * @param way_points_message pointer to the smartobject
   * @param app_id the app ID of the provider sending the way points update or 0
   * if the HMI is the provider
   */
  virtual void SaveWayPointsMessage(
      smart_objects::SmartObjectSPtr way_points_message,
      uint32_t app_id = 0) = 0;

  /**
   * @brief Get subscribed for way points
   * @return reference to set of subscribed apps for way points
   */
  virtual const std::set<uint32_t> GetAppsSubscribedForWayPoints() const = 0;

  virtual void RemoveHMIFakeParameters(
      application_manager::commands::MessageSharedPtr& message,
      const hmi_apis::FunctionID::eType& function_id) = 0;

  virtual mobile_api::HMILevel::eType GetDefaultHmiLevel(
      ApplicationConstSharedPtr application) const = 0;

  /**
   * @brief Checks if required transport for resumption is available
   *
   * The required transport can be configured through smartDeviceLink.ini file.
   *
   * @param application an instance of the app to check
   * @return true if the app is connected through one of the required
   *         transports, false otherwise
   */
  virtual bool CheckResumptionRequiredTransportAvailable(
      ApplicationConstSharedPtr application) const = 0;

  /**
   * @brief hmi_capabilities return capabilities of hmi
   * @return capabilities of hmi
   */
  virtual HMICapabilities& hmi_capabilities() = 0;

  virtual const HMICapabilities& hmi_capabilities() const = 0;

  virtual void ProcessQueryApp(const smart_objects::SmartObject& sm_object,
                               const uint32_t connection_key) = 0;

  /**
   * @brief ProcessReconnection handles reconnection flow for application on
   * transport switch
   * @param application Pointer to switched application, must be validated
   * before passing
   * @param connection_key Connection key from registration request of switched
   * application
   */
  virtual void ProcessReconnection(ApplicationSharedPtr application,
                                   const uint32_t connection_key) = 0;

  virtual bool is_attenuated_supported() const = 0;

  /**
   * @brief Checks if application with the same HMI type
   *        (media, voice communication or navi) exists
   *        in HMI_FULL or HMI_LIMITED level.
   *
   * @param app Pointer to application to compare with
   *
   * @return true if exist otherwise false
   */
  virtual bool IsAppTypeExistsInFullOrLimited(
      ApplicationConstSharedPtr app) const = 0;

  /**
   * @brief Sets default HMI level and configure application after its
   * registration
   * @param app Application
   */
  virtual void OnApplicationRegistered(ApplicationSharedPtr app) = 0;

  /**
   * @brief OnApplicationSwitched starts activies postponed during application
   * transport switching
   * @param app Application
   */
  virtual void OnApplicationSwitched(ApplicationSharedPtr app) = 0;

  virtual connection_handler::ConnectionHandler& connection_handler() const = 0;
  virtual protocol_handler::ProtocolHandler& protocol_handler() const = 0;
  virtual policy::PolicyHandlerInterface& GetPolicyHandler() = 0;
  virtual const policy::PolicyHandlerInterface& GetPolicyHandler() const = 0;

  virtual rpc_service::RPCService& GetRPCService() const = 0;
  virtual rpc_handler::RPCHandler& GetRPCHandler() const = 0;
  virtual request_controller::RequestTimeoutHandler&
  get_request_timeout_handler() const = 0;
  virtual request_controller::RequestController& get_request_controller()
      const = 0;
  virtual bool is_stopping() const = 0;
  virtual bool is_audio_pass_thru_active() const = 0;

  virtual uint32_t GetNextMobileCorrelationID() = 0;
  virtual uint32_t GetNextHMICorrelationID() = 0;
  virtual uint32_t GenerateNewHMIAppID() = 0;

  /**
   * @brief Ends opened navi services audio and video for application
   * @param app_id Application id
   */
  virtual void EndNaviServices(uint32_t app_id) = 0;

  /**
   * @brief Ends opened navi service audio or video for application
   * @param app_id Application id
   * @param service_type Service type to check
   */
  virtual void EndService(const uint32_t app_id,
                          const protocol_handler::ServiceType service_type) = 0;

  /**
   * @brief returns true if low voltage state is active
   */
  virtual bool IsLowVoltage() const = 0;

  /**
   * @brief Starts AudioPassThru process by given application
   * @param app_id ID of the application which starts the process
   * @return true if AudioPassThru can be started, false otherwise
   */
  virtual bool BeginAudioPassThru(uint32_t app_id) = 0;

  /**
   * @brief Finishes already started AudioPassThru process by given application
   * @param app_id ID of the application which started the process
   * @return true if AudioPassThru process has been started with given
   * application and thus it can be stopped, false otherwise
   */
  virtual bool EndAudioPassThru(uint32_t app_id) = 0;

  virtual void ConnectToDevice(const std::string& device_mac) = 0;

  virtual void OnHMIReady() = 0;

  /**
   * @brief Send GetCapabilities requests for
   * each interface (VR, TTS, UI etc) to HMI
   */
  virtual void RequestForInterfacesAvailability() = 0;

  virtual void DisconnectCloudApp(ApplicationSharedPtr app) = 0;

  virtual void RefreshCloudAppInformation() = 0;

  virtual std::string PolicyIDByIconUrl(const std::string url) = 0;

  virtual void SetIconFileFromSystemRequest(const std::string policy_id) = 0;

  /**
   * @brief Retrieve the current connection status of a cloud app
   * @param app A cloud application
   * @return The current CloudConnectionStatus of app
   */
  virtual hmi_apis::Common_CloudConnectionStatus::eType
  GetCloudAppConnectionStatus(ApplicationConstSharedPtr app) const = 0;

  virtual bool IsHMICooperating() const = 0;

  /*
   * @brief Hold or respond to all pending RAI requests
   * @param hmi_cooperating new state to be set
   */
  virtual void SetHMICooperating(const bool hmi_cooperating) = 0;

  /**
   * @brief Notifies all components interested in Vehicle Data update
   * i.e. new value of odometer etc and returns list of applications
   * subscribed for event.
   * @param vehicle_info Enum value of type of vehicle data
   * @param new value (for integer values currently) of vehicle data
   */
  virtual void IviInfoUpdated(const std::string& vehicle_info, int value) = 0;

  /**
   * @brief Creates the application object for a newly registered application.
   * This method performs initialiation of the app object by setting properties
   * and starting the resumption process if applicable.
   * @param request_for_registration Smart Object RegisterAppInterfaceRequest
   * message received from mobile.
   */
  virtual ApplicationSharedPtr RegisterApplication(
      const std::shared_ptr<smart_objects::SmartObject>&
          request_for_registration) = 0;
  /**
   * @brief Completes initialization process by adding the app to the
   * applications accessor. App is now accessible by all other Core components.
   * @param application ApplicationSharedPtr for newly registered application.
   * @param connection_key Internal application id of registering application.
   */
  virtual void FinalizeAppRegistration(ApplicationSharedPtr application,
                                       const uint32_t connection_key) = 0;

  virtual void SendUpdateAppList() = 0;

  virtual void MarkAppsGreyOut(const connection_handler::DeviceHandle handle,
                               bool is_greyed_out) = 0;

  /**
   * @brief Returns pointer to application-to-be-registered (from QUERY_APP
   * list)
   * @param hmi_id HMI application id
   * @return Pointer to application or uninitialized shared pointer
   */
  virtual ApplicationConstSharedPtr WaitingApplicationByID(
      const uint32_t hmi_id) const = 0;

  /**
   * @brief Returns list of applications-to-be-registered (QUERY_APP list)
   * @return Locked list of applications
   */
  virtual DataAccessor<AppsWaitRegistrationSet> AppsWaitingForRegistration()
      const = 0;

  virtual bool IsAppsQueriedFrom(
      const connection_handler::DeviceHandle handle) const = 0;

  virtual bool IsStopping() const = 0;

  /**
   * @brief Waits for HMI readiness and blocks thread if it's not ready yet
   * @return true if HMI is ready and cooperating, otherwise returns false
   */
  virtual bool WaitForHmiIsReady() = 0;

  virtual void RemoveAppFromTTSGlobalPropertiesList(const uint32_t app_id) = 0;

  /**
   * @brief Resets application's global properties to default values
   * @param global_properties_ids container with global properties IDs to reset
   * @param app_id ID of app which properties to reset
   * @return struct with flags indicating success global properties reset
   */
  virtual ResetGlobalPropertiesResult ResetGlobalProperties(
      const smart_objects::SmartObject& global_properties_ids,
      const uint32_t app_id) = 0;

  /**
   * @brief Resets all application's global properties to default values
   * returning struct that indicates which properties have been
   * successfully reset.
   * @param app_id ID of app which properties to reset
   * @return struct with flags indicating global properties reset
   */
  virtual ResetGlobalPropertiesResult ResetAllApplicationGlobalProperties(
      const uint32_t app_id) = 0;

  virtual mobile_apis::Result::eType SaveBinary(
      const std::vector<uint8_t>& binary_data,
      const std::string& file_path,
      const std::string& file_name,
      const uint64_t offset) = 0;
  /*
   * @brief Sets SDL access to all mobile apps
   *
   * @param allowed SDL access to all mobile apps
   */
  virtual void SetAllAppsAllowed(const bool allowed) = 0;

  /*
   * @brief Sets state for driver distraction
   *
   * @param state New state to be set
   */
  virtual void set_driver_distraction_state(
      const hmi_apis::Common_DriverDistractionState::eType state) = 0;

  /*
   * @brief Starts audio pass thru thread
   *
   * @param session_key     Session key of connection for Mobile side
   * @param correlation_id  Correlation id for response for Mobile side
   * @param max_duration    Max duration of audio recording in milliseconds
   * @param sampling_rate   Value for rate(8, 16, 22, 44 kHz)
   * @param bits_per_sample The quality the audio is recorded.
   * @param audio_type      Type of audio data
   */
  virtual void StartAudioPassThruThread(int32_t session_key,
                                        int32_t correlation_id,
                                        int32_t max_duration,
                                        int32_t sampling_rate,
                                        int32_t bits_per_sample,
                                        int32_t audio_type) = 0;

  virtual void StartDevicesDiscovery() = 0;

  virtual void StopAudioPassThru(int32_t application_key) = 0;

  /**
   * @brief TerminateRequest forces termination of request
   * @param connection_key - application id of request
   * @param corr_id correlation id of request
   * @param function_id function id of request
   */
  virtual void TerminateRequest(const uint32_t connection_key,
                                const uint32_t corr_id,
                                const int32_t function_id) = 0;

  /**
   * @brief RetainRequestInstance retains request instance by its
   * connection+correlation key
   * @param connection_key connection key of application
   * @param correlation_id correlation id of request
   * @return true if request was rerained. false if the request with such
   * connection+correlation key was not found
   */
  virtual bool RetainRequestInstance(const uint32_t connection_key,
                                     const uint32_t correlation_id) = 0;

  /**
   * @brief RemoveRetainedRequest removes request instance retained before
   * @param connection_key connection key of application
   * @param correlation_id correlation id of request
   */
  virtual bool RemoveRetainedRequest(const uint32_t connection_key,
                                     const uint32_t correlation_id) = 0;

  /**
   * @brief IsStillWaitingForResponse check if request is still waiting for
   * response
   * @param connection_key connection key of application
   * @param correlation_id correlation id of request
   * @return true if request is still waiting for response, otherwise returns
   * false
   */
  virtual bool IsStillWaitingForResponse(
      const uint32_t connection_key, const uint32_t correlation_id) const = 0;

  /*
   * @brief Closes application by id
   *
   * @param app_id Application id
   * @param reason reason of unregistering application
   * @param is_resuming describes - is this unregister
   *        is normal or need to be resumed\
   * @param is_unexpected_disconnect
   * Indicates if connection was unexpectedly lost(TM layer, HB)
   */
  virtual void UnregisterApplication(const uint32_t& app_id,
                                     mobile_apis::Result::eType reason,
                                     bool is_resuming = false,
                                     bool is_unexpected_disconnect = false) = 0;

  /**
   * @ Updates request timeout
   *
   * @param connection_key Connection key of application
   * @param mobile_correlation_id Correlation ID of the mobile request
   * @param new_timeout_value New timeout in milliseconds to be set
   */
  virtual void UpdateRequestTimeout(uint32_t connection_key,
                                    uint32_t mobile_correlation_id,
                                    uint32_t new_timeout_value) = 0;

  virtual void IncreaseForwardedRequestTimeout(
      uint32_t connection_key, uint32_t mobile_correlation_id) = 0;

  virtual StateController& state_controller() = 0;

  virtual void SetUnregisterAllApplicationsReason(
      mobile_api::AppInterfaceUnregisteredReason::eType reason) = 0;

  /*
   * @brief Called on Master_reset or Factory_defaults
   * when User chooses to reset HU.
   * Resets Policy Table if applicable.
   */
  virtual void HeadUnitReset(
      mobile_api::AppInterfaceUnregisteredReason::eType reason) = 0;

  /**
   * @brief Checks application HMI state and returns true if streaming is
   * allowed
   * @param app_id Application id
   * @param service_type Service type to check
   * @return True if streaming is allowed, false in other case
   */
  virtual bool HMIStateAllowsStreaming(
      uint32_t app_id, protocol_handler::ServiceType service_type) const = 0;

  /**
   * @brief Checks, if given RPC is allowed at current HMI level for specific
   * application in policy table
   * @param app Application
   * @param window_id id of application's window
   * @param function_id FunctionID of RPC
   * @param params_permissions Permissions for RPC parameters (e.g.
   * SubscribeVehicleData) defined in policy table
   * @return SUCCESS, if allowed, otherwise result code of check
   */
  virtual mobile_apis::Result::eType CheckPolicyPermissions(
      const ApplicationSharedPtr app,
      const WindowID window_id,
      const std::string& function_id,
      const RPCParams& rpc_params,
      CommandParametersPermissions* params_permissions = NULL) = 0;

  /**
   * @brief IsApplicationForbidden allows to distinguish if application is
   * not allowed to register, because of spamming.
   *
   * @param connection_key the connection key ofthe required application
   *
   * @param policy_app_id application's mobile(policy) identifier.
   *
   * @return true in case application is allowed to register, false otherwise.
   */
  virtual bool IsApplicationForbidden(
      uint32_t connection_key, const std::string& policy_app_id) const = 0;

  /**
   * @brief IsAppInReconnectMode check if application belongs to session
   * affected by transport switching at the moment
   * @param device_id device indentifier
   * @param policy_app_id Application id
   * @return True if application is registered within session being switched,
   * otherwise - false
   */
  virtual bool IsAppInReconnectMode(
      const connection_handler::DeviceHandle& device_id,
      const std::string& policy_app_id) const = 0;

  virtual resumption::ResumeCtrl& resume_controller() = 0;

  /**
   * @brief hmi_interfaces getter for hmi_interfaces component, that handle
   * hmi_instrfaces state
   * @return reference to hmi_interfaces component
   */
  virtual HmiInterfaces& hmi_interfaces() = 0;

  virtual ns_smart_device_link_rpc::V1::v4_protocol_v1_2_no_extra&
  mobile_v4_protocol_so_factory() = 0;

  virtual app_launch::AppLaunchCtrl& app_launch_ctrl() = 0;

  virtual protocol_handler::MajorProtocolVersion SupportedSDLVersion()
      const = 0;

  /**
   * @brief Applies functor for each plugin
   * @param functor Functor that will be applied to each plugin
   */
  virtual void ApplyFunctorForEachPlugin(
      std::function<void(plugin_manager::RPCPlugin&)> functor) = 0;

  /*
   * @brief Converts connection string transport type representation
   * to HMI Common_TransportType
   *
   * @param transport_type String representing connection type
   *
   * @return Corresponding HMI TransporType value
   */
  virtual hmi_apis::Common_TransportType::eType GetDeviceTransportType(
      const std::string& transport_type) = 0;

  /**
   * @brief method adds application
   * to tts_global_properties_app_list_
   * @param app_id contains application which will
   * send TTS global properties after timeout
   */
  virtual void AddAppToTTSGlobalPropertiesList(const uint32_t app_id) = 0;

  /**
   * Generate grammar ID
   *
   * @return New grammar ID
   */
  virtual uint32_t GenerateGrammarID() = 0;

  virtual policy::DeviceConsent GetUserConsentForDevice(
      const std::string& device_id) const = 0;

  /**
   * @brief Handle sequence for unauthorized application
   * @param app_id Application id
   */
  virtual void OnAppUnauthorized(const uint32_t& app_id) = 0;

  virtual bool ActivateApplication(ApplicationSharedPtr app) = 0;

  /**
   * @brief Callback calls when application starts/stops data streaming
   * @param app_id Streaming application id
   * @param service_type Streaming service type
   * @param state True if streaming started, false if streaming stopped.
   */
  virtual void OnAppStreaming(uint32_t app_id,
                              protocol_handler::ServiceType service_type,
                              bool state) = 0;

  /**
   * @brief CreateRegularState create regular HMI state for application
   * @param app Application
   * @param window_type type of window
   * @param hmi_level of returned state
   * @param audio_state of returned state
   * @param system_context of returned state
   * @return new regular HMI state
   */
  virtual HmiStatePtr CreateRegularState(
      std::shared_ptr<Application> app,
      const mobile_apis::WindowType::eType window_type,
      const mobile_apis::HMILevel::eType hmi_level,
      const mobile_apis::AudioStreamingState::eType audio_state,
      const mobile_apis::VideoStreamingState::eType video_state,
      const mobile_apis::SystemContext::eType system_context) const = 0;

  /**
   * @brief Checks if application can stream (streaming service is started and
   * streaming is enabled in application)
   * @param app_id Application id
   * @param service_type Service type to check
   * @return True if streaming is allowed, false in other case
   */
  virtual bool CanAppStream(
      uint32_t app_id, protocol_handler::ServiceType service_type) const = 0;

  /**
   * @brief ForbidStreaming forbid the stream over the certain application.
   * @param app_id the application's id which should stop streaming.
   * @param service_type Service type to check
   */
  virtual void ForbidStreaming(uint32_t app_id,
                               protocol_handler::ServiceType service_type) = 0;

  /**
   * @brief Called when application successfully completes streaming
   * configuration
   * @param app_id Streaming application id
   * @param service_type Streaming service type
   */
  virtual void OnStreamingConfigurationSuccessful(
      uint32_t app_id, protocol_handler::ServiceType service_type) = 0;

  /**
   * @brief Called when application fails streaming configuration
   * @param app_id Streaming application id
   * @param rejected_params list of rejected parameter names
   * @param reason NACK reason
   */
  virtual void OnStreamingConfigurationFailed(
      uint32_t app_id,
      std::vector<std::string>& rejected_params,
      const std::string& reason) = 0;

  virtual const ApplicationManagerSettings& get_settings() const = 0;
  // Extract the app ID to use internally based on the UseFullAppID .ini setting
  virtual std::string GetCorrectMobileIDFromMessage(
      const commands::MessageSharedPtr& message) const = 0;

  virtual event_engine::EventDispatcher& event_dispatcher() = 0;

  virtual uint32_t GetAvailableSpaceForApp(const std::string& folder_name) = 0;
  virtual void OnTimerSendTTSGlobalProperties() = 0;
  virtual void OnLowVoltage() = 0;
  virtual void OnWakeUp() = 0;

  virtual bool IsSOStructValid(
      const hmi_apis::StructIdentifiers::eType struct_id,
      const smart_objects::SmartObject& display_capabilities) = 0;

  /**
   * @brief Unsubscribe application specified in message from softbuttons.
   * @param response_message - Response message received from HMI.
   * @return bool - Result of unsubscribing process.
   */
  virtual bool UnsubscribeAppFromSoftButtons(
      const commands::MessageSharedPtr response_message) = 0;

  /**
   * @brief Save subscribe/unsubscribe button request after timeout to ensure
   * possibility to align mobile subscription/unsubscription status with actual
   * subscription/unsubscription status on HMI
   * @param app_id Application id from request message
   * @param corr_id Correlation id
   * @param button_name name of button to subscribe/unsubscribe
   */
  virtual void AddExpiredButtonRequest(
      const uint32_t app_id,
      const int32_t corr_id,
      const hmi_apis::Common_ButtonName::eType button_name) = 0;

  /**
   * @brief Return optional structure with information regarding
   * subscribe/unsubscribe button request data
   * @param corr_id Correlation id
   * @return optional structure with subscribe/unsubscribe button request data
   */
  virtual utils::Optional<ExpiredButtonRequestData> GetExpiredButtonRequestData(
      const int32_t corr_id) const = 0;

  /**
   * @brief Delete data about already processed expired subscribe/unsubscribe
   * button request in case if HMI send response to expired request
   * @param corr_id Correlation id
   */
  virtual void DeleteExpiredButtonRequest(const int32_t corr_id) = 0;
};

}  // namespace application_manager

#endif  // SRC_COMPONENTS_INCLUDE_APPLICATION_MANAGER_APPLICATION_MANAGER_H_
