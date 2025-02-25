/*
 Copyright (c) 2015, Ford Motor Company
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

#include "application_manager/state_controller_impl.h"

#include <tuple>

#include "application_manager/rpc_service.h"
#include "application_manager/usage_statistics.h"
#include "connection_handler/connection_handler.h"
#include "utils/helpers.h"

namespace application_manager {

SDL_CREATE_LOG_VARIABLE("StateControllerImpl")

namespace {
bool IsStateChanged(const HmiState& old_state, const HmiState& new_state) {
  return std::make_tuple(old_state.hmi_level(),
                         old_state.audio_streaming_state(),
                         old_state.video_streaming_state(),
                         old_state.system_context()) !=
         std::make_tuple(new_state.hmi_level(),
                         new_state.audio_streaming_state(),
                         new_state.video_streaming_state(),
                         new_state.system_context());
}
}  // unnamed namespace

StateControllerImpl::StateControllerImpl(ApplicationManager& app_mngr)
    : EventObserver(app_mngr.event_dispatcher())
    , app_mngr_(app_mngr)
    , postponed_activation_controller_() {
  subscribe_on_event(hmi_apis::FunctionID::BasicCommunication_OnAppActivated);
  subscribe_on_event(hmi_apis::FunctionID::BasicCommunication_OnAppDeactivated);
  subscribe_on_event(hmi_apis::FunctionID::TTS_Started);
  subscribe_on_event(hmi_apis::FunctionID::TTS_Stopped);
  subscribe_on_event(hmi_apis::FunctionID::VR_Started);
  subscribe_on_event(hmi_apis::FunctionID::VR_Stopped);
  subscribe_on_event(hmi_apis::FunctionID::BasicCommunication_OnEventChanged);
}

void StateControllerImpl::SetRegularState(ApplicationSharedPtr app,
                                          const WindowID window_id,
                                          HmiStatePtr state,
                                          const bool request_hmi_state_change) {
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(app);
  DCHECK_OR_RETURN_VOID(state);
  DCHECK_OR_RETURN_VOID(HmiState::STATE_ID_REGULAR == state->state_id());

  SDL_LOG_DEBUG("Set window #" << window_id << " regular state " << *state);

  if (state->hmi_level() == mobile_apis::HMILevel::INVALID_ENUM ||
      state->audio_streaming_state() ==
          mobile_apis::AudioStreamingState::INVALID_ENUM ||
      state->video_streaming_state() ==
          mobile_apis::VideoStreamingState::INVALID_ENUM ||
      state->system_context() == mobile_apis::SystemContext::INVALID_ENUM) {
    SDL_LOG_ERROR("Got invalid state");
    return;
  }

  const bool app_is_resuming = app->is_resuming();
  const bool is_resumption_allowed = IsResumptionAllowed(app, state);

  if (app_is_resuming && !is_resumption_allowed) {
    return;
  }

  HmiStatePtr resolved_state = ResolveHmiState(app, state);
  if (!resolved_state) {
    state->set_state_id(HmiState::STATE_ID_POSTPONED);
    app->SetPostponedState(window_id, state);
    return;
  }

  SDL_LOG_DEBUG("Resolved state: " << *resolved_state);
  const hmi_apis::Common_HMILevel::eType hmi_level =
      static_cast<hmi_apis::Common_HMILevel::eType>(
          resolved_state->hmi_level());

  if (request_hmi_state_change) {
    const int64_t result = RequestHMIStateChange(app, hmi_level, true);
    if (-1 != result) {
      const uint32_t corr_id = static_cast<uint32_t>(result);
      subscribe_on_event(
          hmi_apis::Common_HMILevel::NONE == hmi_level
              ? hmi_apis::FunctionID::BasicCommunication_CloseApplication
              : hmi_apis::FunctionID::BasicCommunication_ActivateApp,
          corr_id);
      waiting_for_response_[app->app_id()] = resolved_state;
      app_mngr_.set_application_id(corr_id, app->hmi_app_id());
      return;
    }
    SDL_LOG_ERROR("Unable to send BC.ActivateApp");
    return;
  }
  ApplyRegularState(app, window_id, resolved_state);
}

void StateControllerImpl::SetRegularState(
    ApplicationSharedPtr app,
    const WindowID window_id,
    const mobile_apis::HMILevel::eType hmi_level,
    const mobile_apis::AudioStreamingState::eType audio_state,
    const mobile_apis::VideoStreamingState::eType video_state,
    const bool request_hmi_state_change) {
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(app);
  HmiStatePtr prev_regular = app->RegularHmiState(window_id);
  DCHECK_OR_RETURN_VOID(prev_regular);
  HmiStatePtr hmi_state =
      CreateHmiState(app, HmiState::StateID::STATE_ID_REGULAR);
  DCHECK_OR_RETURN_VOID(hmi_state);
  hmi_state->set_hmi_level(hmi_level);
  hmi_state->set_audio_streaming_state(audio_state);
  hmi_state->set_video_streaming_state(video_state);
  hmi_state->set_system_context(prev_regular->system_context());
  hmi_state->set_window_type(prev_regular->window_type());
  SetRegularState(app, window_id, hmi_state, request_hmi_state_change);
}

void StateControllerImpl::SetRegularState(
    ApplicationSharedPtr app,
    const WindowID window_id,
    const mobile_apis::HMILevel::eType hmi_level,
    const bool request_hmi_state_change) {
  using namespace mobile_apis;
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(app);

  HmiStatePtr prev_regular = app->RegularHmiState(window_id);
  DCHECK_OR_RETURN_VOID(prev_regular);

  const HmiStatePtr hmi_state =
      CreateHmiState(app, HmiState::StateID::STATE_ID_REGULAR);

  DCHECK_OR_RETURN_VOID(hmi_state);
  hmi_state->set_hmi_level(hmi_level);
  hmi_state->set_audio_streaming_state(CalcAudioState(app, hmi_level));
  hmi_state->set_video_streaming_state(CalcVideoState(app, hmi_level));
  hmi_state->set_system_context(SystemContext::SYSCTXT_MAIN);
  hmi_state->set_window_type(prev_regular->window_type());
  SetRegularState(app, window_id, hmi_state, request_hmi_state_change);
}

void StateControllerImpl::SetRegularState(
    ApplicationSharedPtr app,
    const WindowID window_id,
    const mobile_apis::HMILevel::eType hmi_level,
    const mobile_apis::AudioStreamingState::eType audio_state,
    const mobile_apis::VideoStreamingState::eType video_state,
    const mobile_apis::SystemContext::eType system_context,
    const bool request_hmi_state_change) {
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(app);

  HmiStatePtr prev_regular = app->RegularHmiState(window_id);
  DCHECK_OR_RETURN_VOID(prev_regular);

  HmiStatePtr hmi_state =
      CreateHmiState(app, HmiState::StateID::STATE_ID_REGULAR);
  DCHECK_OR_RETURN_VOID(hmi_state);

  hmi_state->set_hmi_level(hmi_level);
  hmi_state->set_audio_streaming_state(audio_state);
  hmi_state->set_video_streaming_state(video_state);
  hmi_state->set_system_context(system_context);
  hmi_state->set_window_type(prev_regular->window_type());
  SetRegularState(app, window_id, hmi_state, request_hmi_state_change);
}

void StateControllerImpl::SetRegularState(
    ApplicationSharedPtr app,
    const WindowID window_id,
    const mobile_apis::HMILevel::eType hmi_level) {
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(app);
  HmiStatePtr prev_state = app->RegularHmiState(window_id);
  HmiStatePtr hmi_state =
      CreateHmiState(app, HmiState::StateID::STATE_ID_REGULAR);
  DCHECK_OR_RETURN_VOID(hmi_state);
  hmi_state->set_hmi_level(hmi_level);
  hmi_state->set_audio_streaming_state(CalcAudioState(app, hmi_level));
  hmi_state->set_video_streaming_state(CalcVideoState(app, hmi_level));
  hmi_state->set_system_context(prev_state
                                    ? prev_state->system_context()
                                    : mobile_apis::SystemContext::SYSCTXT_MAIN);
  SetRegularState(app, window_id, hmi_state);
}

void StateControllerImpl::SetRegularState(
    ApplicationSharedPtr app,
    const WindowID window_id,
    const mobile_apis::SystemContext::eType system_context) {
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(app);
  HmiStatePtr prev_regular = app->RegularHmiState(window_id);
  DCHECK_OR_RETURN_VOID(prev_regular);
  HmiStatePtr hmi_state =
      CreateHmiState(app, HmiState::StateID::STATE_ID_REGULAR);
  DCHECK_OR_RETURN_VOID(hmi_state);
  hmi_state->set_hmi_level(prev_regular->hmi_level());
  hmi_state->set_audio_streaming_state(
      CalcAudioState(app, prev_regular->hmi_level()));
  hmi_state->set_video_streaming_state(
      CalcVideoState(app, prev_regular->hmi_level()));
  hmi_state->set_system_context(system_context);
  hmi_state->set_window_type(prev_regular->window_type());
  SetRegularState(app, window_id, hmi_state, false);
}

void StateControllerImpl::SetRegularState(
    ApplicationSharedPtr app,
    const WindowID window_id,
    const mobile_apis::AudioStreamingState::eType audio_state,
    const mobile_apis::VideoStreamingState::eType video_state) {
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(app);
  HmiStatePtr prev_state = app->RegularHmiState(window_id);
  DCHECK_OR_RETURN_VOID(prev_state);
  HmiStatePtr hmi_state =
      CreateHmiState(app, HmiState::StateID::STATE_ID_REGULAR);
  DCHECK_OR_RETURN_VOID(hmi_state);
  hmi_state->set_hmi_level(prev_state->hmi_level());
  hmi_state->set_audio_streaming_state(audio_state);
  hmi_state->set_video_streaming_state(video_state);
  hmi_state->set_system_context(prev_state->system_context());
  hmi_state->set_window_type(prev_state->window_type());
  SetRegularState(app, window_id, hmi_state, false);
}

void StateControllerImpl::SetRegularState(ApplicationSharedPtr app,
                                          const WindowID window_id,
                                          HmiStatePtr state) {
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(app);
  DCHECK_OR_RETURN_VOID(state);

  // SDL should send BC.ActivateApp for main window during resumption
  if (mobile_apis::PredefinedWindows::DEFAULT_WINDOW == window_id &&
      mobile_apis::HMILevel::HMI_FULL == state->hmi_level()) {
    SetRegularState(app, window_id, state, true);
  } else {
    SetRegularState(app, window_id, state, false);
  }
}

namespace {

/**
 * @brief IsStreamableHMILevel checks whether the HMI level
 * supports audio/video streaming.
 * @param val HMI level
 * @return true if streamable, false otherwise
 */
