// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef TTX_DATA_STATUS_H
#define TTX_DATA_STATUS_H

#include "perimortem/core/perimortem.h"

// Data operates on completed descriptions. Matching those descriptions or
// transferring the described values can still fail, so these statuses report
// the outcome to the owning operation. Any semantic question that determines
// geometry must be settled before transport, which requires known sizes and
// positions.
typedef U8 ttx_data_status;
#define TTX_DATA_SUCCESS ((ttx_data_status)0)
#define TTX_DATA_INVALID ((ttx_data_status)1)
#define TTX_DATA_BOUNDS ((ttx_data_status)2)
#define TTX_DATA_OVERFLOW ((ttx_data_status)3)
#define TTX_DATA_INCOMPATIBLE ((ttx_data_status)4)
#define TTX_DATA_UNSUPPORTED ((ttx_data_status)5)
#define TTX_DATA_BUSY ((ttx_data_status)6)
#define TTX_DATA_DENIED ((ttx_data_status)7)
#define TTX_DATA_IO_ERROR ((ttx_data_status)8)

#endif
