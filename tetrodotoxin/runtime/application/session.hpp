// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/runtime/application/product.hpp"

namespace Tetrodotoxin::Runtime::Application {

// Session owns the live Scene stack and event delivery for one immutable App
// product. Window, input, clocks, and presentation remain outside, which lets
// the same lifecycle policy run against a deterministic host in validation.
class Session {
 public:
  explicit Session(const Product& product) : product(product) {}
  ~Session();

  Session(const Session&) = delete;
  Session(Session&&) = delete;
  auto operator=(const Session&) -> Session& = delete;
  auto operator=(Session&&) -> Session& = delete;

  auto start() -> Bool;
  auto update(R64 delta_time) -> Bool;
  auto commit() -> Bool;
  auto stop() -> void;

  auto publish(void* object, const U8* signal) -> void;

  constexpr auto is_running() const -> Bool { return running; }
  auto get_active_object() const -> Perimortem::Core::Object<>;
  auto get_active_scene() const -> Perimortem::Core::Option<const Scene&>;

 private:
  struct Entry {
    Count descriptor;
    void* object;
  };

  struct Event {
    void* object;
    const U8* signal;
  };

  auto create_entry(Count descriptor) -> Perimortem::Core::Option<Entry>;
  auto release_entry(Entry entry) -> void;
  auto release_stack() -> void;
  auto find_transition(Entry active)
      -> Perimortem::Core::Option<const Transition&>;
  auto invoke(Scene::Lifecycle callback, void** object) -> void;
  auto invoke(Scene::Update callback, void** object, R64 delta_time) -> void;

  const Product& product;
  Perimortem::Memory::Dynamic::Vector<Entry> stack;
  Perimortem::Memory::Dynamic::Vector<Event> pending_events;
  Perimortem::Memory::Dynamic::Vector<Event> deferred_events;
  Bool delivering_events = False;
  Bool running = False;
};

}  // namespace Tetrodotoxin::Runtime::Application

extern "C" auto tetrodotoxin_scene_emit(void* scene, const U8* signal) -> void;