bool IsStreamableHMILevel(mobile_apis::HMILevel::eType val) {
  using namespace helpers;
  return Compare<mobile_apis::HMILevel::eType, EQ, ONE>(
      val, mobile_apis::HMILevel::HMI_FULL, mobile_apis::HMILevel::HMI_LIMITED);
}

/**
 * @brief IsSameAudioAppType checks whether the both applications:
 * 1) belongs to exact HMI type that may stream audio without
 * 'isMediaApplication' parameter set to true ('NAVIGATION', 'COMMUNICATION')
 * 2) belongs to other HMI types with parameter 'isMediaApplication'
 * is set to true.
 * 3) are not media.
 * @param app1 1st application
 * @param app2 2nd application
 * @return true if audio applications have same application HMI type
 */
bool IsSameAudioAppType(const Application& app1, const Application& app2) {
  const auto is_only_media_app_type = [](const Application& app) {
    return app.is_media_application() && !app.is_navi() &&
           !app.is_voice_communication_supported();
  };

  const bool both_navi = app1.is_navi() && app2.is_navi();
  const bool both_vc = app1.is_voice_communication_supported() &&
                       app2.is_voice_communication_supported();
  const bool both_media =
      is_only_media_app_type(app1) && is_only_media_app_type(app2);
  const bool both_other =
      !app1.IsAudioApplication() && !app2.IsAudioApplication();
  const bool same_app_audio_type =
      both_navi || both_vc || both_media || both_other;

  return same_app_audio_type;
}
}  // unnamed namespace

