// firmware/device-writer/src/portal.h
#pragma once
#include "config_store.h"

// Starts SoftAP and a minimal HTTP server for configuration.
// Returns only after /save triggers a restart, or if timeoutMs elapsed.
void runConfigPortal(WriterConfig &cfg, uint32_t timeoutMs = 0);
