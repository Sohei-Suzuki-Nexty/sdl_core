/*
 Copyright (c) 2016, Ford Motor Company
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

#include "application_manager/event_engine/event_dispatcher_impl.h"
#include <algorithm>
#include "application_manager/event_engine/event_observer.h"
#include "interfaces/HMI_API.h"

namespace application_manager {
namespace event_engine {
using namespace sync_primitives;

EventDispatcherImpl::EventDispatcherImpl() : observers_event_() {}

EventDispatcherImpl::~EventDispatcherImpl() {}

void EventDispatcherImpl::raise_event(const Event& event) {
  ObserverVector observers;
  AutoLock observer_lock(observer_lock_);
  {
    AutoLock state_lock(state_lock_);

    // check if event is notification
    if (hmi_apis::messageType::notification == event.smart_object_type()) {
      const uint32_t notification_correlation_id = 0;
      observers = observers_event_[event.id()][notification_correlation_id];
    }

    if (hmi_apis::messageType::response == event.smart_object_type() ||
        hmi_apis::messageType::error_response == event.smart_object_type()) {
      observers =
          observers_event_[event.id()][event.smart_object_correlation_id()];
    }
  }

  while (!observers.empty()) {
    EventObserver* temp = *observers.begin();
    observers.erase(observers.begin());
    AutoUnlock unlock_observer(observer_lock);

    if (temp->IncrementReferenceCount()) {
      temp->HandleOnEvent(event);
      temp->DecrementReferenceCount();
    }
  }
}

void EventDispatcherImpl::add_observer(const Event::EventID& event_id,
                                       int32_t hmi_correlation_id,
                                       EventObserver& observer) {
  AutoLock auto_lock(state_lock_);
  observers_event_[event_id][hmi_correlation_id].push_back(&observer);
}

struct IdCheckFunctor {
  explicit IdCheckFunctor(const unsigned long id) : target_id(id) {}

  bool operator()(const EventObserver* obs) const {
    return (obs->id() == target_id);
  }

 private:
  const unsigned long target_id;
};

void EventDispatcherImpl::remove_observer(const Event::EventID& event_id,
                                          const int32_t hmi_correlation_id) {
  AutoLock auto_lock(state_lock_);
  auto& observers = observers_event_[event_id][hmi_correlation_id];
  for (auto observer : observers) {
    remove_observer_from_vector(*observer);
  }
}

void EventDispatcherImpl::remove_observer(const Event::EventID& event_id,
                                          EventObserver& observer) {
  remove_observer_from_vector(observer);
  AutoLock auto_lock(state_lock_);
  ObserversMap::iterator it = observers_event_[event_id].begin();

  for (; observers_event_[event_id].end() != it; ++it) {
    ObserverVector& obs_vec = it->second;
    const ObserverVector::iterator obs_vec_it = obs_vec.end();
    obs_vec.erase(
        std::remove_if(
            obs_vec.begin(), obs_vec_it, IdCheckFunctor(observer.id())),
        obs_vec_it);
  }
}

void EventDispatcherImpl::remove_observer(EventObserver& observer) {
  remove_observer_from_vector(observer);
  EventObserverMap::iterator event_map = observers_event_.begin();

  for (; observers_event_.end() != event_map; ++event_map) {
    remove_observer(event_map->first, observer);
  }
}

void EventDispatcherImpl::remove_observer_from_vector(EventObserver& observer) {
  AutoLock auto_lock(observer_lock_);

  observers_.erase(
      std::remove_if(
          observers_.begin(), observers_.end(), IdCheckFunctor(observer.id())),
      observers_.end());
}

// Mobile Events

void EventDispatcherImpl::raise_mobile_event(const MobileEvent& event) {
  AutoLock observer_lock(mobile_observer_lock_);
  {
    AutoLock state_lock(mobile_state_lock_);

    // check if event is notification
    if (mobile_apis::messageType::notification == event.smart_object_type()) {
      const uint32_t notification_correlation_id = 0;
      mobile_observers_ =
          mobile_observers_event_[event.id()][notification_correlation_id];
    }

    if (mobile_apis::messageType::response == event.smart_object_type()) {
      mobile_observers_ =
          mobile_observers_event_[event.id()]
                                 [event.smart_object_correlation_id()];
    }
  }

  // Call observers
  while (!mobile_observers_.empty()) {
    EventObserver* temp = *mobile_observers_.begin();
    mobile_observers_.erase(mobile_observers_.begin());
    AutoUnlock unlock_observer(observer_lock);

    if (temp->IncrementReferenceCount()) {
      temp->HandleOnEvent(event);
      temp->DecrementReferenceCount();
    }
  }
}

void EventDispatcherImpl::add_mobile_observer(
    const MobileEvent::MobileEventID& event_id,
    int32_t mobile_correlation_id,
    EventObserver& observer) {
  AutoLock auto_lock(mobile_state_lock_);
  mobile_observers_event_[event_id][mobile_correlation_id].push_back(&observer);
}

void EventDispatcherImpl::remove_mobile_observer(
    const MobileEvent::MobileEventID& event_id, EventObserver& observer) {
  remove_mobile_observer_from_vector(observer);
  AutoLock auto_lock(mobile_state_lock_);
  ObserversMap::iterator it = mobile_observers_event_[event_id].begin();

  for (; mobile_observers_event_[event_id].end() != it; ++it) {
    ObserverVector& obs_vec = it->second;
    const ObserverVector::iterator obs_vec_it = obs_vec.end();
    obs_vec.erase(
        std::remove_if(
            obs_vec.begin(), obs_vec_it, IdCheckFunctor(observer.id())),
        obs_vec_it);
  }
}

void EventDispatcherImpl::remove_mobile_observer(EventObserver& observer) {
  remove_mobile_observer_from_vector(observer);
  MobileEventObserverMap::iterator event_map = mobile_observers_event_.begin();

  for (; mobile_observers_event_.end() != event_map; ++event_map) {
    remove_mobile_observer(event_map->first, observer);
  }
}

void EventDispatcherImpl::remove_mobile_observer_from_vector(
    EventObserver& observer) {
  AutoLock auto_lock(mobile_observer_lock_);

  mobile_observers_.erase(
      std::remove_if(
          observers_.begin(), observers_.end(), IdCheckFunctor(observer.id())),
      observers_.end());
}

}  // namespace event_engine
}  // namespace application_manager