void StateControllerImpl::HmiLevelConflictResolver::operator()(
    ApplicationSharedPtr app_to_resolve) {
  DCHECK_OR_RETURN_VOID(app_to_resolve);
  DCHECK_OR_RETURN_VOID(state_ctrl_);
  DCHECK_OR_RETURN_VOID(applied_);
  DCHECK_OR_RETURN_VOID(state_);

  if (applied_ == app_to_resolve) {
    // Same app. Nothing to resolve
    return;
  }

  const HmiStatePtr state_to_resolve =
      app_to_resolve->RegularHmiState(window_id_);
  DCHECK_OR_RETURN_VOID(state_to_resolve);

  // If applied HMI state is FULL:
  // - all NOT audio/video applications become BACKGROUND
  // - all audio/video applications with other app type
  //  (navi, vc, media, projection) in FULL become LIMITED
  // - all audio/video applications with the same app type become BACKGROUND
  //
  // If applied HMI state is LIMITED:
  // - all NOT audio/video applications saves their HMI states
  // - all applications with the other app types saves their HMI states
  // - all audio/video applications with the same app type become BACKGROUND

  if (!IsStreamableHMILevel(state_->hmi_level())) {
    SDL_LOG_DEBUG("Applied for app " << applied_->app_id() << " HMI level "
                                     << state_->hmi_level()
                                     << " is NOT streamable. Exit.");
    return;
  }

  if (!IsStreamableHMILevel(state_to_resolve->hmi_level())) {
    SDL_LOG_DEBUG("To resolve app " << app_to_resolve->app_id() << " HMI level "
                                    << state_to_resolve->hmi_level()
                                    << " is NOT streamable. Exit.");
    return;
  }

  // Applied app constants
  const auto applied_hmi_level = state_->hmi_level();

  const bool applied_grabs_video =
      IsStreamableHMILevel(applied_hmi_level) && applied_->IsVideoApplication();

  // App to resolve constants
  const auto to_resolve_hmi_level = state_to_resolve->hmi_level();

  const bool to_resolve_grabs_audio =
      IsStreamableHMILevel(to_resolve_hmi_level) &&
      app_to_resolve->IsAudioApplication();

  const bool to_resolve_grabs_video =
      IsStreamableHMILevel(to_resolve_hmi_level) &&
      app_to_resolve->IsVideoApplication();

  // Compatibility constants
  const bool same_app_audio_type =
      IsSameAudioAppType(*applied_, *app_to_resolve);

  // Result variables
  mobile_apis::VideoStreamingState::eType result_video_state =
      mobile_apis::VideoStreamingState::NOT_STREAMABLE;
  mobile_apis::AudioStreamingState::eType result_audio_state =
      mobile_apis::AudioStreamingState::NOT_AUDIBLE;

  if (to_resolve_grabs_audio && !same_app_audio_type) {
    result_audio_state = mobile_apis::AudioStreamingState::AUDIBLE;
  }

  if (to_resolve_grabs_video && !applied_grabs_video) {
    result_video_state = mobile_apis::VideoStreamingState::STREAMABLE;
  }

  mobile_apis::HMILevel::eType result_hmi_level;

  using namespace helpers;
  if (mobile_apis::VideoStreamingState::STREAMABLE == result_video_state ||
      Compare<mobile_apis::AudioStreamingState::eType, EQ, ONE>(
          result_audio_state,
          mobile_apis::AudioStreamingState::AUDIBLE,
          mobile_apis::AudioStreamingState::ATTENUATED)) {
    result_hmi_level =
        mobile_apis::PredefinedWindows::DEFAULT_WINDOW == window_id_ &&
                applied_->IsFullscreen()
            ? mobile_apis::HMILevel::HMI_LIMITED
            : to_resolve_hmi_level;
  } else {
    result_hmi_level =
        mobile_apis::HMILevel::HMI_FULL == to_resolve_hmi_level &&
                mobile_apis::HMILevel::HMI_FULL != applied_hmi_level
            ? to_resolve_hmi_level
            : mobile_apis::HMILevel::HMI_BACKGROUND;
  }

  if (std::make_tuple(to_resolve_hmi_level,
                      state_to_resolve->audio_streaming_state(),
                      state_to_resolve->video_streaming_state()) !=
      std::make_tuple(
          result_hmi_level, result_audio_state, result_video_state)) {
    SDL_LOG_DEBUG("Application " << app_to_resolve->app_id() << " window "
                                 << window_id_ << " will change state to: "
                                 << "HMI level " << to_resolve_hmi_level
                                 << " --> " << result_hmi_level << ", audio "
                                 << state_to_resolve->audio_streaming_state()
                                 << " --> " << result_audio_state << ", video "
                                 << state_to_resolve->video_streaming_state()
                                 << " --> " << result_video_state);
    state_ctrl_->SetupRegularHmiState(app_to_resolve,
                                      window_id_,
                                      result_hmi_level,
                                      result_audio_state,
                                      result_video_state);
  } else {
    SDL_LOG_DEBUG("Application " << app_to_resolve->app_id() << " window "
                                 << window_id_ << " will NOT change HMI level");
  }
}

HmiStatePtr StateControllerImpl::ResolveHmiState(ApplicationSharedPtr app,
                                                 HmiStatePtr state) const {
  using namespace mobile_apis;
  SDL_LOG_AUTO_TRACE();
  SDL_LOG_DEBUG("State to resolve: " << *state);

  HmiStatePtr available_state =
      CreateHmiState(app, HmiState::StateID::STATE_ID_REGULAR);
  DCHECK_OR_RETURN(available_state, HmiStatePtr());
  available_state->set_hmi_level(state->hmi_level());
  available_state->set_window_type(state->window_type());
  available_state->set_audio_streaming_state(state->audio_streaming_state());
  available_state->set_video_streaming_state(state->video_streaming_state());
  available_state->set_system_context(state->system_context());

  if (app->is_resuming()) {
    HMILevel::eType available_level =
        GetAvailableHmiLevel(app, state->hmi_level());
    available_state->set_hmi_level(available_level);
    available_state->set_audio_streaming_state(
        CalcAudioState(app, available_level));
    available_state->set_video_streaming_state(
        CalcVideoState(app, available_level));
  }
  return IsStateAvailable(app, available_state) ? available_state
                                                : HmiStatePtr();
}

bool StateControllerImpl::IsResumptionAllowed(ApplicationSharedPtr app,
                                              HmiStatePtr state) const {
  SDL_LOG_AUTO_TRACE();
  using namespace mobile_apis;
  if (!app->is_resuming() || !IsStreamableHMILevel(state->hmi_level())) {
    SDL_LOG_DEBUG("Application is not in resuming mode.");
    return true;
  }

  if (IsTempStateActive(HmiState::StateID::STATE_ID_AUDIO_SOURCE) &&
      app->is_media_application()) {
    SDL_LOG_DEBUG("Resumption for media app is not allowed. "
                  << "AUDIO_SOURCE event is active");
    return false;
  }

  const bool is_navi_app = app->is_navi();
  const bool is_mob_projection_app = app->mobile_projection_enabled();
  const bool is_wep_app = app->webengine_projection_enabled();

  if (IsTempStateActive(HmiState::StateID::STATE_ID_EMBEDDED_NAVI) &&
      (is_navi_app || is_mob_projection_app || is_wep_app)) {
    SDL_LOG_DEBUG("Resumption for navi and projection apps is not allowed. "
                  << "EMBEDDED_NAVI event is active");
    return false;
  }

  return true;
}

mobile_apis::HMILevel::eType StateControllerImpl::GetAvailableHmiLevel(
    ApplicationSharedPtr app,
    mobile_apis::HMILevel::eType desired_hmi_level) const {
  SDL_LOG_AUTO_TRACE();

  mobile_apis::HMILevel::eType result = desired_hmi_level;

  if (!IsStreamableHMILevel(desired_hmi_level)) {
    return result;
  }

  const bool is_audio_app = app->IsAudioApplication();
  const bool does_audio_app_with_same_type_exist =
      app_mngr_.IsAppTypeExistsInFullOrLimited(app);

  if (mobile_apis::HMILevel::HMI_LIMITED == desired_hmi_level) {
    if (!is_audio_app || does_audio_app_with_same_type_exist) {
      SDL_LOG_DEBUG(
          "Not audio application trying to resume in limited or "
          "audio application with the same type active");
      result = app_mngr_.GetDefaultHmiLevel(app);
    }
    return result;
  }

  ApplicationConstSharedPtr active_app = app_mngr_.active_application();
  const bool is_active_app_exist =
      (active_app.use_count() != 0) && active_app->app_id() != app->app_id();
  if (is_audio_app) {
    if (does_audio_app_with_same_type_exist) {
      SDL_LOG_DEBUG("Audio application with the same type active");
      result = app_mngr_.GetDefaultHmiLevel(app);
    } else if (is_active_app_exist) {
      result = mobile_apis::HMILevel::HMI_LIMITED;
    } else if (app->is_navi() &&
               IsTempStateActive(HmiState::StateID::STATE_ID_AUDIO_SOURCE)) {
      SDL_LOG_DEBUG(
          "Navigation app will be resumed to LIMITED, "
          "because of AUDIO_SOURCE ia active.");
      result = mobile_apis::HMILevel::HMI_LIMITED;
    } else if (app->is_media_application() &&
               IsTempStateActive(HmiState::StateID::STATE_ID_EMBEDDED_NAVI)) {
      SDL_LOG_DEBUG(
          "Media app will be resumed to LIMITED, "
          "because of EMBEDDED_NAVI is active.");
      result = mobile_apis::HMILevel::HMI_LIMITED;
    }
  } else if (is_active_app_exist) {
    result = app_mngr_.GetDefaultHmiLevel(app);
  }

  return result;
}

