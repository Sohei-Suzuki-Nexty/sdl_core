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

#include "sdl_rpc_plugin/commands/mobile/system_request.h"

#include <stdio.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "application_manager/policies/policy_handler_interface.h"
#include "formatters/CFormatterJsonBase.h"
#include "interfaces/MOBILE_API.h"
#include "json/json.h"
#include "policy/policy_table/enums.h"
#include "utils/custom_string.h"
#include "utils/file_system.h"
#include "utils/helpers.h"
#include "utils/jsoncpp_reader_wrapper.h"

namespace sdl_rpc_plugin {
using namespace application_manager;

SDL_CREATE_LOG_VARIABLE("Commands")
namespace {

#ifdef ENABLE_LOG
const char* kQueryAppsValidationFailedPrefix =
    ":QUERY_APPS_VALIDATION_FAILED: ";
#endif

const unsigned int kVrSynonymLengthMax = 40U;
const unsigned int kVrSynonymLengthMin = 1U;
const unsigned int kTtsNameLengthMax = 500U;
const unsigned int kVrArraySizeMax = 100U;
const unsigned int kVrArraySizeMin = 1U;
const unsigned int kUrlSchemaLengthMax = 255U;
const unsigned int kPackageNameLengthMax = 255U;
const unsigned int kAppIdLengthMax = 40U;
const unsigned int kAppNameLengthMax = 100U;
const unsigned int kLanguageArraySizeMax = 100U;

typedef std::set<std::string> SynonymsSet;
typedef std::map<std::string, SynonymsSet> SynonymsMap;

bool ValidateSynonymsAtLanguage(const smart_objects::SmartObject& language,
                                const std::string& language_name,
                                SynonymsMap& synonyms_map) {
  if (!language[language_name].keyExists(json::vrSynonyms)) {
    SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                 << "'languages.vrSynonyms' doesn't exist");
    return false;
  }
  const smart_objects::SmartArray* synonyms_array =
      language[language_name][json::vrSynonyms].asArray();
  if (!synonyms_array) {
    SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                 << "vrSynonyms is not array.");
    return false;
  }
  const size_t synonyms_array_size = synonyms_array->size();
  if (synonyms_array_size < kVrArraySizeMin) {
    SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                 << "vrSynomyms array has [" << synonyms_array_size
                 << "] size < allowed min size [" << kVrArraySizeMin << "]");
    return false;
  }
  if (synonyms_array_size > kVrArraySizeMax) {
    SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                 << "vrSynomyms array size [" << synonyms_array_size
                 << "] exceeds maximum allowed size [" << kVrArraySizeMax
                 << "]");
    return false;
  }

  for (std::size_t idx = 0; idx < synonyms_array_size; ++idx) {
    const smart_objects::SmartObject& synonym = (*synonyms_array)[idx];
    const std::string vrSynonym = synonym.asString();
    if (vrSynonym.length() > kVrSynonymLengthMax) {
      SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                   << "vrSYnomym item [" << idx << "] exceeds max length ["
                   << vrSynonym.length() << "]>[" << kVrSynonymLengthMax
                   << "]");
      return false;
    }
    if (vrSynonym.length() < kVrSynonymLengthMin) {
      SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                   << "vrSYnomym item [" << idx << "] length ["
                   << vrSynonym.length() << "] is less then min length ["
                   << kVrSynonymLengthMin << "] allowed.");
      return false;
    }
    // Verify duplicates
    SynonymsMap::iterator synonyms_map_iter = synonyms_map.find(language_name);
    if (synonyms_map_iter != synonyms_map.end()) {
      if (!(*synonyms_map_iter).second.insert(vrSynonym).second) {
        SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                     << "vrSYnomym item already defined [" << vrSynonym.c_str()
                     << "] for language [" << language_name << "]");
        return false;
      }
    }
  }
  return true;
}

bool CheckMandatoryParametersPresent(
    const smart_objects::SmartObject& app_data) {
  if (!app_data.keyExists(json::android) && !app_data.keyExists(json::ios)) {
    return false;
  }

  if (app_data.keyExists(json::android) &&
      !app_data[json::android].keyExists(json::packageName)) {
    return false;
  }

  if (app_data.keyExists(json::ios) &&
      !app_data[json::ios].keyExists(json::urlScheme)) {
    return false;
  }

  if (!app_data.keyExists(json::appId)) {
    return false;
  }

  if (!app_data.keyExists(json::name)) {
    return false;
  }

  return true;
}

