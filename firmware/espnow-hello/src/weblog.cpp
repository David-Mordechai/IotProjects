// Live log web page for a phone. Only built into the *_web envs (-D WEB_LOG).
//
// How it works:
//  1. The board joins your phone's Wi-Fi hotspot (name/password in secrets.h).
//  2. It runs a small web server on port 80.
//  3. On the phone, open http://<board address>/ . The page asks the board for
//     new log lines once a second (/data?since=N) and adds them to the screen.
// ESP-NOW shares the Wi-Fi radio, so it runs on the hotspot's channel.
#ifdef WEB_LOG

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "weblog.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Copy src/secrets.example.h to src/secrets.h and type in your phone hotspot's name and password"
#endif

#ifdef ROLE_SENDER
static const char* HOSTNAME = "espnow-tx";   // name shown in the phone's hotspot device list
#else
static const char* HOSTNAME = "espnow-rx";
#endif

static WebServer server(80);

// The last 600 log lines (~20 min of walking), in a ring: line number i lives in slot i % MAX_LINES.
static const uint32_t MAX_LINES = 600;
static String lines[MAX_LINES];
static uint32_t nextId = 0;                 // number the next line will get

static long sReceived = 0, sLost = 0;
static float sRssi = 0;
static uint32_t lastPacketMs = 0;
static bool havePacket = false;
static bool online = false;

// ---- Phone GPS (sender_web): Automate calls /loc?lat=..&lon=..&acc=.. every second ----
static double locLat = 0, locLon = 0, homeLat = 0, homeLon = 0;
static int locAcc = 0;
static uint32_t locMs = 0;
static bool haveLoc = false, haveHome = false;
static const int HOME_MAX_ACCURACY_M = 20;   // only a good fix may become "home"

// Distance in metres between two GPS points (flat-earth approximation, fine below ~10 km).
static double metresBetween(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0, D2R = PI / 180.0;
  double x = (lon2 - lon1) * D2R * cos((lat1 + lat2) / 2 * D2R);
  double y = (lat2 - lat1) * D2R;
  return R * sqrt(x * x + y * y);
}

static void handleLoc() {
  if (!server.hasArg("lat") || !server.hasArg("lon")) { server.send(400, "text/plain", "need lat and lon"); return; }
  locLat = server.arg("lat").toDouble();
  locLon = server.arg("lon").toDouble();
  locAcc = server.hasArg("acc") ? (int)server.arg("acc").toFloat() : -1;
  locMs = millis();
  haveLoc = true;
  // Home = the first good fix, so start the app where the receiver is (or at your
  // building's door). /home on the page sets it again.
  if (!haveHome && locAcc >= 0 && locAcc <= HOME_MAX_ACCURACY_M) {
    homeLat = locLat; homeLon = locLon; haveHome = true;
    String m = "Home set to " + String(homeLat, 6) + "," + String(homeLon, 6) + " (accuracy " + String(locAcc) + " m)";
    Serial.println(m);
    webAdd(m.c_str());
  }
  server.send(200, "text/plain", "ok");
}

static void handleHome() {
  haveHome = false;                          // next good fix becomes home
  webAdd("Home cleared: the next good GPS fix becomes home (0 m)");
  server.sendHeader("Location", "/");
  server.send(303);
}

bool webLocation(double& lat, double& lon, int& accuracyM, int& distanceM) {
  if (!haveLoc || millis() - locMs > 5000) return false;
  lat = locLat; lon = locLon; accuracyM = locAcc;
  distanceM = haveHome ? (int)lround(metresBetween(homeLat, homeLon, locLat, locLon)) : -1;
  return true;
}

void webAdd(const char* line) {
  lines[nextId % MAX_LINES] = line;
  nextId++;
}

void webStats(long received, long lost, float rssi) {
  sReceived = received;
  sLost = lost;
  if (rssi != 0) {                // 0 = this round had no answer
    sRssi = rssi;
    lastPacketMs = millis();
    havePacket = true;
  }
}

// Put text into JSON quotes safely (escape " and \, replace odd bytes).
static void addJsonString(String& out, const String& s) {
  out += '"';
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if ((uint8_t)c < 32 || (uint8_t)c > 126) out += '?';
    else out += c;
  }
  out += '"';
}

// /data?since=N -> the numbers + every log line from number N on.
static void handleData() {
  uint32_t since = server.hasArg("since") ? strtoul(server.arg("since").c_str(), nullptr, 10) : 0;
  if (since > nextId) since = 0;                         // board restarted: send all
  uint32_t oldest = nextId > MAX_LINES ? nextId - MAX_LINES : 0;
  if (since < oldest) since = oldest;                    // older lines are gone

  String out;
  out.reserve(512 + (nextId - since) * 90);
  out += "{\"next\":";  out += nextId;
  out += ",\"rx\":";    out += sReceived;
  out += ",\"lost\":";  out += sLost;
  out += ",\"rssi\":";  out += String(sRssi, 1);
  out += ",\"ago\":";   out += havePacket ? String((millis() - lastPacketMs) / 1000.0, 1) : String("-1");
  double la, lo; int acc = -1, dist = -1;
  bool gps = webLocation(la, lo, acc, dist);
  out += ",\"gps\":";   out += gps ? "true" : "false";
  out += ",\"acc\":";   out += acc;
  out += ",\"dist\":";  out += dist;
#ifdef ROLE_SENDER
  out += ",\"good\":\"Answered\",\"bad\":\"Missed\"";
#else
  out += ",\"good\":\"Received\",\"bad\":\"Lost\"";
#endif
  out += ",\"lines\":[";
  for (uint32_t i = since; i < nextId; i++) {
    if (i > since) out += ',';
    addJsonString(out, lines[i % MAX_LINES]);
  }
  out += "]}";
  server.send(200, "application/json", out);
}