bool StateControllerImpl::IsStateAvailable(ApplicationSharedPtr app,
                                           HmiStatePtr state) const {
  using namespace mobile_apis;
  SDL_LOG_AUTO_TRACE();
  SDL_LOG_DEBUG("Checking state: " << *state);

  if (app->is_resuming()) {
    return IsStateAvailableForResumption(app, state);
  }

  if (HMILevel::HMI_FULL == state->hmi_level()) {
    if (IsTempStateActive(HmiState::StateID::STATE_ID_AUDIO_SOURCE) &&
        app->is_media_application()) {
      SDL_LOG_DEBUG("Media app is not allowed to activate due"
                    << " to AUDIO_SOURCE event is active");
      return false;
    }

    if (IsTempStateActive(HmiState::StateID::STATE_ID_EMBEDDED_NAVI) &&
        app->is_navi()) {
      SDL_LOG_DEBUG("Navi app is not allowed to activate due"
                    << " to EMBEDDED_NAVI event is active");
      return false;
    }
  }

  SDL_LOG_DEBUG("Requested state is available");
  return true;
}

bool StateControllerImpl::IsStateAvailableForResumption(
    ApplicationSharedPtr app, HmiStatePtr state) const {
  SDL_LOG_AUTO_TRACE();
  using namespace mobile_apis;

  if (!app->is_resuming() || !IsStreamableHMILevel(state->hmi_level())) {
    SDL_LOG_DEBUG("Application is not in resuming mode."
                  << " Requested state is available");
    return true;
  }

  if (IsTempStateActive(HmiState::StateID::STATE_ID_VR_SESSION) ||
      IsTempStateActive(HmiState::StateID::STATE_ID_SAFETY_MODE)) {
    SDL_LOG_DEBUG("Requested state is not available. "
                  << "VR session or emergency event is active");
    return false;
  }
  if (IsTempStateActive(HmiState::StateID::STATE_ID_PHONE_CALL) &&
      app->is_media_application()) {
    SDL_LOG_DEBUG("Requested state for media application "
                  << "is not available. Phone call is active");
    return false;
  }
  if (IsTempStateActive(HmiState::StateID::STATE_ID_DEACTIVATE_HMI)) {
    SDL_LOG_DEBUG("Requested state is not available. "
                  << "Deactivate HMI event is active");
    return false;
  }

  SDL_LOG_DEBUG("Requested state is available");
  return true;
}

void StateControllerImpl::SetupRegularHmiState(ApplicationSharedPtr app,
                                               const WindowID window_id,
                                               HmiStatePtr state) {
  using namespace mobile_apis;
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(state);
  SDL_LOG_DEBUG("Setup window #" << window_id << " regular state: " << *state);
  HmiStatePtr curr_state = app->CurrentHmiState(window_id);
  HmiStatePtr old_state =
      CreateHmiState(app, HmiState::StateID::STATE_ID_REGULAR);
  DCHECK_OR_RETURN_VOID(old_state);
  old_state->set_hmi_level(curr_state->hmi_level());
  old_state->set_audio_streaming_state(curr_state->audio_streaming_state());
  old_state->set_video_streaming_state(curr_state->video_streaming_state());
  old_state->set_system_context(curr_state->system_context());
  old_state->set_window_type(curr_state->window_type());
  app->SetRegularState(window_id, state);

  if (mobile_apis::PredefinedWindows::DEFAULT_WINDOW == window_id &&
      HMILevel::HMI_LIMITED == state->hmi_level() && app->is_resuming()) {
    SDL_LOG_DEBUG("Resuming to LIMITED level. "
                  << "Send OnResumeAudioSource notification");
    MessageHelper::SendOnResumeAudioSourceToHMI(app->app_id(), app_mngr_);
  }

  HmiStatePtr new_state = app->CurrentHmiState(window_id);
  OnStateChanged(app, window_id, old_state, new_state);
}

void StateControllerImpl::SetupRegularHmiState(
    ApplicationSharedPtr app,
    const WindowID window_id,
    const mobile_apis::HMILevel::eType hmi_level,
    const mobile_apis::AudioStreamingState::eType audio_state,
    const mobile_apis::VideoStreamingState::eType video_state) {
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(app);
  HmiStatePtr prev_state = app->RegularHmiState(window_id);
  DCHECK_OR_RETURN_VOID(prev_state);
  HmiStatePtr new_state =
      CreateHmiState(app, HmiState::StateID::STATE_ID_REGULAR);
  DCHECK_OR_RETURN_VOID(new_state);
  new_state->set_hmi_level(hmi_level);
  new_state->set_audio_streaming_state(audio_state);
  new_state->set_video_streaming_state(video_state);
  new_state->set_system_context(prev_state->system_context());
  new_state->set_window_type(prev_state->window_type());
  SetupRegularHmiState(app, window_id, new_state);
}

void StateControllerImpl::ApplyRegularState(ApplicationSharedPtr app,
                                            const WindowID window_id,
                                            HmiStatePtr state) {
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(app);
  DCHECK_OR_RETURN_VOID(state);
  DCHECK_OR_RETURN_VOID(state->state_id() == HmiState::STATE_ID_REGULAR);
  SDL_LOG_DEBUG("Applying to app " << app->app_id() << " window #" << window_id
                                   << " state " << *state);
  SetupRegularHmiState(app, window_id, state);

  if (mobile_apis::PredefinedWindows::DEFAULT_WINDOW != window_id) {
    SDL_LOG_DEBUG("No need to resolve conflicts for a widget");
    return;
  }

  SDL_LOG_DEBUG("Resolving HMI level conflicts for app " << app->app_id());
  ForEachApplication(HmiLevelConflictResolver(app, window_id, state, this));
}

