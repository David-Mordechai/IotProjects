# WireClaw on the classic ESP32 (HW-394): port notes

Copied from [M64GitHub/WireClaw](https://github.com/M64GitHub/WireClaw) at commit `ad84614` (v0.4.0, MIT licence) on 2026-11-20.
Upstream supports only the ESP32-C3, C6 and S3. Its `platformio.ini` said: "Classic ESP32 not supported: DRAM overflow (~60KB over dram0_0_seg limit)". This copy runs on our HW-394.

## Why it did not fit

On the classic ESP32, every static variable (`.data` and `.bss`) must fit in `dram0_0_seg`, which is about 124.7 KB here. The chip has about 320 KB of RAM, but the rest can only be used as heap. WireClaw declares many large static buffers: a 20 KB LLM request, 17 KB of chat history, a 12.7 KB `LlmResult`, and so on. The build overflowed by 78,712 bytes.

## What changed

1. **`include/big_static.h`** (new) adds `BIG_STATIC(T, name)`.
   - On the classic ESP32 it reserves the buffer from the heap on first use and gives back a *reference* to the same type, so `sizeof(name)` and indexing still work. The memory is zeroed, like a normal static.
   - On C3/C6/S3 it is a plain `static`, so upstream builds are unchanged.
   - If memory runs out it prints `[BIG_STATIC] out of memory: <name>` and stops.
2. **19 function-local static buffers** (about 112 KB) now use `BIG_STATIC`:
   - `llm_client.cpp`: `request_buf`, `response_buf`
   - `main.cpp`: `escaped` ×2, history-load `buf`, `result`, Telegram `req`/`resp`
   - `tools.cpp`: `req`
   - `rules.cpp`: `buf` ×2
   - `web_config.cpp`: `buf` ×3, `existing`, `fields`
   - `devices.cpp`: `buf` ×2
   - `setup_portal.cpp`: `body`
3. **The chat `history` global** is now reached through `historyArr()`, which allocates it on first use.
   - **Do not allocate on the heap at startup.** Before FreeRTOS starts, the only heap is what is left of `dram0_0_seg` after the statics (15.7 KB when this failed).
   - A 17 KB global allocation there left no room for the 4 KB `main` task. The board then boot-looped with `assert failed: esp_startup_start_app app_startup.c:83 (res == pdTRUE)`.
   - Today the startup margin is about 25 KB: `0x3FFDC200 - _heap_start`. Check it with `xtensa-esp32-elf-nm firmware.elf | grep _heap_start`.
4. **The system prompt fits** (upstream issue #18, which applies to every chip). `cfg_system_prompt` grew from 4096 to `SYSTEM_PROMPT_MAX` (8192, in `web_config.h`), because the stock `system_prompt.txt` is 7.3 KB.
   - Before, the last 3.2 KB never reached the model.
   - The web-page limit was raised to match.
   - Boot now warns if the file is still cut off.

5. **`chip_temp` is hidden on the classic ESP32** (`devices.cpp`).
   - The built-in sensor is unsupported. Measured on our board with Wi-Fi on, it read raw 128 = 53.3 °C on every reading, the known "no valid reading" value.
   - Upstream code returned 0.0 °C for it.
   - `deviceRegister()` now refuses `internal_temp` on this chip, so a saved copy in `/devices.json` or a model request can't bring it back.
   - For temperature, use an external `ntc_10k` (10 kΩ, B3950) on an ADC1 pin, GPIO 32-39. ADC2 does not work while Wi-Fi is on.
6. **Chat runs on the PC, not on the board.** The board keeps upstream's web page. Chat goes over NATS: the page and Docker setup are in `tools/nats/` (see `CLAUDE.md`). A short-lived web Chat tab on the board was removed again.
7. **Upstream bugs fixed (they affect every chip, not only ours):**
   - **`setTimeout` units.** Arduino-ESP32 3.x+ `NetworkClient::setTimeout()` takes milliseconds; WireClaw passed seconds (`LLM_READ_TIMEOUT_MS / 1000` = 120 → 120 ms, Telegram 30 → 30 ms). The 120 ms socket timeout also limits sending, so the 17 KB LLM request could be cut off. Ollama then waited for the rest of the body and the board hit `Response timeout (120s)`. Both are now in ms.
   - **Request write checked.** A partial request write now fails at once with `Request only partly sent (x of y bytes)`.
   - **`led_set` without an RGB LED** used to answer "LED set to RGB(...)" while doing nothing, so the model claimed colours it never set. It now returns an error when the board has no `RGB_BUILTIN`.
   - **No silent cut of the response.** The response buffer is 16 KB (`LLM_RESPONSE_BUF_LEN`), because thinking models add a `reasoning` text. A reply that is still too big now gives a clear error instead of a parse failure.
   - **`WiFi.setSleep(false)`.** In modem sleep, the web page (18 KB, sent in 1436-byte pieces) failed after a few chats; with the radio awake it loads every time.
   - **Errors reach the user.** Chat errors show the real reason (`llm.lastError()`).
   - **Duplicate NATS subscriptions after a reconnect.** The NATS library re-sends its whole subscription table itself on reconnect (`lib/nats/proto/nats_core.c`, "Re-subscribe existing subscriptions"), but `connectNats()` subscribed the 6 base subjects again every time.
     - Each reconnect added 6 entries until the 16-entry table (`NATS_MAX_SUBSCRIPTIONS`) was full, and new `nats_value` sensors silently failed to subscribe: they always read 0.0. Seen: chat ×3, cmd ×3.
     - `connectNats()` now subscribes the base subjects only on the first connect.
     - Verified: 7 subscriptions after boot, and the `distance` sensor reads live values.
   - **Rule check interval minimum lowered from 5 s to 1 s** (`tools.cpp`, `rule_create` and `chain_create`). With 5 s, a rule could announce up to 5 s after the distance changed. Reading a `nats_value` or digital sensor is instant.
   - **Lower heap peak during a chat.** `LlmClient::chat()` now frees the 20 KB request buffer as soon as it is sent, before the 16 KB response buffer is allocated. The lowest free heap during a multi-step chat went from 61.8 KB to 90.4 KB.
   - **Heap diagnostics.** `/heap` and `/api/status` also show the largest free block and the minimum ever free.
8. **Large sends need about 64 KB of free heap (measured).** On this chip and core, a TCP send larger than one lwIP send buffer (5744 B) stalls once **total** free heap is below about 64-68 KB: errno 11 once per second, then an incomplete reply.
   - The largest free block does not matter: it was 55 KB in every step.
   - Isolated with a minimal sketch (WebServer 7364-byte `send` plus a 17 KB POST to Ollama):
     - At 190 KB free: 10 of 10 rounds OK.
     - With heap ballast, stepped down in 4 KB steps: OK at 68.4 KB free, failing at 64.1 KB. At 63.5 KB even the 17 KB request sent 0 bytes.
   - lwIP has no such check (`mem_malloc` is plain `heap_caps_malloc`). The limit most likely sits in the closed-source Wi-Fi driver, which allocates a dynamic TX buffer from the heap for every outgoing packet.
   - **Cause on our side:** `BIG_STATIC` kept all ~112 KB of moved buffers allocated forever, leaving only ~70 KB free after the first chat. The Prompt tab (7.3 KB in one write) then stayed empty, and LLM requests could stall.
   - **Fix:** `BIG_TEMP` in `big_static.h`. 17 buffers that are only used during one call are now allocated on entry and freed on return. Only three stay permanent: `result` (its text is returned to the caller), `history`, and the NATS request in `tools.cpp` (the NATS client may write to it later).
   - Result: about 127 KB free when idle, lowest-ever 86.7 KB during chats. 8 of 8 rounds of chat + `/api/prompt` + page OK. The Prompt tab shows the whole prompt.
   - **Rule for future changes:** keep the lowest-ever free heap (`heap_min_free` in `/api/status`) above about 80 KB.
9. **Still open (2026-11-20): the board itself closes its NATS connection, about 6-30 s after some chats.**
   - The server debug log says "Client connection closed: Client Closed" for `nats-embedded`. The board logs "NATS: disconnected", then reconnects within a second.
   - Messages sent during that moment are lost. Rules and events keep working, since they run on the board.
   - No core error line is printed; root cause not found yet.
   - It is not a heap leak: the heap stays flat with 2 sensor messages/s, and memory measured 90 KB+ during chats after the request-buffer fix.
   - NATS server debug logging is on in `tools/nats/nats-server.conf` to catch it.

## Measured (2026-11-20, HW-394 on COM5, Ollama `granite4.1:3b` on the PC)

| What | Value |
|---|---|
| Static RAM | 99,188 B |
| Free heap after boot and Wi-Fi | about 131 KB |
| `gpio_write` tool call round trip | 2.3 s |

## Known limits on this chip

- **No usable temperature sensor.** Upstream code returns `0` for `chip_temp` on the classic ESP32.
- **No onboard RGB LED.** The `led_set` tool has nothing to drive; the blue LED is on GPIO 2 (`gpio_write`).
- **Small models invent answers.** `granite4.1:3b` gave sensor values, and said it switched a pin, without calling a tool (upstream issue #17).
  - Switched to **`qwen3.5:4b`** (thinking on). In the same tests it called `device_info`, `gpio_write` and `temperature_read` correctly, and reported "no temperature sensor" honestly.
  - With Ollama defaults it ran 76% on the GPU and 24% on the CPU (RTX 2060, 6 GB): 7-9 s per answer, 72 s for the first.
  - Ollama reserves the KV cache for 4 parallel conversations by default (1,024 MB). WireClaw only ever sends one.
  - With the user environment variables `OLLAMA_NUM_PARALLEL=1` and `OLLAMA_KV_CACHE_TYPE=q8_0`, the KV cache is 136 MB. All 33 layers then fit on the GPU (`ollama ps`: 100% GPU): about 4.2-4.6 s per tool-call answer, 14 s for the first after loading.
- **The model reuses chat history.** After a question has been answered once, it may repeat the old value instead of calling the tool again. Send `/clear` (not `/history clear`) to wipe the history.

## Build and flash (PowerShell, not Git Bash)

```powershell
pio run -t upload --upload-port COM5      # firmware
pio run -t uploadfs --upload-port COM5    # data/: config.json + system_prompt.txt
```

`data/config.json` holds the Wi-Fi password and is git-ignored. Make it from `data/config.json.ollama.example`. The Wi-Fi name is case-sensitive (`HomeLab`).
