// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_MODEL_TYPE_UNSIGNED_H
#define TETRODOTOXIN_MODEL_TYPE_UNSIGNED_H

#define TETRODOTOXIN_MODEL_TYPE_UNSIGNED_ID_HIGH 0xf8af28efa1f5424fULL
#define TETRODOTOXIN_MODEL_TYPE_UNSIGNED_ID_LOW 0x92e54cccf13b5a2dULL

// Unsigned promises that the numeric interpretation excludes negative values.
// Consumers can ask for this property without acquiring an API or a storage
// description. Width, arithmetic and overflow rules belong to other contracts,
// so a substitute can preserve this promise while choosing another carrier.

#endif

