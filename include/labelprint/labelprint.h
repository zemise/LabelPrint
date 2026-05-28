#pragma once

// Version
#include "labelprint/version.h"

// Elements and document model
#include "labelprint/elements.h"
#include "labelprint/document.h"

// Printer profiles
#include "labelprint/printer_profile.h"

// Backend interface + all concrete backends
#include "labelprint/backend.h"
#include "labelprint/zpl_backend.h"
#include "labelprint/tspl_backend.h"
#include "labelprint/ezpl_gb2312_backend.h"
#include "labelprint/tspl_gb18030_backend.h"
#include "labelprint/tspl_bitmap_backend.h"

// Template API
#include "labelprint/template.h"

// High-level medical label printing API
#include "labelprint/medical_label_print.h"

// Transport layer
#include "labelprint/transport.h"
