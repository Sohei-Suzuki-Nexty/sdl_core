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

#include "rc_rpc_plugin/commands/mobile/set_interior_vehicle_data_request.h"
#include "application_manager/application.h"
#include "application_manager/commands/command_request_test.h"
#include "application_manager/mock_application.h"
#include "interfaces/MOBILE_API.h"
#include "rc_rpc_plugin/mock/mock_interior_data_cache.h"
#include "rc_rpc_plugin/mock/mock_interior_data_manager.h"
#include "rc_rpc_plugin/mock/mock_rc_capabilities_manager.h"
#include "rc_rpc_plugin/mock/mock_rc_consent_manager.h"
#include "rc_rpc_plugin/mock/mock_resource_allocation_manager.h"
#include "rc_rpc_plugin/rc_module_constants.h"
#include "rc_rpc_plugin/rc_rpc_plugin.h"

#include <stdint.h>
#include "gtest/gtest.h"

using application_manager::ApplicationSet;
using application_manager::commands::MessageSharedPtr;
using test::components::application_manager_test::MockApplication;
using test::components::application_manager_test::MockApplicationManager;
using test::components::commands_test::CommandRequestTest;
using test::components::commands_test::CommandsTestMocks;
using test::components::commands_test::HMIResultCodeIs;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;

namespace {
const uint32_t kAppId = 0u;
const int kModuleId = 153u;
const uint32_t kConnectionKey = 1u;
const std::string kPolicyAppId = "Test";
}  // namespace