static const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP-NOW link</title>
<style>
body{margin:0;font-family:system-ui,sans-serif;background:#10161c;color:#e6edf3}
header{padding:12px 16px;background:#18212a;position:sticky;top:0}
h1{font-size:16px;margin:0 0 8px;font-weight:600}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.box{background:#22303c;border-radius:8px;padding:8px 10px}
.lbl{font-size:12px;color:#9fb0c0;text-transform:uppercase;letter-spacing:.05em}
.val{font-size:28px;font-weight:700;font-variant-numeric:tabular-nums}
.good{color:#4ade80}.warn{color:#facc15}.bad{color:#f87171}
#conn{font-size:13px;margin-top:8px}
#log{font:13px/1.45 ui-monospace,monospace;padding:8px 16px;white-space:pre-wrap;word-break:break-all}
#log div{border-bottom:1px solid #1e2a35;padding:2px 0}
.t{color:#7d8f9f}
</style></head><body>
<header>
<h1>ESP-NOW (2.4 GHz) &middot; live</h1>
<div class="grid">
<div class="box"><div class="lbl">Signal (RSSI)</div><div class="val" id="rssi">&ndash;</div></div>
<div class="box"><div class="lbl">Last message</div><div class="val" id="ago">&ndash;</div></div>
<div class="box"><div class="lbl" id="goodL">Received</div><div class="val" id="rx">0</div></div>
<div class="box"><div class="lbl" id="badL">Lost</div><div class="val" id="lost">0</div></div>
<div class="box" style="grid-column:1/3"><div class="lbl">Distance from home (phone GPS)</div><div class="val" id="dist">no GPS yet</div></div>
</div>
<div style="margin-top:8px"><a href="/home" style="color:#9fb0c0;font-size:13px">Set home = my current position</a></div>
<div id="conn">connecting&hellip;</div>
</header>
<div id="log"></div>
<script>
let next=0;
const $=id=>document.getElementById(id);
function hhmmss(){return new Date().toTimeString().slice(0,8)}
async function poll(){
  try{
    const r=await fetch('/data?since='+next,{cache:'no-store'});
    const d=await r.json();
    if(d.next<next){$('log').innerHTML=''}          // board restarted
    next=d.next;
    const t=hhmmss();
    for(const line of d.lines){
      const div=document.createElement('div');
      div.innerHTML='<span class="t">'+t+'</span> ';
      div.appendChild(document.createTextNode(line));
      $('log').prepend(div);
    }
    while($('log').children.length>300)$('log').lastChild.remove();
    const pct=d.rx+d.lost>0?Math.round(100*d.lost/(d.rx+d.lost)):0;
    $('goodL').textContent=d.good;$('badL').textContent=d.bad;
    $('dist').textContent=!d.gps?'no GPS yet':d.dist<0?'waiting for a good fix ('+d.acc+' m)':d.dist+' m  (±'+d.acc+' m)';
    $('rx').textContent=d.rx;
    $('lost').textContent=d.lost+' ('+pct+'%)';
    $('lost').className='val '+(pct==0?'good':pct<10?'warn':'bad');
    if(d.ago<0){$('rssi').textContent='–';$('ago').textContent='none yet'}
    else{
      $('rssi').textContent=d.rssi+' dBm';
      $('rssi').className='val '+(d.rssi>-85?'good':d.rssi>-95?'warn':'bad');
      $('ago').textContent=Math.round(d.ago)+' s ago';
      $('ago').className='val '+(d.ago<2.5?'good':d.ago<5?'warn':'bad');
    }
    $('conn').textContent='connected to board · '+t;
    $('conn').className='good';
  }catch(e){
    $('conn').textContent='board not reachable · '+hhmmss();
    $('conn').className='bad';
  }
  setTimeout(poll,1000);
}
poll();
</script></body></html>)HTML";

void webBegin() {
  if (strcmp(WIFI_SSID, "PUT_HOTSPOT_NAME_HERE") == 0) {
    Serial.println("Wi-Fi: secrets.h still has the placeholder name. Type in your hotspot name and password, then upload again.");
  }
  WiFi.mode(WIFI_STA);
  // When the hotspot refuses us, print the reason code it gives.
  // Common: 15 = handshake timeout (usually wrong password), 201 = network not found,
  // 202 = authentication failed, 2/4 = timed out (often weak signal).
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
    Serial.printf("Wi-Fi: refused, reason %d\n", info.wifi_sta_disconnected.reason);
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.setHostname(HOSTNAME);
  WiFi.setAutoReconnect(true);               // rejoin by itself if the hotspot drops
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("Wi-Fi: joining hotspot \"%s\" (radio keeps working meanwhile)\n", WIFI_SSID);

  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/data", handleData);
  server.on("/loc", handleLoc);
  server.on("/home", handleHome);
  server.begin();
}

void webLoop() {
  bool now = WiFi.status() == WL_CONNECTED;
  if (now && !online) {
    String ip = WiFi.localIP().toString();
    Serial.printf("Wi-Fi: connected. On the phone open  http://%s/\n", ip.c_str());
    String msg = "Wi-Fi connected, address http://" + ip + "/";
    webAdd(msg.c_str());
  } else if (!now && online) {
    Serial.println("Wi-Fi: lost the hotspot, retrying...");
  }
  online = now;
  server.handleClient();
}

#endif  // WEB_LOG
