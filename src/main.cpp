#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WS2812FX.h>

#define LED_COUNT 4
#define LED_PIN 12

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

const int httpPort = 80;
String deviceName = "WS2812RGB灯";
String version = "1.0";
ESP8266WebServer server(httpPort);
const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

// led的开关状态
bool LedStatus = true;
// 当前颜色
uint32_t color = 100;
// Led的亮度
uint8_t brightness = 100;
uint8_t mode = 12;
uint16_t speed = 1000;
// web服务器的根目录
void handleRoot() {
  server.send(200, "text/html", "<h1>this is index page from esp8266!</h1>");
}
// 操作LED开关状态的API
void handleStatusChange(){
  String message = "{\"code\":0,\"message\":\"success\"}";
  for (uint8_t i=0; i<server.args(); i++){
    if (server.argName(i)=="color")
    {
      ws2812fx.setColor(uint32_t(server.arg(i).toInt()));
    }else if (server.argName(i)=="mode"){
      ws2812fx.setMode(uint8_t(server.arg(i).toInt()));
    }else if (server.argName(i)=="LedStatus"){
      if (server.arg(i)=="on"){
        ws2812fx.start();
        LedStatus = true;
      }else if (server.arg(i)=="off"){
        ws2812fx.stop();
        LedStatus = false;
      }
    }else if (server.argName(i)=="brightness"){
      ws2812fx.setBrightness(uint8_t(server.arg(i).toInt()));
    }else if (server.argName(i)=="speed"){
      ws2812fx.setSpeed(uint8_t(server.arg(i).toInt()));
    }
  }
  server.send(200, "application/json", message);
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
  message = "{\"color\":"+String(color)+
  ",\"mode\":"+String(mode)+
  ",\"LedStatus\":"+String(LedStatus)+
  ",\"brightness\":"+String(brightness)+
  ",\"speed\":"+String(speed)+
  ",\"code\":0,\"message\":\"success\"}";
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
  message += "\"firmware-respository\":\"https://github.com/iotdevice/phicomm_dc1\",\n";
  message += "\"firmware-version\":\""+version+"\"\n";
  message +="}";
  server.send(200, "application/json", message);
}

// 页面或者api没有找到
void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void){
  ws2812fx.init();
  ws2812fx.setBrightness(3);
  ws2812fx.setSpeed(1000);
  ws2812fx.setMode(1);
  ws2812fx.setColor(RED);
  ws2812fx.start();
  // Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  // 选取一种连接路由器的方式 
  // WiFi.begin(ssid, password);
  WiFi.beginSmartConfig();

  // Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
      ws2812fx.setMode(0);
  }

  if (MDNS.begin("rgb-"+String(ESP.getFlashChipId()))) {
    
  }

  server.on("/", handleRoot);
  server.on("/changeStatus", handleStatusChange);
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

  server.onNotFound(handleNotFound);

  server.begin();
  // Serial.println("HTTP server started");
  MDNS.addService("iotdevice", "tcp", httpPort);
}

void loop(void){
  MDNS.update();
  server.handleClient();
  ws2812fx.service();
}