namespace rc_rpc_plugin_test {
using namespace rc_rpc_plugin;

class SetInteriorVehicleDataRequestTest
    : public CommandRequestTest<CommandsTestMocks::kIsNice> {
 public:
  SetInteriorVehicleDataRequestTest()
      : mock_app_(std::make_shared<NiceMock<MockApplication> >())
      , rc_app_extension_(
            std::make_shared<RCAppExtension>(rc_plugin_, *mock_app_))
      , rc_capabilities_(std::make_shared<smart_objects::SmartObject>(
            smart_objects::SmartType::SmartType_Array)) {}

  void SetUp() OVERRIDE {
    smart_objects::SmartObject control_caps((smart_objects::SmartType_Array));
    (*rc_capabilities_)[strings::kradioControlCapabilities] = control_caps;
    ON_CALL(app_mngr_, hmi_interfaces())
        .WillByDefault(ReturnRef(mock_hmi_interfaces_));
    ON_CALL(
        mock_hmi_interfaces_,
        GetInterfaceState(application_manager::HmiInterfaces::HMI_INTERFACE_RC))
        .WillByDefault(Return(application_manager::HmiInterfaces::
                                  InterfaceState::STATE_AVAILABLE));
    ON_CALL(app_mngr_, application(kAppId)).WillByDefault(Return(mock_app_));
    ON_CALL(*mock_app_, QueryInterface(RCAppExtension::RCAppExtensionID))
        .WillByDefault(Return(rc_app_extension_));

    ON_CALL(*mock_app_, policy_app_id()).WillByDefault(Return(kPolicyAppId));
    ON_CALL(mock_allocation_manager_, IsResourceFree(_, _))
        .WillByDefault(Return(true));
    ON_CALL(mock_allocation_manager_, AcquireResource(_, _, _))
        .WillByDefault(Return(AcquireResult::ALLOWED));
    ON_CALL(*mock_app_, app_id()).WillByDefault(Return(kAppId));
    ON_CALL(mock_policy_handler_,
            CheckHMIType(kPolicyAppId,
                         mobile_apis::AppHMIType::eType::REMOTE_CONTROL,
                         nullptr))
        .WillByDefault(Return(true));
    ON_CALL(mock_hmi_capabilities_, rc_capability())
        .WillByDefault(Return(rc_capabilities_));
    ON_CALL(mock_allocation_manager_, is_rc_enabled())
        .WillByDefault(Return(true));
    ON_CALL(mock_rc_capabilities_manager_, CheckIfModuleExistsInCapabilities(_))
        .WillByDefault(Return(true));
    ON_CALL(mock_rc_capabilities_manager_, GetModuleDataCapabilities(_, _))
        .WillByDefault(
            Return(std::make_pair("", capabilitiesStatus::kSuccess)));
    ON_CALL(mock_allocation_manager_, GetAccessMode())
        .WillByDefault(Return(hmi_apis::Common_RCAccessMode::AUTO_ALLOW));
  }

  MessageSharedPtr CreateBasicMessage() {
    MessageSharedPtr message = CreateMessage();
    (*message)[application_manager::strings::params]
              [application_manager::strings::function_id] =
                  mobile_apis::FunctionID::SetInteriorVehicleDataID;
    (*message)[application_manager::strings::params]
              [application_manager::strings::connection_key] = kConnectionKey;
    (*message)[application_manager::strings::params]
              [application_manager::strings::connection_key] = kAppId;
    return message;
  }

  template <class Command>
  std::shared_ptr<Command> CreateRCCommand(MessageSharedPtr& msg) {
    InitCommand(kDefaultTimeout_);
    RCCommandParams params{app_mngr_,
                           mock_rpc_service_,
                           mock_hmi_capabilities_,
                           mock_policy_handler_,
                           mock_allocation_manager_,
                           mock_interior_data_cache_,
                           mock_interior_data_manager_,
                           mock_rc_capabilities_manager_,
                           mock_rc_consent_manger_};
    return std::make_shared<Command>(msg ? msg : msg = CreateMessage(), params);
  }

 protected:
  testing::NiceMock<rc_rpc_plugin_test::MockResourceAllocationManager>
      mock_allocation_manager_;
  testing::NiceMock<rc_rpc_plugin_test::MockInteriorDataCache>
      mock_interior_data_cache_;
  testing::NiceMock<rc_rpc_plugin_test::MockInteriorDataManager>
      mock_interior_data_manager_;
  std::shared_ptr<MockApplication> mock_app_;
  RCRPCPlugin rc_plugin_;
  std::shared_ptr<RCAppExtension> rc_app_extension_;
  testing::NiceMock<rc_rpc_plugin_test::MockRCCapabilitiesManager>
      mock_rc_capabilities_manager_;
  smart_objects::SmartObjectSPtr rc_capabilities_;
  testing::NiceMock<MockRCConsentManager> mock_rc_consent_manger_;
};

TEST_F(SetInteriorVehicleDataRequestTest,
       Execute_ValidWithoutReadOnlyParams_ExpectResendToHMI) {
  // Arrange
  MessageSharedPtr mobile_message = CreateBasicMessage();
  ns_smart_device_link::ns_smart_objects::SmartObject& msg_params =
      (*mobile_message)[application_manager::strings::msg_params];
  msg_params[message_params::kModuleData][message_params::kModuleType] =
      mobile_apis::ModuleType::CLIMATE;
  smart_objects::SmartObject climate_control_data(smart_objects::SmartType_Map);
  climate_control_data[message_params::kFanSpeed] = 10;

  msg_params[message_params::kModuleData][message_params::kClimateControlData] =
      climate_control_data;
  // Expectations
  EXPECT_CALL(mock_policy_handler_, CheckModule(kPolicyAppId, _))
      .WillOnce(Return(rc_rpc_plugin::TypeAccess::kAllowed));

  EXPECT_CALL(
      mock_rpc_service_,
      ManageHMICommand(
          HMIResultCodeIs(hmi_apis::FunctionID::RC_SetInteriorVehicleData), _))
      .WillOnce(Return(true));
  // Act
  auto command =
      CreateRCCommand<rc_rpc_plugin::commands::SetInteriorVehicleDataRequest>(
          mobile_message);
  ASSERT_TRUE(command->Init());
  command->Run();
}

TEST_F(
    SetInteriorVehicleDataRequestTest,
    Execute_ValidWithSettableAndReadOnlyParams_ExpectCutReadOnlyAndResendToHMI) {
  // Arrange

  MessageSharedPtr mobile_message = CreateBasicMessage();
  ns_smart_device_link::ns_smart_objects::SmartObject& msg_params =
      (*mobile_message)[application_manager::strings::msg_params];
  msg_params[message_params::kModuleData][message_params::kModuleType] =
      mobile_apis::ModuleType::RADIO;
  smart_objects::SmartObject radio_control_data(smart_objects::SmartType_Map);
  radio_control_data[message_params::kState] = true;
  radio_control_data[message_params::kRadioEnable] = true;
  msg_params[message_params::kModuleData][message_params::kRadioControlData] =
      radio_control_data;

  // Expectations
  EXPECT_CALL(mock_policy_handler_, CheckModule(kPolicyAppId, _))
      .WillOnce(Return(rc_rpc_plugin::TypeAccess::kAllowed));

  EXPECT_CALL(app_mngr_, RemoveHMIFakeParameters(_, _));

  EXPECT_CALL(
      mock_rpc_service_,
      ManageHMICommand(
          HMIResultCodeIs(hmi_apis::FunctionID::RC_SetInteriorVehicleData), _))
      .WillOnce(Return(true));

  // Act
  std::shared_ptr<rc_rpc_plugin::commands::SetInteriorVehicleDataRequest>
      command = CreateRCCommand<
          rc_rpc_plugin::commands::SetInteriorVehicleDataRequest>(
          mobile_message);
  ASSERT_TRUE(command->Init());
  command->Run();
}

TEST_F(
    SetInteriorVehicleDataRequestTest,
    Execute_MessageValidationFailed_ExpectMessageNotSentToHMI_AndFalseSentToMobile) {
  // Arrange
  MessageSharedPtr mobile_message = CreateBasicMessage();
  ns_smart_device_link::ns_smart_objects::SmartObject& msg_params =
      (*mobile_message)[application_manager::strings::msg_params];
  msg_params[message_params::kModuleData][message_params::kModuleType] =
      mobile_apis::ModuleType::RADIO;

  msg_params[message_params::kModuleData][message_params::kClimateControlData] =
      smart_objects::SmartObject(smart_objects::SmartType_Map);

  // Expectations
  EXPECT_CALL(mock_policy_handler_, CheckModule(kPolicyAppId, _))
      .WillOnce(Return(rc_rpc_plugin::TypeAccess::kAllowed));

  EXPECT_CALL(
      mock_rpc_service_,
      ManageMobileCommand(test::components::commands_test::MobileResultCodeIs(
                              mobile_apis::Result::INVALID_DATA),
                          application_manager::commands::Command::SOURCE_SDL));

  // Act
  std::shared_ptr<rc_rpc_plugin::commands::SetInteriorVehicleDataRequest>
      command = CreateRCCommand<
          rc_rpc_plugin::commands::SetInteriorVehicleDataRequest>(
          mobile_message);
  ASSERT_TRUE(command->Init());
  command->Run();
}

TEST_F(SetInteriorVehicleDataRequestTest,
       Execute_ValidWithSettableParams_SUCCESSSendToHMI) {
  MessageSharedPtr mobile_message = CreateBasicMessage();
  auto& msg_params =
      (*mobile_message)[application_manager::strings::msg_params];
  msg_params[message_params::kModuleData][message_params::kModuleType] =
      mobile_apis::ModuleType::CLIMATE;

  msg_params[message_params::kModuleData][message_params::kClimateControlData] =
      smart_objects::SmartObject(smart_objects::SmartType_Map);

  msg_params[message_params::kModuleData][message_params::kClimateControlData]
            [message_params::kClimateEnable] = true;

  // Expectations
  EXPECT_CALL(mock_policy_handler_, CheckModule(kPolicyAppId, _))
      .WillOnce(Return(rc_rpc_plugin::TypeAccess::kAllowed));

  MessageSharedPtr message_from_mobile = CreateBasicMessage();

  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _))
      .WillOnce(DoAll(SaveArg<0>(&message_from_mobile), Return(true)));

  std::shared_ptr<rc_rpc_plugin::commands::SetInteriorVehicleDataRequest>
      command = CreateRCCommand<
          rc_rpc_plugin::commands::SetInteriorVehicleDataRequest>(
          mobile_message);

  ASSERT_TRUE(command->Init());
  command->Run();

  auto& msg_params_from_mobile =
      (*message_from_mobile)[application_manager::strings::msg_params];

  const bool climate_enable =
      msg_params_from_mobile[message_params::kModuleData]
                            [message_params::kClimateControlData]
                            [message_params::kClimateEnable]
                                .asBool();
  EXPECT_TRUE(climate_enable);
}