class QueryAppsDataValidator {
 public:
  QueryAppsDataValidator(smart_objects::SmartObject& object,
                         const ApplicationManager& manager)
      : data_(object), manager_(manager) {}

  bool Validate() {
    SDL_LOG_AUTO_TRACE();
    if (!data_.isValid()) {
      SDL_LOG_ERROR(kQueryAppsValidationFailedPrefix
                    << "QueryApps response is not valid.");
      return false;
    }
    if (!HasResponseKey()) {
      return false;
    }
    return ValidateAppDataAndOsAndLanguagesData();
  }

 private:
  bool HasResponseKey() const {
    if (!data_.keyExists(json::response)) {
      SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                   << "QueryApps response does not contain '" << json::response
                   << "' parameter.");
      return false;
    }
    return true;
  }

  bool ValidateAppDataAndOsAndLanguagesData() {
    smart_objects::SmartArray* objects_array = data_[json::response].asArray();

    if (!objects_array) {
      SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                   << "QueryApps response is not array.");
      return false;
    }

    SynonymsMap synonyms_map;
    bool has_response_valid_application = false;

    smart_objects::SmartArray::iterator applications_iterator =
        objects_array->begin();

    for (; applications_iterator != objects_array->end();) {
      const smart_objects::SmartObject& app_data = *applications_iterator;

      if (!app_data.isValid()) {
        SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                     << "Wrong application data in json file.");
        return false;
      }

      if (!CheckMandatoryParametersPresent(app_data)) {
        SDL_LOG_WARN(
            "Application hasn`t some of mandatory parameters. "
            "Application will be skipped.");

        applications_iterator = objects_array->erase(applications_iterator);
        continue;
      }

      if (!ValidateAppIdAndAppName(app_data)) {
        return false;
      }

      // If we dont have any of android/ios field
      // we skip this json in  CheckMandatoryParametersPresent
      const std::string os_type =
          (app_data.keyExists(json::android)) ? json::android : json::ios;

      // Verify os and dependent languages data
      if (json::ios == os_type) {
        if (app_data[json::ios][json::urlScheme].asString().length() >
            kUrlSchemaLengthMax) {
          SDL_LOG_WARN(
              kQueryAppsValidationFailedPrefix
              << "An urlscheme length exceeds maximum allowed ["
              << app_data[json::ios][json::urlScheme].asString().length()
              << "]>[" << kUrlSchemaLengthMax << "]");
          return false;
        }
      }

      if (json::android == os_type) {
        if (app_data[json::android][json::packageName].asString().length() >
            kPackageNameLengthMax) {
          SDL_LOG_WARN(
              kQueryAppsValidationFailedPrefix
              << "Package name length ["
              << app_data[json::android][json::packageName].asString().length()
              << "] exceeds max length [" << kPackageNameLengthMax
              << "]in json file.");
          return false;
        }
      }

      // Languages verification
      if (!app_data[os_type].keyExists(json::languages)) {
        SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                     << "'languages' doesn't exist");
        return false;
      }
      if (!ValidateLanguages(app_data[os_type][json::languages],
                             synonyms_map)) {
        return false;
      }
      has_response_valid_application = true;
      ++applications_iterator;
    }
    return has_response_valid_application;
  }

  bool ValidateAppIdAndAppName(const smart_objects::SmartObject& app_data) {
    // Verify appid length
    const std::string app_id(app_data[json::appId].asString());
    if (app_id.length() > kAppIdLengthMax) {
      SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                   << "An Object ID length exceeds maximum allowed ["
                   << app_id.length() << "]>[" << kAppIdLengthMax << "]");
      return false;
    }

    // Verify that appid is unique
    if (applications_id_set_.find(app_id) != applications_id_set_.end()) {
      SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                   << "An Object ID is not unigue [" << app_id << "]");
      return false;
    }
    applications_id_set_.insert(app_id);

    // Verify that app is not registered yet
    ApplicationSharedPtr registered_app =
        manager_.application_by_policy_id(app_id);
    if (registered_app) {
      SDL_LOG_INFO("Application with the id: " << app_id
                                               << " is already registered.");
    }
    // And app name length
    const std::string appName(app_data[json::name].asString());
    if (appName.length() > kAppNameLengthMax) {
      SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                   << "Name of application exceeds maximum allowed ["
                   << appName.length() << "]>[" << kAppNameLengthMax << "].");
      return false;
    }
    return true;
  }

  bool ValidateLanguages(const smart_objects::SmartObject& languages,
                         SynonymsMap& synonyms_map) const {
    bool default_language_found = false;
    const size_t languages_array_size = languages.length();
    if (languages_array_size > kLanguageArraySizeMax) {
      SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                   << "'languages' array exceeds max size ["
                   << languages_array_size << "]>[" << kLanguageArraySizeMax
                   << "]");
      return false;
    }
    // Every language has ttsname string and vrsynonyms array
    for (size_t idx = 0; idx < languages_array_size; ++idx) {
      const smart_objects::SmartObject& language = languages.getElement(idx);
      if (smart_objects::SmartType_Map != language.getType()) {
        SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                     << "language is not a map.");
        return false;
      }
      if (language.length() != 1) {
        SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                     << "language map size is not equal 1.");
        return false;
      }
      const std::string language_name = (*language.map_begin()).first;
      if (!language_name.length()) {
        SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                     << "language name is empty");
        return false;
      }
      // Verify default language defined
      if (!(language_name).compare(json::default_)) {
        default_language_found = true;
      }
      // Add set for synonyms' duplicates validation
      if (synonyms_map.find(language_name) == synonyms_map.end()) {
        synonyms_map[language_name] = SynonymsSet();
      }
      // ttsName verification
      if (!language[language_name].keyExists(json::ttsName)) {
        SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                     << "'languages.ttsName' doesn't exist");
        return false;
      }
      const smart_objects::SmartObject& ttsNameObject =
          language[language_name][json::ttsName];
      // ttsName is string
      if (smart_objects::SmartType_String == ttsNameObject.getType()) {
        const std::string ttsName =
            language[language_name][json::ttsName].asString();
        if (ttsName.length() > kTtsNameLengthMax) {
          SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                       << "ttsName string exceeds max length ["
                       << ttsName.length() << "]>[" << kTtsNameLengthMax
                       << "]");
          return false;
        }
      } else {
        SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                     << "ttsName is not the string type.");
        return false;
      }

      if (!ValidateSynonymsAtLanguage(language, language_name, synonyms_map)) {
        return false;
      }
    }
    if (!default_language_found) {
      SDL_LOG_WARN(kQueryAppsValidationFailedPrefix
                   << " 'languages'.default' doesn't exist");
      return false;
    }
    return true;
  }

  smart_objects::SmartObject& data_;
  std::set<std::string> applications_id_set_;
  const ApplicationManager& manager_;

  DISALLOW_COPY_AND_ASSIGN(QueryAppsDataValidator);
};
}  // namespace

