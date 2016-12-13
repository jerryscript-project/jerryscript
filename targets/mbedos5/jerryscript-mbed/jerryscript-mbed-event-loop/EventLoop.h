/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _JERRYSCRIPT_MBED_EVENT_LOOP_EVENT_LOOP_H
#define _JERRYSCRIPT_MBED_EVENT_LOOP_EVENT_LOOP_H

#include <vector>

#include "jerry-core/jerry-api.h"

#include "Callback.h"
#include "mbed_assert.h"

#include "events/EventQueue.h"

#include "jerryscript-mbed-util/logging.h"
#include "jerryscript-mbed-event-loop/BoundCallback.h"

extern "C" void exit(int return_code);

namespace mbed {
namespace js {

static const uint32_t EVENT_INTERVAL_MS = 1;

class EventLoop {
 private:
    static EventLoop instance;

 public:
    static EventLoop& getInstance() {
        return instance;
    }

    void go() {
        while (true) {
            queue.dispatch();
        }
    }

    Callback<void()> wrapFunction(jerry_value_t f) {
        MBED_ASSERT(jerry_value_is_function(f));

        // not sure if this is necessary?
        jerry_acquire_value(f);

        // we need to return a callback that'll schedule this
        Callback<void(uint32_t)> cb_raw(this, &EventLoop::callback);
        BoundCallback<void(uint32_t)> *cb = new BoundCallback<void(uint32_t)>(cb_raw, f);

        bound_callbacks.push_back(std::make_pair(f, cb));

        return *cb;
    }

    void dropCallback(jerry_value_t f) {
        jerry_release_value(f);

        for (std::vector<std::pair<jerry_value_t, BoundCallback<void(uint32_t)>*> >::iterator it = bound_callbacks.begin(); it != bound_callbacks.end(); it++) {
            std::pair<jerry_value_t, BoundCallback<void(uint32_t)>*> element = *it;

            if (element.first == f) {
                delete element.second;
                break;
            }
        }
    }

    void callback(jerry_value_t f) {
        queue.call(jerry_call_function, f, jerry_create_null(), (const jerry_value_t*)NULL, 0);
    }

    void nativeCallback(Callback<void()> cb) {
        queue.call(cb);
    }

    events::EventQueue& getQueue() {
        return queue;
    }

 private:
    EventLoop() {}

    std::vector<std::pair<jerry_value_t, BoundCallback<void(uint32_t)>*> > bound_callbacks;
    events::EventQueue queue;
};

void event_loop();

}  // namespace js
}  // namespace mbed

#endif  // _JERRYSCRIPT_MBED_EVENT_LOOP_EVENT_LOOP_H
