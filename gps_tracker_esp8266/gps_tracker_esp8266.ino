// GPS location web page (NodeMCU ESP8266 + u-blox NEO-M8N).
//
// Reads NMEA sentences from the GPS module, extracts latitude, longitude, date
// and time with TinyGPS++, and serves them on a small web page in the local
// network with a link that opens the bin's position in Google Maps.
//
// Wiring: GPS TX -> D2 (GPIO4), GPS RX -> D1 (GPIO5), VCC 3.3-5 V, GND.
// Libraries: TinyGPSPlus, EspSoftwareSerial (bundled with the ESP8266 core).

#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

const char* WIFI_SSID = "your-wifi";
const char* WIFI_PASS = "your-password";

const int UTC_OFFSET_MINUTES = 120;  // Egypt (EET, UTC+2); change for your time zone

TinyGPSPlus gps;
SoftwareSerial ss(4, 5);  // RX, TX
WiFiServer server(80);

String lat_str = "-", lng_str = "-", date_str = "-", time_str = "-";

void setup() {
  Serial.begin(115200);
  ss.begin(9600);  // NEO-M8N default baud rate

  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  server.begin();
  Serial.println("Server started");
  Serial.println(WiFi.localIP());  // open this address in a browser
}

String twoDigits(int v) { return v < 10 ? "0" + String(v) : String(v); }

void readGps() {
  while (ss.available() > 0) {
    if (!gps.encode(ss.read())) continue;

    if (gps.location.isValid()) {
      lat_str = String(gps.location.lat(), 6);
      lng_str = String(gps.location.lng(), 6);
    }
    if (gps.date.isValid()) {
      date_str = twoDigits(gps.date.day()) + " / " + twoDigits(gps.date.month()) + " / " +
                 String(gps.date.year());
    }
    if (gps.time.isValid()) {
      // GPS time is UTC: shift to local time and show it as 12-hour time.
      int minutes = gps.time.hour() * 60 + gps.time.minute() + UTC_OFFSET_MINUTES;
      minutes = (minutes + 24 * 60) % (24 * 60);
      int hour = minutes / 60;
      const bool pm = hour >= 12;
      hour = hour % 12;
      if (hour == 0) hour = 12;
      time_str = twoDigits(hour) + ":" + twoDigits(minutes % 60) + ":" +
                 twoDigits(gps.time.second()) + (pm ? " PM" : " AM");
    }
  }
}

String page() {
  String s =
      "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nRefresh: 5\r\n\r\n"
      "<!DOCTYPE html><html><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width, initial-scale=1'>"
      "<title>Smart Bin Location</title>"
      "<style>body{font-family:sans-serif;text-align:center}"
      "table{margin:auto;border-collapse:collapse;width:90%;max-width:420px}"
      "th,td{border:1px solid #333;padding:8px}a{font-size:125%;color:#c00}</style>"
      "</head><body><h1>Smart Bin Location</h1><table>";
  s += "<tr><th>Latitude</th><td>" + lat_str + "</td></tr>";
  s += "<tr><th>Longitude</th><td>" + lng_str + "</td></tr>";
  s += "<tr><th>Date</th><td>" + date_str + "</td></tr>";
  s += "<tr><th>Time</th><td>" + time_str + "</td></tr></table>";
  if (gps.location.isValid()) {
    s += "<p><a href='https://maps.google.com/maps?q=" + lat_str + "," + lng_str +
         "'>Open in Google Maps</a></p>";
  } else {
    s += "<p>Waiting for a GPS fix...</p>";
  }
  s += "</body></html>";
  return s;
}

void loop() {
  readGps();

  WiFiClient client = server.available();
  if (!client) return;

  // Read and discard the request line and headers.
  const unsigned long start = millis();
  while (client.connected() && millis() - start < 1000) {
    if (client.available() && client.readStringUntil('\n') == "\r") break;
  }
  client.print(page());
  delay(1);
  client.stop();
}
