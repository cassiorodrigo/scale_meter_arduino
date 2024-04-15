const char* ssid = "SSID";         // Enter your WiFi SSID
const char* password = "PASSWORD";  // Enter your WiFi password

#include <WiFi.h>
#include <WebServer.h>
#include "HX711.h"
/*
const char* ssid = "YourSSID";
const char* password = "YourPassword";
*/
WebServer server(80);

HX711 scale;

float emptyWeight = 15.0; // Empty bottle weight
float fullWeight = 26.0; // Full bottle weight
float calibrationValue = 19000.0; // Default calibration value
int D1 = 4;
int D2 = 5;


float minWeight = 0.0; // Default minimum weight
float maxWeight = 100.0; // Default maximum weight

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  scale.begin(D1, D2); // Initialize the HX711
  scale.set_scale(calibrationValue); // Apply calibration value
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/calibration", HTTP_GET, handleCalibrationPage);
  server.on("/tare", HTTP_GET, handleTare);
  server.on("/calibrate", HTTP_GET, handleCalibrate);
  server.on("/setlimits", HTTP_GET, handleSetLimitsPage);
  server.on("/setlimits", HTTP_POST, handleSetLimits);
  server.on("/data", HTTP_GET, handleData);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

String getIndexHtml(float weight, float percentage, float calibration) {
  String color;
  if (percentage >= 0 && percentage <= 100) {
    int red = (int)(255 * (percentage / 100.0));
    int green = (int)(255 * ((100 - percentage) / 100.0));
    color = "rgb(" + String(red) + "," + String(green) + ", 255)";
  } else if (percentage < 0) {
    color = "rgb(0, 255, 255)";
  } else {
    color = "rgb(255, 0, 0)";
  }
  
  return String("<!DOCTYPE html>\n") +
         "<html>\n" +
         "<head>\n" +
         "<title>Gas Bottle Monitor</title>\n" +
         "<style>\n" +
         "body { background-color: #f0f8ff; font-family: Arial, sans-serif; text-align: center; }\n" +
         "h1 { color: #4169e1; }\n" +
         "p { color: #4169e1; }\n" +
         "button { background-color: #4169e1; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }\n" +
         "button:hover { background-color: #3558cc; }\n" +
         "#progress { background-color: " + color + "; height: 20px; border-radius: 5px; box-shadow: 2px 2px 5px #888888; }\n" +
         ".container { margin: 0 auto; max-width: 600px; }\n" +
         "</style>\n" +
         "<script>\n" +
         "function updateData() {\n" +
         "  var xhttp = new XMLHttpRequest();\n" +
         "  xhttp.onreadystatechange = function() {\n" +
         "    if (this.readyState == 4 && this.status == 200) {\n" +
         "      var data = this.responseText.split(';');\n" +
         "      var weight = parseFloat(data[0]);\n" +
         "      if (weight > 50) weight /= 1000; // Convert to kg if weight is in grams\n" +
         "      var percentage = parseFloat(data[1]);\n" +
         "      if (weight < 0) weight = 0;\n" +
         "      if (percentage < 0) percentage = 0;\n" +
         "      document.getElementById('weight').innerHTML = 'Weight: ' + weight.toFixed(1) + ' kg';\n" +
         "      document.getElementById('percentage').innerHTML = 'Percentage: ' + percentage.toFixed(1) + ' %';\n" +
         "      document.getElementById('progress').style.width = Math.min(percentage, 100).toFixed(1) + '%';\n" +
         "      document.getElementById('calibration').innerHTML = 'Calibration Value: ' + data[2];\n" +
         "    }\n" +
         "  };\n" +
         "  xhttp.open('GET', '/data', true);\n" +
         "  xhttp.send();\n" +
         "}\n" +
         "setInterval(updateData, 1000);\n" +
         "function tare() {\n" +
         "  var xhttp = new XMLHttpRequest();\n" +
         "  xhttp.open('GET', '/tare', true);\n" +
         "  xhttp.send();\n" +
         "}\n" +
         "</script>\n" +
         "</head>\n" +
         "<body onload='updateData()'>\n" +
         "<div class='container'>\n" +
         "<h1>Gas Bottle Monitor</h1>\n" +
         "<p id='weight'>Weight: " + String(weight, 1) + " kg</p>\n" +
         "<p id='percentage'>Percentage: " + String(percentage, 1) + " %</p>\n" +
         "<div style='background-color: #ddd; width: 100%;'>\n" +
         "<div id='progress'></div>\n" +
         "</div>\n" +
         "<p id='calibration'>Calibration Value: " + String(calibration, 1) + "</p>\n" +
         "<button onclick='tare()'>Tare</button>\n" +
         "<button onclick='window.location.href=\"/calibration\"'>Calibration Page</button>\n" +
         "<button onclick='window.location.href=\"/setlimits\"'>Set Limits</button>\n" +
         "</div>\n" +
         "</body>\n" +
         "</html>\n";
}

