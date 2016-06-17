#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>

#define SSID "<YOUR-WIFI-SSID>"
#define PASSKEY "<YOUR-WIFI-KEY>"

#define LEFT_FORWARD  D1
#define LEFT_REVERSE  D3

#define RIGHT_FORWARD D2
#define RIGHT_REVERSE D4

#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server = ESP8266WebServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void Set_Speed(String MOTOR, int SPEED) {
    USE_SERIAL.printf("[%u] Speed!\n", SPEED);
    if(MOTOR == "left") {
      
        if(SPEED >= 0) {
            USE_SERIAL.printf("Left Forward!\n");
            digitalWrite(LEFT_REVERSE, LOW);
            analogWrite(LEFT_FORWARD, SPEED);
        } else {
           USE_SERIAL.printf("Left Reverse!\n");
           digitalWrite(LEFT_FORWARD, LOW);
           digitalWrite(LEFT_REVERSE, SPEED * -1);
        }
      } else if(MOTOR == "right") {
      
        if(SPEED >= 0) {
            USE_SERIAL.printf("Right Forward!\n");
            digitalWrite(RIGHT_REVERSE, LOW);
            analogWrite(RIGHT_FORWARD, SPEED);
        } else {
           USE_SERIAL.printf("Right Reverse!\n");
           digitalWrite(RIGHT_FORWARD, LOW);
           digitalWrite(RIGHT_REVERSE, SPEED * -1);
        }
    }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

            // send message to client
            webSocket.sendTXT(num, "Connected");
        }
            break;
        case WStype_TEXT:
            USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);
            String strpayload = (char*)payload;

            if(int location = strpayload.indexOf(':')) {
                String MOTOR = strpayload.substring(0, location);
                String SPEED = strpayload.substring(location+1);

                int intSPEED = SPEED.toInt();
                
                Set_Speed(MOTOR, intSPEED);
            }
            break;
    }

}

void setupWifi(String ssid, String passkey) {
    WiFiMulti.addAP(ssid, passkey);
}

void setup() {
    //USE_SERIAL.begin(921600);
    USE_SERIAL.begin(115200);

    USE_SERIAL.setDebugOutput(true);

    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    pinMode(LEFT_FORWARD, OUTPUT);
    pinMode(LEFT_REVERSE, OUTPUT);
    pinMode(RIGHT_FORWARD, OUTPUT);
    pinMode(RIGHT_REVERSE, OUTPUT);

    analogWrite(LEFT-FORWARD, 0);
    analogWrite(LEFT-REVERSE, 0);
    analogWrite(RIGHT-FORWARD, 0);
    analogWrite(RIGHT-REVERSE, 0);

    setupWifi(SSID, PASSKEY);

    while(WiFiMulti.run() != WL_CONNECTED && millis() < 5000) {
        delay(100);
    }

    // start webSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    if(MDNS.begin("robot")) {
        USE_SERIAL.println("MDNS responder started");
    }

    // handle index
    server.on("/", []() {
        // send index.html
        server.send(200, "text/html", "<html><head><title>Controller</title><script type=\"text/javascript\">function send(n){var o=parseInt(document.getElementById(n).value);console.log(n+\": \"+o.toString()),connection.send(n+\":\"+o.toString())}var connection=new WebSocket(\"ws://\"+location.hostname+\":81/\",[\"arduino\"]);connection.onopen=function(){connection.send(\"#Connect \"+new Date)},connection.onerror=function(n){console.log(\"WebSocket Error \",n)},connection.onmessage=function(n){console.log(\"WebSocket Message \",n.data)},document.addEventListener(\"DOMContentLoaded\",function(n){var o=document.getElementById(\"left\"),e=document.getElementById(\"right\");o.onmousedown=function(n){o.onmousemove=function(){send(\"left\")}},o.onmouseup=function(n){o.value=0,send(\"left\"),o.onmousemove=null},e.onmousedown=function(n){e.onmousemove=function(){send(\"right\")}},e.onmouseup=function(n){e.value=0,send(\"right\"),e.onmousemove=null},o.ontouchstart=function(n){o.ontouchmove=function(){send(\"left\")}},o.ontouchend=function(n){o.value=0,send(\"left\"),o.ontouchmove=null},e.ontouchstart=function(n){e.ontouchmove=function(){send(\"right\")}},e.ontouchend=function(n){e.value=0,send(\"right\"),e.ontouchmove=null}});</script></head><body><input id=\"left\" type=\"range\" min=\"-1023\" max=\"1023\" step=\"1\" value=\"0\" orient=\"vertical\" style=\"writing-mode: bt-lr;-webkit-appearance: slider-vertical; display: inline; float: left; height: 18vh; width: 25%; zoom: 5\" /><input id=\"right\" type=\"range\" min=\"-1023\" max=\"1023\" step=\"1\" value=\"0\" orient=\"vertical\" style=\"writing-mode: bt-lr;-webkit-appearance: slider-vertical; display: inline; float: right; height: 18vh; width: 25%; zoom: 5\" /></body></html>");
    });

    server.begin();

    // Add service to MDNS
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
}

void loop() {
    webSocket.loop();
    server.handleClient();
}