namespace commands {

namespace custom_str = utils::custom_string;

uint32_t SystemRequest::index = 0;

const std::string kSYNC = "SYNC";
const std::string kIVSU = "IVSU";

SystemRequest::SystemRequest(
    const application_manager::commands::MessageSharedPtr& message,
    ApplicationManager& application_manager,
    app_mngr::rpc_service::RPCService& rpc_service,
    app_mngr::HMICapabilities& hmi_capabilities,
    policy::PolicyHandlerInterface& policy_handler)
    : RequestFromMobileImpl(message,
                            application_manager,
                            rpc_service,
                            hmi_capabilities,
                            policy_handler) {}

SystemRequest::~SystemRequest() {}

void SystemRequest::Run() {
  SDL_LOG_AUTO_TRACE();

  ApplicationSharedPtr application =
      application_manager_.application(connection_key());

  if (application.use_count() == 0) {
    SDL_LOG_ERROR("NULL pointer");
    SendResponse(false, mobile_apis::Result::APPLICATION_NOT_REGISTERED);
    return;
  }
  const mobile_apis::RequestType::eType request_type =
      static_cast<mobile_apis::RequestType::eType>(
          (*message_)[strings::msg_params][strings::request_type].asInt());

  const policy::PolicyHandlerInterface& policy_handler = policy_handler_;

  const std::string stringified_request_type =
      rpc::policy_table_interface_base::EnumToJsonString(
          static_cast<rpc::policy_table_interface_base::RequestType>(
              request_type));

  if (!policy_handler.IsRequestTypeAllowed(
          application->device(), application->policy_app_id(), request_type)) {
    SDL_LOG_ERROR("RequestType " << stringified_request_type
                                 << " is DISALLOWED by policies");
    SendResponse(false, mobile_apis::Result::DISALLOWED);
    return;
  }
  SDL_LOG_TRACE("RequestType " << stringified_request_type << " is ALLOWED");

  const bool request_subtype_present =
      (*message_)[strings::msg_params].keyExists(strings::request_subtype);
  if (request_subtype_present) {
    const std::string request_subtype =
        (*message_)[strings::msg_params][strings::request_subtype].asString();
    if (!policy_handler.IsRequestSubTypeAllowed(application->policy_app_id(),
                                                request_subtype)) {
      SDL_LOG_ERROR("Request subtype: " << request_subtype
                                        << " is DISALLOWED by policies");
      SendResponse(false, mobile_apis::Result::DISALLOWED);
      return;
    }
    SDL_LOG_TRACE("Request subtype: " << request_subtype << " is ALLOWED");
  }

  std::string file_name;
  if ((*message_)[strings::msg_params].keyExists(strings::file_name)) {
    file_name = (*message_)[strings::msg_params][strings::file_name].asString();
  } else {
    file_name = kSYNC;
  }

  if (!CheckSyntax(file_name)) {
    SDL_LOG_ERROR("Incoming request contains \t\n \\t \\n or whitespace");
    SendResponse(false, mobile_apis::Result::INVALID_DATA);
    return;
  }

  const uint16_t query_apps_min_version = 4;

  if (mobile_apis::RequestType::QUERY_APPS == request_type &&
      application_manager_.get_settings().max_supported_protocol_version() <
          query_apps_min_version) {
    SendResponse(false, mobile_apis::Result::UNSUPPORTED_RESOURCE);
    return;
  }

  if (!file_system::IsFileNameValid(file_name) &&
      mobile_apis::RequestType::ICON_URL != request_type) {
    const std::string err_msg = "Sync file name contains forbidden symbols.";
    SDL_LOG_ERROR(err_msg);
    SendResponse(false, mobile_apis::Result::INVALID_DATA, err_msg.c_str());
    return;
  }

  const bool is_system_file = std::string::npos != file_name.find(kSYNC) ||
                              std::string::npos != file_name.find(kIVSU);

  // to avoid override existing file
  if (is_system_file) {
    const uint8_t max_size = 255;
    char buf[max_size] = {'\0'};
    snprintf(buf, max_size - 1, "%d%s", index++, file_name.c_str());
    file_name = buf;
  }

  std::vector<uint8_t> binary_data;
  std::string binary_data_folder;
  if ((*message_)[strings::params].keyExists(strings::binary_data)) {
    binary_data = (*message_)[strings::params][strings::binary_data].asBinary();
    if (mobile_apis::RequestType::ICON_URL == request_type) {
      binary_data_folder =
          application_manager_.get_settings().app_icons_folder();
      // Use the URL file name to identify the policy id.
      // Save the icon file with the policy id as the name.
      file_name = application_manager_.PolicyIDByIconUrl(file_name);
      if (file_name.empty()) {
        const std::string err_msg = "Invalid file name";
        SendResponse(false, mobile_apis::Result::INVALID_DATA, err_msg.c_str());
        return;
      }
      SDL_LOG_DEBUG("Got ICON_URL Request. File name: " << file_name);
    } else {
      binary_data_folder =
          application_manager_.get_settings().system_files_path();
    }
  } else {
    binary_data_folder =
        application_manager_.get_settings().app_storage_folder();
    binary_data_folder += "/";
    binary_data_folder += application->folder_name();
    binary_data_folder += "/";
  }

  std::string file_dst_path =
      application_manager_.get_settings().system_files_path();
  file_dst_path += "/";
  file_dst_path += file_name;

  if ((*message_)[strings::params].keyExists(strings::binary_data)) {
    SDL_LOG_DEBUG(
        "Binary data is present. Trying to save it to: " << binary_data_folder);
    if (mobile_apis::Result::SUCCESS !=
        (application_manager_.SaveBinary(
            binary_data, binary_data_folder, file_name, 0))) {
      SDL_LOG_DEBUG("Binary data can't be saved.");
      SendResponse(false, mobile_apis::Result::GENERIC_ERROR);
      return;
    }
  } else {
    std::string app_full_file_path = binary_data_folder;
    app_full_file_path += file_name;

    SDL_LOG_DEBUG("Binary data is not present. Trying to find file "
                  << file_name << " within previously saved app file in "
                  << binary_data_folder);

    const application_manager::AppFile* file =
        application->GetFile(app_full_file_path);
    if (!file || !file->is_download_complete ||
        !file_system::MoveFile(app_full_file_path, file_dst_path)) {
      SDL_LOG_DEBUG("Binary data not found.");
      SendResponse(false, mobile_apis::Result::REJECTED);
      return;
    }
    processing_file_ = file_dst_path;
  }

  SDL_LOG_DEBUG("Binary data ok.");

  if (mobile_apis::RequestType::ICON_URL == request_type) {
    application_manager_.SetIconFileFromSystemRequest(file_name);
    SendResponse(true, mobile_apis::Result::SUCCESS);
    return;
  }

  if (mobile_apis::RequestType::HTTP == request_type &&
      (*message_)[strings::msg_params].keyExists(strings::file_name)) {
    const std::string& file =
        (*message_)[strings::msg_params][strings::file_name].asString();
    policy_handler_.ReceiveMessageFromSDK(file, binary_data);
    SendResponse(true, mobile_apis::Result::SUCCESS);
    return;
  } else if (mobile_apis::RequestType::QUERY_APPS == request_type) {
    using namespace ns_smart_device_link::ns_json_handler::formatters;
    application_manager_.OnQueryAppsRequest(application->device());

    utils::JsonReader reader;
    std::string json(binary_data.begin(), binary_data.end());
    Json::Value root;

    if (!reader.parse(json, &root)) {
      SDL_LOG_DEBUG("Unable to parse query_app json file. ");
      return;
    }

    smart_objects::SmartObject sm_object;

    CFormatterJsonBase::jsonValueToObj(root, sm_object);

    if (!ValidateQueryAppData(sm_object)) {
      SendResponse(false, mobile_apis::Result::GENERIC_ERROR);
      return;
    }

    application_manager_.ProcessQueryApp(sm_object, connection_key());
    SendResponse(true, mobile_apis::Result::SUCCESS);
    return;
  }

  smart_objects::SmartObject msg_params =
      smart_objects::SmartObject(smart_objects::SmartType_Map);
  if (std::string::npos != file_name.find(kIVSU)) {
    msg_params[strings::file_name] = file_name;
  } else {
    msg_params[strings::file_name] = file_dst_path;
  }

  // expected int, mandatory=true, all Policies flow (HTTP,Proprietary,External)
  msg_params[strings::app_id] = application->hmi_app_id();

  msg_params[strings::request_type] =
      (*message_)[strings::msg_params][strings::request_type];
  if (request_subtype_present) {
    msg_params[strings::request_subtype] =
        (*message_)[strings::msg_params][strings::request_subtype];
  }
  StartAwaitForInterface(HmiInterfaces::HMI_INTERFACE_BasicCommunication);
  SendHMIRequest(hmi_apis::FunctionID::BasicCommunication_SystemRequest,
                 &msg_params,
                 true);
}

void SystemRequest::on_event(const event_engine::Event& event) {
  SDL_LOG_AUTO_TRACE();
  using namespace helpers;

  const smart_objects::SmartObject& message = event.smart_object();

  switch (event.id()) {
    case hmi_apis::FunctionID::BasicCommunication_SystemRequest: {
      EndAwaitForInterface(HmiInterfaces::HMI_INTERFACE_BasicCommunication);
      mobile_apis::Result::eType result_code =
          GetMobileResultCode(static_cast<hmi_apis::Common_Result::eType>(
              message[strings::params][hmi_response::code].asUInt()));

      const bool result = Compare<mobile_api::Result::eType, EQ, ONE>(
          result_code,
          mobile_api::Result::SUCCESS,
          mobile_api::Result::WARNINGS);

      ApplicationSharedPtr application =
          application_manager_.application(connection_key());

      if (application.use_count() == 0) {
        SDL_LOG_ERROR("NULL pointer");
        return;
      }

      if (!processing_file_.empty()) {
        file_system::DeleteFile(processing_file_);
        processing_file_.clear();
      }

      SendResponse(result, result_code, NULL, &(message[strings::msg_params]));
      break;
    }
    default: {
      SDL_LOG_ERROR("Received unknown event " << event.id());
      return;
    }
  }
}

bool SystemRequest::ValidateQueryAppData(
    smart_objects::SmartObject& data) const {
  if (!data.isValid()) {
    SDL_LOG_ERROR(kQueryAppsValidationFailedPrefix
                  << "QueryApps response is not valid.");
    return false;
  }
  if (!data.keyExists(json::response)) {
    SDL_LOG_ERROR(kQueryAppsValidationFailedPrefix
                  << "QueryApps response does not contain '" << json::response
                  << "' parameter.");
    return false;
  }

  QueryAppsDataValidator validator(data, application_manager_);
  return validator.Validate();
}

}  // namespace commands

}  // namespace sdl_rpc_plugin