void handleRoot() {
  float currentWeight = scale.get_units() - emptyWeight;
  float percentage = ((currentWeight - minWeight) / (maxWeight - minWeight)) * 100;
  if (currentWeight < 0) currentWeight = 0;
  if (percentage < 0) percentage = 0;
  server.send(200, "text/html", getIndexHtml(currentWeight, percentage, calibrationValue));
}

void handleCalibrationPage() {
  String calibrationPage = "<!DOCTYPE html>\n";
  calibrationPage += "<html>\n";
  calibrationPage += "<head>\n";
  calibrationPage += "<title>Calibration</title>\n";
  calibrationPage += "<style>\n";
  calibrationPage += "body { background-color: #f0f8ff; font-family: Arial, sans-serif; text-align: center; }\n";
  calibrationPage += "h1 { color: #4169e1; }\n";
  calibrationPage += "p { color: #4169e1; }\n";
  calibrationPage += "button { background-color: #32CD32; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }\n";
  calibrationPage += "button:hover { background-color: #228B22; }\n";
  calibrationPage += "</style>\n";
  calibrationPage += "</head>\n";
  calibrationPage += "<body>\n";
  calibrationPage += "<div class='container'>\n";
  calibrationPage += "<h1>Calibration Page</h1>\n";
  calibrationPage += "<p>Current Calibration Value: " + String(calibrationValue, 1) + "</p>\n";
  calibrationPage += "<form action='/calibrate' method='GET'>\n";
  calibrationPage += "Calibrate Value: <input type='text' name='value' value='" + String(calibrationValue, 1) + "'><br><br>\n";
  calibrationPage += "<input type='submit' value='Submit'>\n";
  calibrationPage += "</form>\n";
  calibrationPage += "<button onclick='window.location.href=\"/\"'>Back to Main Page</button>\n";
  calibrationPage += "</div>\n";
  calibrationPage += "</body>\n";
  calibrationPage += "</html>\n";
  server.send(200, "text/html", calibrationPage);
}

void handleCalibrate() {
  if (server.hasArg("value")) {
    calibrationValue = server.arg("value").toFloat();
    scale.set_scale(calibrationValue);
  }
  server.sendHeader("Location", String("/"), true);
  server.send(303);
}

void handleSetLimitsPage() {
  String setLimitsPage = "<!DOCTYPE html>\n";
  setLimitsPage += "<html>\n";
  setLimitsPage += "<head>\n";
  setLimitsPage += "<title>Set Limits</title>\n";
  setLimitsPage += "<style>\n";
  setLimitsPage += "body { background-color: #f0f8ff; font-family: Arial, sans-serif; text-align: center; }\n";
  setLimitsPage += "h1 { color: #4169e1; }\n";
  setLimitsPage += "p { color: #4169e1; }\n";
  setLimitsPage += "button { background-color: #32CD32; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }\n";
  setLimitsPage += "button:hover { background-color: #228B22; }\n";
  setLimitsPage += "</style>\n";
  setLimitsPage += "</head>\n";
  setLimitsPage += "<body>\n";
  setLimitsPage += "<div class='container'>\n";
  setLimitsPage += "<h1>Set Limits Page</h1>\n";
  setLimitsPage += "<p>Current Minimum Weight: " + String(minWeight, 1) + " kg</p>\n";
  setLimitsPage += "<p>Current Maximum Weight: " + String(maxWeight, 1) + " kg</p>\n";
  setLimitsPage += "<form action='/setlimits' method='POST'>\n";
  setLimitsPage += "Minimum Weight: <input type='text' name='min' value='" + String(minWeight, 1) + "'><br><br>\n";
  setLimitsPage += "Maximum Weight: <input type='text' name='max' value='" + String(maxWeight, 1) + "'><br><br>\n";
  setLimitsPage += "<input type='submit' value='Submit'>\n";
  setLimitsPage += "</form>\n";
  setLimitsPage += "<button onclick='window.location.href=\"/\"'>Back to Main Page</button>\n";
  setLimitsPage += "</div>\n";
  setLimitsPage += "</body>\n";
  setLimitsPage += "</html>\n";
  server.send(200, "text/html", setLimitsPage);
}

void handleSetLimits() {
  if (server.hasArg("min") && server.hasArg("max")) {
    minWeight = server.arg("min").toFloat();
    maxWeight = server.arg("max").toFloat();
  }
  server.sendHeader("Location", String("/"), true);
  server.send(303);
}

void handleData() {
  float currentWeight = scale.get_units() - emptyWeight;
  if (currentWeight > 50) currentWeight /= 1000; // Convert to kg if weight is in grams
  float percentage = ((currentWeight - minWeight) / (maxWeight - minWeight)) * 100;
  if (currentWeight < 0) currentWeight = 0;
  if (percentage < 0) percentage = 0;
  server.send(200, "text/plain", String(currentWeight, 1) + ";" + String(percentage, 1) + ";" + String(calibrationValue, 1));
}

void handleTare() {
  scale.tare();
  server.sendHeader("Location", String("/"), true);
  server.send(303);
}
