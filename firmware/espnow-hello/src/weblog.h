// Live log web page for a phone (sender env).
// The receiver joins your phone's Wi-Fi hotspot and serves a page that shows
// the radio messages as they arrive. In every other env these do nothing.
#pragma once

#ifdef WEB_LOG
void webBegin();                                      // start Wi-Fi + web server
void webLoop();                                       // call often from loop()
void webAdd(const char* line);                        // add one line to the page's log
void webStats(long received, long lost, float rssi);  // update the big numbers
// Latest phone GPS fix (sent by the Automate app to /loc) and its distance from home.
// Returns false if no fix arrived in the last 5 s.
bool webLocation(double& lat, double& lon, int& accuracyM, int& distanceM);
#else
inline void webBegin() {}
inline void webLoop() {}
inline void webAdd(const char*) {}
inline void webStats(long, long, float) {}
inline bool webLocation(double&, double&, int&, int&) { return false; }
#endif