void StateControllerImpl::UpdateAppWindowsStreamingState(
    ApplicationSharedPtr app, HmiStatePtr state) {
  SDL_LOG_AUTO_TRACE();
  const auto window_ids = app->GetWindowIds();
  for (auto window_id : window_ids) {
    HmiStatePtr window_hmi_state = app->RegularHmiState(window_id);
    SDL_LOG_DEBUG("State: " << *state
                            << " window state: " << *window_hmi_state);
    if (window_hmi_state->audio_streaming_state() !=
            state->audio_streaming_state() ||
        window_hmi_state->video_streaming_state() !=
            state->video_streaming_state()) {
      SDL_LOG_DEBUG("Updating streaming state for window #" << window_id);

      HmiStatePtr new_window_state =
          CreateHmiState(app, HmiState::StateID::STATE_ID_REGULAR);
      DCHECK_OR_RETURN_VOID(new_window_state);
      new_window_state->set_hmi_level(window_hmi_state->hmi_level());
      new_window_state->set_audio_streaming_state(
          state->audio_streaming_state());
      new_window_state->set_video_streaming_state(
          state->video_streaming_state());
      new_window_state->set_system_context(window_hmi_state->system_context());
      new_window_state->set_window_type(window_hmi_state->window_type());
      app->SetRegularState(window_id, new_window_state);

      auto notification =
          MessageHelper::CreateHMIStatusNotification(app, window_id);
      app_mngr_.GetRPCService().ManageMobileCommand(
          notification, commands::Command::SOURCE_SDL);
    }
  }
}

void StateControllerImpl::HandleOnEvent(
    const event_engine::MobileEvent& event) {
  using namespace mobile_apis;

  SDL_LOG_AUTO_TRACE();
  SDL_LOG_DEBUG("Received event for function" << event.id());
  switch (event.id()) {
    case FunctionID::RegisterAppInterfaceID: {
      auto message = event.smart_object();
      uint32_t connection_key =
          message[strings::params][strings::connection_key].asUInt();
      ApplicationSharedPtr app = app_mngr_.application(connection_key);

      if (app.use_count() == 0) {
        SDL_LOG_WARN("Application doesn't exist");
        return;
      }
      {
        sync_primitives::AutoLock autolock(
            apps_with_pending_hmistatus_notification_lock_);

        auto it = apps_with_pending_hmistatus_notification_.find(app->app_id());
        if (it == apps_with_pending_hmistatus_notification_.end()) {
          SDL_LOG_WARN("Application does not have a pending OnHMIStatus");
          return;
        }

        bool success = message[strings::msg_params][strings::success].asBool();
        if (success) {
          // Only send notification if RAI was a success
          auto notification =
              MessageHelper::CreateHMIStatusNotification(app, 0);
          app_mngr_.GetRPCService().ManageMobileCommand(
              notification, commands::Command::SOURCE_SDL);
        }

        apps_with_pending_hmistatus_notification_.erase(app->app_id());
        if (apps_with_pending_hmistatus_notification_.empty()) {
          unsubscribe_from_event(FunctionID::RegisterAppInterfaceID);
        }
      }
    } break;

    default:
      break;
  }
}  // namespace application_manager

void StateControllerImpl::HandleOnEvent(const event_engine::Event& event) {
  using event_engine::Event;
  using smart_objects::SmartObject;
  using namespace hmi_apis;
  namespace FunctionID = hmi_apis::FunctionID;

  SDL_LOG_AUTO_TRACE();
  const SmartObject& message = event.smart_object();
  const FunctionID::eType id = static_cast<FunctionID::eType>(event.id());
  switch (id) {
    case FunctionID::BasicCommunication_ActivateApp:
    case FunctionID::BasicCommunication_CloseApplication: {
      OnHMIResponse(message);
      break;
    }
    case FunctionID::BasicCommunication_OnAppActivated: {
      OnAppActivated(message);
      break;
    }
    case FunctionID::BasicCommunication_OnAppDeactivated: {
      OnAppDeactivated(message);
      break;
    }
    case FunctionID::VR_Started: {
      ApplyTempState<HmiState::STATE_ID_VR_SESSION>();
      break;
    }
    case FunctionID::VR_Stopped: {
      CancelTempState<HmiState::STATE_ID_VR_SESSION>();
      break;
    }
    case FunctionID::TTS_Started: {
      ApplyTempState<HmiState::STATE_ID_TTS_SESSION>();
      break;
    }
    case FunctionID::TTS_Stopped: {
      CancelTempState<HmiState::STATE_ID_TTS_SESSION>();
      break;
    }
    case FunctionID::BasicCommunication_OnEventChanged: {
      bool is_active =
          message[strings::msg_params][hmi_notification::is_active].asBool();
      const uint32_t event_id =
          message[strings::msg_params][hmi_notification::event_name].asUInt();
      // TODO(AOleynik): Add verification/conversion check here
      const Common_EventTypes::eType state_id =
          static_cast<Common_EventTypes::eType>(event_id);
      if (is_active) {
        if (Common_EventTypes::AUDIO_SOURCE == state_id) {
          ApplyTempState<HmiState::STATE_ID_AUDIO_SOURCE>();
          break;
        }
        if (Common_EventTypes::EMBEDDED_NAVI == state_id) {
          ApplyTempState<HmiState::STATE_ID_EMBEDDED_NAVI>();
          break;
        }
        if (Common_EventTypes::PHONE_CALL == state_id) {
          ApplyTempState<HmiState::STATE_ID_PHONE_CALL>();
          break;
        }
        if (Common_EventTypes::EMERGENCY_EVENT == state_id) {
          ApplyTempState<HmiState::STATE_ID_SAFETY_MODE>();
          break;
        }
        if (Common_EventTypes::DEACTIVATE_HMI == state_id) {
          ApplyTempState<HmiState::STATE_ID_DEACTIVATE_HMI>();
          break;
        }
      } else {
        if (Common_EventTypes::AUDIO_SOURCE == state_id) {
          CancelTempState<HmiState::STATE_ID_AUDIO_SOURCE>();
          break;
        }
        if (Common_EventTypes::EMBEDDED_NAVI == state_id) {
          CancelTempState<HmiState::STATE_ID_EMBEDDED_NAVI>();
          break;
        }
        if (Common_EventTypes::PHONE_CALL == state_id) {
          CancelTempState<HmiState::STATE_ID_PHONE_CALL>();
          break;
        }
        if (Common_EventTypes::EMERGENCY_EVENT == state_id) {
          CancelTempState<HmiState::STATE_ID_SAFETY_MODE>();
          break;
        }
        if (Common_EventTypes::DEACTIVATE_HMI == state_id) {
          CancelTempState<HmiState::STATE_ID_DEACTIVATE_HMI>();
          break;
        }
      }

      SDL_LOG_WARN("Couldn't recognize state id (val='" << state_id << "')");
      break;
    }
    default:
      break;
  }
}

void StateControllerImpl::ActivateDefaultWindow(ApplicationSharedPtr app) {
  SDL_LOG_AUTO_TRACE();
  using namespace mobile_apis;

  const WindowID window_id = PredefinedWindows::DEFAULT_WINDOW;
  const HMILevel::eType hmi_level = HMILevel::HMI_FULL;
  const AudioStreamingState::eType audio_state =
      app->IsAudioApplication() ? AudioStreamingState::AUDIBLE
                                : AudioStreamingState::NOT_AUDIBLE;
  const VideoStreamingState::eType video_state =
      app->IsVideoApplication() ? VideoStreamingState::STREAMABLE
                                : VideoStreamingState::NOT_STREAMABLE;

  SetRegularState(app, window_id, hmi_level, audio_state, video_state, false);

  // After main window activation, streaming state should be updated for
  // another windows of the app
  HmiStatePtr new_state =
      app->RegularHmiState(PredefinedWindows::DEFAULT_WINDOW);
  UpdateAppWindowsStreamingState(app, new_state);
}