TEST_F(SetInteriorVehicleDataRequestTest,
       Execute_ValidWithSettableParams_SUCCESSSendToHMI_HDChannel) {
  MessageSharedPtr mobile_message = CreateBasicMessage();
  auto& msg_params =
      (*mobile_message)[application_manager::strings::msg_params];
  msg_params[message_params::kModuleData][message_params::kModuleType] =
      mobile_apis::ModuleType::RADIO;

  msg_params[message_params::kModuleData][message_params::kRadioControlData] =
      smart_objects::SmartObject(smart_objects::SmartType_Map);

  const std::uint32_t hd_channel = 2u;
  msg_params[message_params::kModuleData][message_params::kRadioControlData]
            [message_params::kHdChannel] = hd_channel;

  // Expectations
  EXPECT_CALL(mock_policy_handler_, CheckModule(kPolicyAppId, _))
      .WillOnce(Return(rc_rpc_plugin::TypeAccess::kAllowed));

  auto message_from_mobile = CreateBasicMessage();

  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _))
      .WillOnce(DoAll(SaveArg<0>(&message_from_mobile), Return(true)));

  std::shared_ptr<rc_rpc_plugin::commands::SetInteriorVehicleDataRequest>
      command = CreateRCCommand<
          rc_rpc_plugin::commands::SetInteriorVehicleDataRequest>(
          mobile_message);

  command->Init();
  command->Run();

  auto& msg_params_from_mobile =
      (*message_from_mobile)[application_manager::strings::msg_params];

  const uint64_t hd_channel_from_hmi =
      msg_params_from_mobile[message_params::kModuleData]
                            [message_params::kRadioControlData]
                            [message_params::kHdChannel]
                                .asUInt();

  EXPECT_EQ(hd_channel, hd_channel_from_hmi);
}
}  // namespace rc_rpc_plugin_test
