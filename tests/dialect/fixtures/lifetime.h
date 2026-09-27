// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef VALIDATION_DIALECT_LIFETIME_H
#define VALIDATION_DIALECT_LIFETIME_H

#include "perimortem/core/perimortem.h"

// These counters have static storage in the test host. An unload regression
// must fail an assertion without leaving a module finalizer pointing into a
// departed test stack. The watch symbol is fixture instrumentation, separate
// from module acquisition and every negotiated Source contract in the test.
typedef struct monograph_lifetime {
  U64 acquisitions;
  U64 acquisition_releases;
  U64 live;
  U64 releases;
  U64 children_released;
  U64 unloaded;
} monograph_lifetime;

PERIMORTEM_C void monograph_fixture_watch(monograph_lifetime* lifetime);

#endif