void StateControllerImpl::ExitDefaultWindow(ApplicationSharedPtr app) {
  SDL_LOG_AUTO_TRACE();
  using namespace mobile_apis;

  const WindowID window_id = PredefinedWindows::DEFAULT_WINDOW;
  const HMILevel::eType hmi_level = HMILevel::HMI_NONE;
  const AudioStreamingState::eType audio_state =
      AudioStreamingState::NOT_AUDIBLE;
  const VideoStreamingState::eType video_state =
      VideoStreamingState::NOT_STREAMABLE;

  SetRegularState(app, window_id, hmi_level, audio_state, video_state, false);

  // After main window exiting, streaming state should be updated for another
  // windows of the app
  HmiStatePtr new_state =
      app->RegularHmiState(PredefinedWindows::DEFAULT_WINDOW);
  UpdateAppWindowsStreamingState(app, new_state);
}

void StateControllerImpl::OnStateChanged(ApplicationSharedPtr app,
                                         const WindowID window_id,
                                         HmiStatePtr old_state,
                                         HmiStatePtr new_state) {
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(app);
  DCHECK_OR_RETURN_VOID(old_state);
  DCHECK_OR_RETURN_VOID(new_state);
  SDL_LOG_DEBUG("Window #" << window_id << " old state: " << *old_state);
  SDL_LOG_DEBUG("Window #" << window_id << " new state: " << *new_state);

  if ((new_state->hmi_level() == mobile_apis::HMILevel::INVALID_ENUM) &&
      (old_state->hmi_level() == mobile_apis::HMILevel::INVALID_ENUM)) {
    SDL_LOG_DEBUG("HMI level is invalid data.");
    return;
  }

  if (!IsStateChanged(*old_state, *new_state)) {
    SDL_LOG_DEBUG("State has NOT been changed.");
    return;
  }

  if (app->is_ready()) {
    SDL_LOG_DEBUG("Sending OnHMIStatus to application " << app->app_id());
    auto notification =
        MessageHelper::CreateHMIStatusNotification(app, window_id);
    app_mngr_.GetRPCService().ManageMobileCommand(
        notification, commands::Command::SOURCE_SDL);
  } else {
    SDL_LOG_DEBUG(
        "Application "
        << app->app_id()
        << " not ready to receive OnHMIStatus. Delaying notification");
    {
      sync_primitives::AutoLock autolock(
          apps_with_pending_hmistatus_notification_lock_);
      apps_with_pending_hmistatus_notification_.insert(app->app_id());
    }
    subscribe_on_event(mobile_apis::FunctionID::RegisterAppInterfaceID);
  }
  if (mobile_apis::PredefinedWindows::DEFAULT_WINDOW != window_id) {
    SDL_LOG_DEBUG(
        "State was changed not for a main application window. No "
        "additional actions required");
    return;
  }

  if (new_state->hmi_level() == mobile_apis::HMILevel::HMI_NONE) {
    app->ResetDataInNone();
  }

  app_mngr_.OnHMIStateChanged(app->app_id(), old_state, new_state);
  app->usage_report().RecordHmiStateChanged(new_state->hmi_level());

  if (mobile_apis::HMILevel::INVALID_ENUM == old_state->hmi_level()) {
    const auto app_default_hmi_level = app_mngr_.GetDefaultHmiLevel(app);
    if (app_default_hmi_level == new_state->hmi_level()) {
      const uint32_t app_id = app->app_id();
      const uint32_t corr_id =
          postponed_activation_controller_.GetPendingActivationCorrId(app_id);
      if (corr_id > 0) {
        app_mngr_.GetPolicyHandler().OnActivateApp(app_id, corr_id);
        postponed_activation_controller_.RemoveAppToActivate(app_id);
      }
    }
  }
}

bool StateControllerImpl::IsTempStateActive(HmiState::StateID id) const {
  sync_primitives::AutoLock autolock(active_states_lock_);
  return helpers::in_range(active_states_, id);
}

void StateControllerImpl::ResumePostponedWindows(const uint32_t app_id) {
  SDL_LOG_AUTO_TRACE();

  auto it_postponed_windows = postponed_app_widgets_.find(app_id);
  if (it_postponed_windows != postponed_app_widgets_.end()) {
    const WindowStatePairs& window_pairs = it_postponed_windows->second;
    SDL_LOG_DEBUG("Application " << app_id << " has " << window_pairs.size()
                                 << " postponed windows. Restoring...");

    auto application = app_mngr_.application(app_id);
    if (!application) {
      SDL_LOG_ERROR("Application " << app_id << " is not registered");
      postponed_app_widgets_.erase(it_postponed_windows);
      return;
    }

    for (const WindowStatePair& pair : window_pairs) {
      const WindowID window_id = pair.first;
      HmiStatePtr postponed_state = pair.second;

      OnAppWindowAdded(application,
                       window_id,
                       postponed_state->window_type(),
                       postponed_state->hmi_level());
    }

    postponed_app_widgets_.erase(it_postponed_windows);
  }
}

void StateControllerImpl::DropPostponedWindows(const uint32_t app_id) {
  SDL_LOG_AUTO_TRACE();
  auto it_postponed_windows = postponed_app_widgets_.find(app_id);
  if (it_postponed_windows != postponed_app_widgets_.end()) {
    SDL_LOG_DEBUG("Dropping postponed windows information for application "
                  << app_id);
    postponed_app_widgets_.erase(it_postponed_windows);
  }
}

void StateControllerImpl::OnApplicationRegistered(
    ApplicationSharedPtr app,
    const mobile_apis::HMILevel::eType default_level) {
  SDL_LOG_AUTO_TRACE();

  // After app registration HMI level should be set for DEFAULT_WINDOW only
  OnAppWindowAdded(app,
                   mobile_apis::PredefinedWindows::DEFAULT_WINDOW,
                   mobile_apis::WindowType::MAIN,
                   default_level);
}

