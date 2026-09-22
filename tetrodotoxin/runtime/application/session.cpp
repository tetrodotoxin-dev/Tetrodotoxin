// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/runtime/application/session.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;

static thread_local Runtime::Application::Session* active_session = nullptr;

Runtime::Application::Session::~Session() {
  release_stack();
}

auto Runtime::Application::Session::invoke(
    Scene::Lifecycle callback,
    void** object) -> void {
  Session* previous = active_session;
  active_session = this;
  callback(object);
  active_session = previous;
}

auto Runtime::Application::Session::invoke(
    Scene::Update callback,
    void** object,
    R64 delta_time) -> void {
  Session* previous = active_session;
  active_session = this;
  callback(object, delta_time);
  active_session = previous;
}

auto Runtime::Application::Session::create_entry(Count descriptor)
    -> Core::Option<Entry> {
  BAIL_IF(descriptor >= product.scene_count);
  const Scene& scene = product.scenes[descriptor];
  BAIL_IF(
      !scene.construct || !scene.prepare || !scene.update || !scene.release);
  void* object = scene.construct();
  BAIL_IF(object == nullptr);
  invoke(scene.prepare, &object);
  return Entry{descriptor, object};
}

auto Runtime::Application::Session::release_entry(Entry entry) -> void {
  if (entry.descriptor >= product.scene_count || entry.object == nullptr) {
    return;
  }
  const Scene& scene = product.scenes[entry.descriptor];
  invoke(scene.release, &entry.object);
  Core::Object<>(Core::Data::cast<U8>(entry.object)).release();
}

auto Runtime::Application::Session::release_stack() -> void {
  while (stack.get_size() != 0) {
    release_entry(stack[stack.get_size() - 1]);
    stack.remove(stack.get_size() - 1);
  }
  pending_events.clear();
  deferred_events.clear();
  delivering_events = False;
  running = False;
}

auto Runtime::Application::Session::start() -> Bool {
  BAIL_IF(running || product.initial_scene >= product.scene_count);
  auto initial = create_entry(product.initial_scene);
  BAIL_IF(!initial);
  stack.insert(*initial);
  running = True;
  return True;
}

auto Runtime::Application::Session::update(R64 delta_time) -> Bool {
  BAIL_IF(!running || stack.get_size() == 0);
  Entry& active = stack[stack.get_size() - 1];
  invoke(product.scenes[active.descriptor].update, &active.object, delta_time);
  return True;
}

auto Runtime::Application::Session::find_transition(Entry active)
    -> Core::Option<const Transition&> {
  for (const Event& event : pending_events.get_view()) {
    if (event.object != active.object) {
      continue;
    }
    for (Count index = 0; index < product.transition_count; index++) {
      const Transition& transition = product.transitions[index];
      if (transition.source == active.descriptor &&
          transition.signal == event.signal) {
        return transition;
      }
    }
  }
  return {};
}

auto Runtime::Application::Session::commit() -> Bool {
  BAIL_IF(!running || stack.get_size() == 0);
  Entry& active = stack[stack.get_size() - 1];
  delivering_events = True;
  auto transition = find_transition(active);
  if (transition) {
    switch (transition->action) {
    case Action::Replace: {
      release_entry(active);
      stack.remove(stack.get_size() - 1);
      auto replacement = create_entry(transition->destination);
      if (!replacement) {
        running = False;
      } else {
        stack.insert(*replacement);
      }
      break;
    }
    case Action::Push: {
      const Scene& scene = product.scenes[active.descriptor];
      if (scene.pause) {
        invoke(scene.pause, &active.object);
      }
      auto pushed = create_entry(transition->destination);
      if (!pushed) {
        running = False;
      } else {
        stack.insert(*pushed);
      }
      break;
    }
    case Action::Pop:
      release_entry(active);
      stack.remove(stack.get_size() - 1);
      if (stack.get_size() == 0) {
        running = False;
      } else {
        Entry& resumed = stack[stack.get_size() - 1];
        auto callback = product.scenes[resumed.descriptor].resume;
        if (callback) {
          invoke(callback, &resumed.object);
        }
      }
      break;
    case Action::Exit:
      running = False;
      break;
    }
  }

  // Events raised while committing belong to the next frame. Moving that
  // queue after selection keeps the first matching transition deterministic
  // even when release or prepare callbacks publish another Signal.
  pending_events.clear();
  pending_events =
      static_cast<Memory::Dynamic::Vector<Event>&&>(deferred_events);
  deferred_events = Memory::Dynamic::Vector<Event>();
  delivering_events = False;
  return running;
}

auto Runtime::Application::Session::stop() -> void {
  release_stack();
}

auto Runtime::Application::Session::publish(void* object, const U8* signal)
    -> void {
  if (object == nullptr || signal == nullptr) {
    return;
  }
  Event event{object, signal};
  if (delivering_events) {
    deferred_events.insert(event);
  } else {
    pending_events.insert(event);
  }
}

auto Runtime::Application::Session::get_active_object() const
    -> Core::Object<> {
  return stack.get_size() == 0
             ? Core::Object<>()
             : Core::Object<>(Core::Data::cast<U8>(
                   stack.get_view().get_data()[stack.get_size() - 1].object));
}

auto Runtime::Application::Session::get_active_scene() const
    -> Core::Option<const Scene&> {
  BAIL_IF(stack.get_size() == 0);
  Count descriptor =
      stack.get_view().get_data()[stack.get_size() - 1].descriptor;
  BAIL_IF(descriptor >= product.scene_count);
  return product.scenes[descriptor];
}

extern "C" auto tetrodotoxin_scene_emit(void* scene, const U8* signal) -> void {
  if (active_session) {
    active_session->publish(scene, signal);
  }
}
