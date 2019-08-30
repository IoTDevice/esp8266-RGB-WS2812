//MIT origin example:https://github.com/kitesurfer1404/WS2812FX/tree/master/examples/esp8266_webinterface
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WS2812FX.h>
#include <ESP8266mDNS.h>

extern const char index_html[];
extern const char main_js[];

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define LED_PIN 1                       // 0 = GPIO0, 2=GPIO2
#define LED_COUNT 4

#define WIFI_TIMEOUT 30000              // checks WiFi every ...ms. Reset after this time, if WiFi cannot reconnect.
#define HTTP_PORT 80

#define DEFAULT_COLOR 0xFF5900
#define DEFAULT_BRIGHTNESS 255
#define DEFAULT_SPEED 1000
#define DEFAULT_MODE FX_MODE_STATIC

unsigned long auto_last_change = 0;
unsigned long last_wifi_check_time = 0;
String modes = "";
String modes_json = "";
uint8_t myModes[] = {}; // *** optionally create a custom list of effect/mode numbers
boolean auto_cycle = false;

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server(HTTP_PORT);

String deviceName = "RGB灯";
String version = "1.0";
const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

void wifi_setup() {
  WiFi.mode(WIFI_STA);
  WiFi.beginSmartConfig();
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void modes_setup() {
  modes = "";
  uint8_t num_modes = sizeof(myModes) > 0 ? sizeof(myModes) : ws2812fx.getModeCount();
  for(uint8_t i=0; i < num_modes; i++) {
    uint8_t m = sizeof(myModes) > 0 ? myModes[i] : i;
    modes += "<li><a href='#' class='m' id='";
    modes += m;
    modes += "'>";
    modes += ws2812fx.getModeName(m);
    modes += "</a></li>";
  }
}

void modes_setup_json() {
  modes_json = "{";
  uint8_t num_modes = sizeof(myModes) > 0 ? sizeof(myModes) : ws2812fx.getModeCount();
  for(uint8_t i=0; i < num_modes; i++) {
    uint8_t m = sizeof(myModes) > 0 ? myModes[i] : i;
    if(i==0){
      modes_json = modes_json + "\"" + ws2812fx.getModeName(m) + "\":" + m;
    }else{
      modes_json = modes_json + ",\"" + ws2812fx.getModeName(m) + "\":" + m;
    }
  }
  modes_json += "}";
}

void srv_handle_index_html() {
  server.send_P(200,"text/html", index_html);
}

void srv_handle_main_js() {
  server.send_P(200,"application/javascript", main_js);
}

void srv_handle_not_found() {
  server.send(404, "text/plain", "File Not Found");
}

void srv_handle_modes() {
  server.send(200,"text/plain", modes);
}

void srv_handle_modes_json() {
  server.send(200,"application/json", modes_json);
}

void srv_handle_set() {
  for (uint8_t i=0; i < server.args(); i++){
    if(server.argName(i) == "c") {
      uint32_t tmp = (uint32_t) strtol(server.arg(i).c_str(), NULL, 16);
      if(tmp >= 0x000000 && tmp <= 0xFFFFFF) {
        ws2812fx.setColor(tmp);
      }
    }

    if(server.argName(i) == "m") {
      uint8_t tmp = (uint8_t) strtol(server.arg(i).c_str(), NULL, 10);
      ws2812fx.setMode(tmp % ws2812fx.getModeCount());
      Serial.print("mode is "); Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
    }

    if(server.argName(i) == "b") {
      if(server.arg(i)[0] == '-') {
        ws2812fx.setBrightness(ws2812fx.getBrightness() * 0.8);
      } else if(server.arg(i)[0] == ' ') {
        ws2812fx.setBrightness(min(max(ws2812fx.getBrightness(), 5) * 1.2, 255));
      } else { // set brightness directly
        uint8_t tmp = (uint8_t) strtol(server.arg(i).c_str(), NULL, 10);
        ws2812fx.setBrightness(tmp);
      }
      Serial.print("brightness is "); Serial.println(ws2812fx.getBrightness());
    }

    if(server.argName(i) == "s") {
      if(server.arg(i)[0] == '-') {
        ws2812fx.setSpeed(max(ws2812fx.getSpeed(), 5) * 1.2);
      } else {
        ws2812fx.setSpeed(ws2812fx.getSpeed() * 0.8);
      }
      Serial.print("speed is "); Serial.println(ws2812fx.getSpeed());
    }

    if(server.argName(i) == "a") {
      if(server.arg(i)[0] == '-') {
        auto_cycle = false;
      } else {
        auto_cycle = true;
        auto_last_change = 0;
      }
    }
  }
  server.send(200, "text/plain", "OK");
}

// 设备改名的API
void handleDeviceRename(){
  String message = "{\"code\":0,\"message\":\"success\"}";
  for (uint8_t i=0; i<server.args(); i++){
    if (server.argName(i)=="name")
    {
      deviceName = server.arg(i);
    }
  }
  server.send(200, "application/json", message);
}

// 当前的LED开关状态API
void handleCurrentLEDStatus(){
  String message;
  message = "{\"auto_cycle\":"+String(auto_cycle)+
  ",\"name\":\""+String(deviceName)+
  "\",\"code\":0,\"message\":\"success\"}";
  server.send(200, "application/json", message);
}
// 设备信息
void handleDeviceInfo(){
  String message;
  message = "{\n";
  message += "\"name\":\""+deviceName +"\",\n";
  message += "\"model\":\"com.iotserv.devices.rgbaLed\",\n";
  message += "\"mac\":\""+WiFi.macAddress()+"\",\n";
  message += "\"id\":\""+String(ESP.getFlashChipId())+"\",\n";
  message += "\"ui-support\":[\"web\",\"native\"],\n";
  message += "\"ui-first\":\"native\",\n";
  message += "\"author\":\"Farry\",\n";
  message += "\"email\":\"newfarry@126.com\",\n";
  message += "\"home-page\":\"https://github.com/iotdevice\",\n";
  message += "\"firmware-respository\":\"https://github.com/iotdevice/esp8266-RGB-WS2812\",\n";
  message += "\"firmware-version\":\""+version+"\"\n";
  message +="}";
  server.send(200, "application/json", message);
}

void setup(){
  Serial.begin(115200);

  modes.reserve(5000);
  modes_setup();
  modes_setup_json();

  Serial.println("WS2812FX setup");
  ws2812fx.init();
  ws2812fx.setMode(DEFAULT_MODE);
  ws2812fx.setColor(DEFAULT_COLOR);
  ws2812fx.setSpeed(DEFAULT_SPEED);
  ws2812fx.setBrightness(DEFAULT_BRIGHTNESS);
  ws2812fx.start();

  wifi_setup();
 
  if (MDNS.begin("RGB-"+String(ESP.getFlashChipId()))) {
    // Serial.println("MDNS responder started");
  }

  MDNS.addService("iotdevice", "tcp", HTTP_PORT);

  Serial.println("HTTP server setup");
  server.on("/", srv_handle_index_html);
  server.on("/main.js", srv_handle_main_js);
  server.on("/modes", srv_handle_modes);
  server.on("/modes", srv_handle_modes_json);
  server.on("/set", srv_handle_set);
  server.on("/rename", handleDeviceRename);
  server.on("/status", handleCurrentLEDStatus);
  // about this device
  server.on("/info", handleDeviceInfo);

  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "{\"code\":1,\"message\":\"fail\"}" : "{\"code\":0,\"message\":\"success\"}");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      WiFiUDP::stopAll();
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        
      } else {
        
      }
    }
    yield();
  });

  server.on("/ota", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });

  server.onNotFound(srv_handle_not_found);
  server.begin();
}


void loop() {
  unsigned long now = millis();
  MDNS.update();
  server.handleClient();
  ws2812fx.service();

  if(now - last_wifi_check_time > WIFI_TIMEOUT) {
    Serial.print("Checking WiFi... ");
    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost. Reconnecting...");
      wifi_setup();
    } else {
      Serial.println("OK");
    }
    last_wifi_check_time = now;
  }

  if(auto_cycle && (now - auto_last_change > 10000)) { // cycle effect mode every 10 seconds
    uint8_t next_mode = (ws2812fx.getMode() + 1) % ws2812fx.getModeCount();
    if(sizeof(myModes) > 0) { // if custom list of modes exists
      for(uint8_t i=0; i < sizeof(myModes); i++) {
        if(myModes[i] == ws2812fx.getMode()) {
          next_mode = ((i + 1) < sizeof(myModes)) ? myModes[i + 1] : myModes[0];
          break;
        }
      }
    }
    ws2812fx.setMode(next_mode);
    Serial.print("mode is "); Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
    auto_last_change = now;
  }
}
