#pragma once

#include <Arduino.h>

struct SeenDevice {
  String mac;
  String name;
  int rssi;
  unsigned long lastSeenMs;
  const char *knownLabel;  // nullptr if not a known device
};

inline String buildDashboardHtml(const SeenDevice *devices, int count,
                                  bool scanning, unsigned long nowMs) {
  String html;
  html.reserve(2048);

  html += "<!DOCTYPE html><html><head><meta http-equiv=\"refresh\" content=\"3\">";
  html += "<title>ESP32 BLE Scanner</title>";
  html += "<style>"
          "body{font-family:sans-serif;margin:2em;}"
          "table{border-collapse:collapse;width:100%;}"
          "th,td{border:1px solid #ccc;padding:6px 10px;text-align:left;}"
          "tr.known{background:#e6ffe6;font-weight:bold;}"
          ".btn{display:inline-block;padding:8px 16px;margin-right:8px;"
          "text-decoration:none;color:#fff;border-radius:4px;}"
          ".start{background:#2e7d32;} .stop{background:#c62828;}"
          "</style></head><body>";

  html += "<h1>ESP32 BLE Scanner</h1>";
  html += "<p>Status: <strong>";
  html += scanning ? "Scanning ON" : "Scanning OFF (paused)";
  html += "</strong></p>";
  html += "<p><a class=\"btn start\" href=\"/start\">Start</a>"
          "<a class=\"btn stop\" href=\"/stop\">Stop</a></p>";

  html += "<table><tr><th>Name</th><th>MAC</th><th>RSSI</th>"
          "<th>Last seen (s ago)</th><th>Known?</th></tr>";
  for (int i = 0; i < count; i++) {
    const SeenDevice &d = devices[i];
    html += d.knownLabel ? "<tr class=\"known\">" : "<tr>";
    html += "<td>" + d.name + "</td>";
    html += "<td>" + d.mac + "</td>";
    html += "<td>" + String(d.rssi) + "</td>";
    html += "<td>" + String((nowMs - d.lastSeenMs) / 1000) + "</td>";
    html += "<td>" + String(d.knownLabel ? d.knownLabel : "-") + "</td>";
    html += "</tr>";
  }
  html += "</table></body></html>";

  return html;
}