void StateControllerImpl::OnAppWindowAdded(
    ApplicationSharedPtr app,
    const WindowID window_id,
    const mobile_apis::WindowType::eType window_type,
    const mobile_apis::HMILevel::eType default_level) {
  namespace HMILevel = mobile_apis::HMILevel;
  namespace AudioStreamingState = mobile_apis::AudioStreamingState;
  namespace SystemContext = mobile_apis::SystemContext;
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(app);

  if (mobile_apis::WindowType::WIDGET == window_type) {
    auto main_state =
        app->CurrentHmiState(mobile_apis::PredefinedWindows::DEFAULT_WINDOW);
    if (mobile_apis::HMILevel::INVALID_ENUM == main_state->hmi_level()) {
      SDL_LOG_DEBUG("Application " << app->app_id()
                                   << " is not registered. Widget with ID: "
                                   << window_id << " has been postponed");

      HmiStatePtr postponed_state =
          CreateHmiState(app, HmiState::StateID::STATE_ID_REGULAR);
      postponed_state->set_window_type(window_type);
      postponed_state->set_hmi_level(default_level);

      WindowStatePair pair = std::make_pair(window_id, postponed_state);
      postponed_app_widgets_[app->app_id()].push_back(pair);
      return;
    }
  }

  {
    sync_primitives::AutoLock lck(active_states_lock_);
    for (const auto state_id : active_states_) {
      HmiStatePtr new_state = CreateHmiState(app, state_id);
      DCHECK_OR_RETURN_VOID(new_state);
      DCHECK_OR_RETURN_VOID(new_state->state_id() !=
                            HmiState::STATE_ID_REGULAR);
      HmiStatePtr old_hmi_state = app->CurrentHmiState(window_id);
      new_state->set_parent(old_hmi_state);
      new_state->set_window_type(old_hmi_state->window_type());
      app->AddHMIState(window_id, new_state);
    }
  }

  HmiStatePtr default_state =
      CreateHmiState(app, HmiState::StateID::STATE_ID_REGULAR);
  DCHECK_OR_RETURN_VOID(default_state);
  default_state->set_hmi_level(default_level);
  default_state->set_audio_streaming_state(CalcAudioState(app, default_level));
  default_state->set_video_streaming_state(CalcVideoState(app, default_level));
  default_state->set_system_context(SystemContext::SYSCTXT_MAIN);
  default_state->set_window_type(window_type);

  HmiStatePtr initial_state = app->RegularHmiState(window_id);

  app->SetRegularState(window_id, default_state);

  HmiStatePtr new_state = app->CurrentHmiState(window_id);

  OnStateChanged(app, window_id, initial_state, new_state);
}

int64_t StateControllerImpl::RequestHMIStateChange(
    ApplicationConstSharedPtr app,
    hmi_apis::Common_HMILevel::eType level,
    bool send_policy_priority) {
  SDL_LOG_AUTO_TRACE();
  smart_objects::SmartObjectSPtr request = NULL;
  if (hmi_apis::Common_HMILevel::NONE == level) {
    request = MessageHelper::GetBCCloseApplicationRequestToHMI(app, app_mngr_);
  } else {
    request = MessageHelper::GetBCActivateAppRequestToHMI(
        app,
        app_mngr_.GetPolicyHandler(),
        level,
        send_policy_priority,
        app_mngr_);
  }
  if (!request) {
    SDL_LOG_ERROR("Unable to create request");
    return -1;
  }
  if (!app_mngr_.GetRPCService().ManageHMICommand(request)) {
    SDL_LOG_ERROR("Unable to send request");
    return -1;
  }
  const uint32_t corr_id =
      (*request)[strings::params][strings::correlation_id].asUInt();
  return static_cast<int64_t>(corr_id);
}

void StateControllerImpl::ApplyPostponedStateForApp(ApplicationSharedPtr app) {
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(app);
  const WindowIds window_ids = app->GetWindowIds();

  for (const auto& window_id : window_ids) {
    HmiStatePtr state = app->PostponedHmiState(window_id);
    if (state) {
      app->RemovePostponedState(window_id);
      state->set_state_id(HmiState::STATE_ID_REGULAR);
      SetRegularState(app, window_id, state);
    }
  }
}

void StateControllerImpl::TempStateStarted(HmiState::StateID id) {
  SDL_LOG_AUTO_TRACE();

  {
    sync_primitives::AutoLock autolock(active_states_lock_);
    if (!helpers::in_range(active_states_, id)) {
      active_states_.push_back(id);
      return;
    }
  }

  SDL_LOG_ERROR("StateID '" << id << "' is already active");
}

void StateControllerImpl::TempStateStopped(HmiState::StateID ID) {
  SDL_LOG_AUTO_TRACE();
  {
    sync_primitives::AutoLock autolock(active_states_lock_);
    active_states_.remove(ID);
  }
  ForEachApplication(std::bind1st(
      std::mem_fun(&StateControllerImpl::ApplyPostponedStateForApp), this));
}

void StateControllerImpl::DeactivateApp(ApplicationSharedPtr app,
                                        const WindowID window_id) {
  using namespace mobile_apis;
  SDL_LOG_AUTO_TRACE();

  DCHECK_OR_RETURN_VOID(app);
  const HmiStatePtr regular = app->RegularHmiState(window_id);
  DCHECK_OR_RETURN_VOID(regular);
  HmiStatePtr new_regular = std::make_shared<HmiState>(*regular);

  SDL_LOG_DEBUG("Window #" << window_id << " current HMI level: '"
                           << app->hmi_level(window_id) << "'");
  const bool is_audio_app = app->IsAudioApplication();
  const bool is_video_app = app->IsVideoApplication();

  if (is_audio_app || is_video_app) {
    // audio or video app move to HMI level limited
    new_regular->set_hmi_level(HMILevel::HMI_LIMITED);

    if (is_audio_app) {
      new_regular->set_audio_streaming_state(AudioStreamingState::AUDIBLE);
    } else {
      new_regular->set_audio_streaming_state(AudioStreamingState::NOT_AUDIBLE);
    }

    if (is_video_app) {
      new_regular->set_video_streaming_state(VideoStreamingState::STREAMABLE);
    } else {
      new_regular->set_video_streaming_state(
          VideoStreamingState::NOT_STREAMABLE);
    }
  } else {
    new_regular->set_hmi_level(HMILevel::HMI_BACKGROUND);
    new_regular->set_audio_streaming_state(AudioStreamingState::NOT_AUDIBLE);
    new_regular->set_video_streaming_state(VideoStreamingState::NOT_STREAMABLE);
  }

  SetRegularState(app, window_id, new_regular, false);
}

void StateControllerImpl::OnHMIResponse(
    const smart_objects::SmartObject& message) {
  const hmi_apis::Common_Result::eType code =
      static_cast<hmi_apis::Common_Result::eType>(
          message[strings::params][hmi_response::code].asInt());
  const uint32_t correlation_id =
      message[strings::params][strings::correlation_id].asUInt();
  const uint32_t hmi_app_id = app_mngr_.application_id(correlation_id);
  ApplicationSharedPtr application =
      app_mngr_.application_by_hmi_app(hmi_app_id);
  if (application && hmi_apis::Common_Result::SUCCESS == code) {
    HmiStatePtr pending_state = waiting_for_response_[application->app_id()];
    DCHECK_OR_RETURN_VOID(pending_state);
    ApplyRegularState(application,
                      mobile_apis::PredefinedWindows::DEFAULT_WINDOW,
                      pending_state);
  }
}

