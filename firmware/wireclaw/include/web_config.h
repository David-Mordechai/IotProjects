/**
 * @file web_config.h
 * @brief Web-based configuration portal for runtime config changes
 *
 * Provides a lightweight HTTP server on port 80 with REST API and
 * single-page UI for editing config, system prompt, memory, and
 * viewing device status. Uses ESPmDNS for <device-name>.local access.
 */

#ifndef WEB_CONFIG_H
#define WEB_CONFIG_H

/* System prompt buffer (incl. NUL). The stock data/system_prompt.txt is ~7.3 KB. */
#define SYSTEM_PROMPT_MAX 8192

/**
 * Initialize the web config server and mDNS responder.
 * Call from setup() after WiFi is connected.
 */
void webConfigSetup();

/**
 * Process pending HTTP requests.
 * Call from loop() on every iteration.
 */
void webConfigLoop();

#endif /* WEB_CONFIG_H */
