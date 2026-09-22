// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TETRODOTOXIN_MODEL_EXECUTION_CONSTANT_H
#define TETRODOTOXIN_MODEL_EXECUTION_CONSTANT_H


#define TETRODOTOXIN_MODEL_EXECUTION_CONSTANT_ID_HIGH 0xd7ddef7c03a64871ULL
#define TETRODOTOXIN_MODEL_EXECUTION_CONSTANT_ID_LOW 0xabec05487ee1b4bdULL

// This property promises that the payload exposed through Execution Value is
// immutable for its publication. A terminal can therefore embed that payload
// in generated code. Domain supplies the Type independently, and get_data has
// no implied constant promise. A constant payload does not make a later
// conversion or destination permission Constant.

#endif