void StateControllerImpl::OnAppActivated(
    const smart_objects::SmartObject& message) {
  using namespace mobile_apis;
  SDL_LOG_AUTO_TRACE();

  uint32_t app_id = message[strings::msg_params][strings::app_id].asUInt();
  ApplicationSharedPtr app = app_mngr_.application(app_id);
  if (!app) {
    SDL_LOG_ERROR("Application with id " << app_id << " not found");
    return;
  }

  WindowID window_id = mobile_apis::PredefinedWindows::DEFAULT_WINDOW;
  if (message[strings::msg_params].keyExists(strings::window_id)) {
    window_id = message[strings::msg_params][strings::window_id].asInt();
  }

  const auto window_ids = app->GetWindowIds();
  if (!helpers::in_range(window_ids, window_id)) {
    SDL_LOG_ERROR("Application " << app_id << " does not contain window #"
                                 << window_id);
    return;
  }

  if (PredefinedWindows::DEFAULT_WINDOW != window_id) {
    const auto window_hmi_level = app->hmi_level(window_id);
    const HMILevel::eType new_hmi_level = HMILevel::HMI_NONE == window_hmi_level
                                              ? HMILevel::HMI_BACKGROUND
                                              : HMILevel::HMI_FULL;
    const AudioStreamingState::eType audio_state = app->audio_streaming_state();
    const VideoStreamingState::eType video_state = app->video_streaming_state();

    SetRegularState(
        app, window_id, new_hmi_level, audio_state, video_state, false);
    return;
  }

  SetRegularState(app, window_id, HMILevel::HMI_FULL, true);
}

void StateControllerImpl::OnAppDeactivated(
    const smart_objects::SmartObject& message) {
  using namespace mobile_apis;
  SDL_LOG_AUTO_TRACE();

  uint32_t app_id = message[strings::msg_params][strings::app_id].asUInt();
  ApplicationSharedPtr app = app_mngr_.application(app_id);
  if (!app) {
    SDL_LOG_ERROR("Application with id " << app_id << " not found");
    return;
  }

  WindowID window_id = mobile_apis::PredefinedWindows::DEFAULT_WINDOW;
  if (message[strings::msg_params].keyExists(strings::window_id)) {
    window_id = message[strings::msg_params][strings::window_id].asInt();
  }

  const auto window_ids = app->GetWindowIds();
  if (!helpers::in_range(window_ids, window_id)) {
    SDL_LOG_ERROR("Application " << app_id << " does not contain window #"
                                 << window_id);
    return;
  }

  const auto window_hmi_level = app->hmi_level(window_id);
  if (PredefinedWindows::DEFAULT_WINDOW != window_id) {
    const HMILevel::eType new_hmi_level = HMILevel::HMI_FULL == window_hmi_level
                                              ? HMILevel::HMI_BACKGROUND
                                              : HMILevel::HMI_NONE;
    const AudioStreamingState::eType audio_state = app->audio_streaming_state();
    const VideoStreamingState::eType video_state = app->video_streaming_state();

    SetRegularState(
        app, window_id, new_hmi_level, audio_state, video_state, false);
    return;
  }

  if (HMILevel::HMI_FULL != window_hmi_level) {
    return;
  }

  // TODO(AOleynik): Need to delete DeactivateReason and modify
  // OnAppDeactivated when HMI will support that, otherwise won't be testable
  DeactivateApp(app, window_id);
}

void StateControllerImpl::OnVideoStreamingStarted(
    ApplicationConstSharedPtr app) {
  if (app->is_navi()) {
    ApplyTempState<HmiState::STATE_ID_NAVI_STREAMING>();
  } else {
    ApplyTempState<HmiState::STATE_ID_VIDEO_STREAMING>();
  }
}

void StateControllerImpl::OnVideoStreamingStopped(
    ApplicationConstSharedPtr app) {
  if (app->is_navi()) {
    CancelTempState<HmiState::STATE_ID_NAVI_STREAMING>();
  } else {
    CancelTempState<HmiState::STATE_ID_VIDEO_STREAMING>();
  }
}

bool StateControllerImpl::IsStateActive(HmiState::StateID state_id) const {
  SDL_LOG_AUTO_TRACE();
  switch (state_id) {
    case HmiState::STATE_ID_CURRENT:
    case HmiState::STATE_ID_REGULAR:
      return true;
    default:
      return IsTempStateActive(state_id);
  }
  return false;
}

HmiStatePtr StateControllerImpl::CreateHmiState(
    std::shared_ptr<Application> app, HmiState::StateID state_id) const {
  using namespace utils;
  SDL_LOG_AUTO_TRACE();
  HmiStatePtr new_state;
  switch (state_id) {
    case HmiState::STATE_ID_PHONE_CALL: {
      new_state = std::make_shared<PhoneCallHmiState>(app, app_mngr_);
      break;
    }
    case HmiState::STATE_ID_SAFETY_MODE: {
      new_state = std::make_shared<SafetyModeHmiState>(app, app_mngr_);
      break;
    }
    case HmiState::STATE_ID_VR_SESSION: {
      new_state = std::make_shared<VRHmiState>(app, app_mngr_);
      break;
    }
    case HmiState::STATE_ID_TTS_SESSION: {
      new_state = std::make_shared<TTSHmiState>(app, app_mngr_);
      break;
    }
    case HmiState::STATE_ID_VIDEO_STREAMING: {
      new_state = std::make_shared<VideoStreamingHmiState>(app, app_mngr_);
      break;
    }
    case HmiState::STATE_ID_NAVI_STREAMING: {
      new_state = std::make_shared<NaviStreamingHmiState>(app, app_mngr_);
      break;
    }
    case HmiState::STATE_ID_REGULAR: {
      new_state = std::make_shared<HmiState>(app, app_mngr_);
      break;
    }
    case HmiState::STATE_ID_POSTPONED: {
      new_state = std::make_shared<HmiState>(app, app_mngr_, state_id);
      break;
    }
    case HmiState::STATE_ID_DEACTIVATE_HMI: {
      new_state = std::make_shared<DeactivateHMI>(app, app_mngr_);
      break;
    }
    case HmiState::STATE_ID_AUDIO_SOURCE: {
      new_state = std::make_shared<AudioSource>(app, app_mngr_);
      break;
    }
    case HmiState::STATE_ID_EMBEDDED_NAVI: {
      new_state = std::make_shared<EmbeddedNavi>(app, app_mngr_);
      break;
    }
    default:
      SDL_LOG_FATAL("Invalid state_id " << state_id);
      NOTREACHED();
      break;
  }
  return new_state;
}

mobile_apis::AudioStreamingState::eType StateControllerImpl::CalcAudioState(
    ApplicationSharedPtr app,
    const mobile_apis::HMILevel::eType hmi_level) const {
  auto state = mobile_apis::AudioStreamingState::NOT_AUDIBLE;
  if (IsStreamableHMILevel(hmi_level) && app->IsAudioApplication()) {
    state = mobile_apis::AudioStreamingState::AUDIBLE;
  }

  SDL_LOG_DEBUG("Calculated audio state of app "
                << app->app_id() << " for " << hmi_level << " HMI level is "
                << state);
  return state;
}

mobile_apis::VideoStreamingState::eType StateControllerImpl::CalcVideoState(
    ApplicationSharedPtr app,
    const mobile_apis::HMILevel::eType hmi_level) const {
  auto state = mobile_apis::VideoStreamingState::NOT_STREAMABLE;

  if (app->IsVideoApplication() && IsStreamableHMILevel(hmi_level)) {
    state = mobile_apis::VideoStreamingState::STREAMABLE;
  }

  SDL_LOG_DEBUG("Calculated video state of app "
                << app->app_id() << " for " << hmi_level << " HMI level is "
                << state);
  return state;
}

PostponedActivationController&
StateControllerImpl::GetPostponedActivationController() {
  return postponed_activation_controller_;
}

}  // namespace application_manager
