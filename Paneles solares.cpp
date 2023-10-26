#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>

const char* ssid = "empanada";
const char* password = "1234567890";

const int voltagePin = 34;
const int currentPin = 35;
const int ledPin = 2;
const int webSocketPort = 81;

float voltage = 0.0;
float current = 0.0;
float power = 0.0;

float maxVoltage = 0.0;
float maxCurrent = 0.0;
float maxPower = 0.0;

AsyncWebServer server(80);
WebSocketsServer webSocket(webSocketPort);

void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Desconectado!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Conectado desde %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] Recibido: %s\n", num, payload);
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a la red Wi-Fi...");
  }

  Serial.println("Conexión Wi-Fi establecida");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  webSocket.begin();
  webSocket.onEvent(handleWebSocketEvent);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String respuesta = "<html><head><title>Bumblebee recolector de datos</title><style>body { background-color: black; color: yellow; } h1 { text-align: center; } h2 { text-align: center; }</style><script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script></head><body><h1>Bumblebee recolector de datos</h1><h2>Voltaje: <span id=\"voltage\">0.0</span> V</h2><h2>Corriente: <span id=\"current\">0.0</span> A</h2><h2>Potencia: <span id=\"power\">0.0</span> W</h2><h2>Valores maximos:</h2><h2>Voltaje maximo: <span id=\"maxVoltage\">0.0</span> V</h2><h2>Corriente maxima: <span id=\"maxCurrent\">0.0</span> A</h2><h2>Potencia maxima: <span id=\"maxPower\">0.0</span> W</h2><canvas id=\"chart\"></canvas><script>var chartData = {labels: [], voltageData: [], currentData: [], powerData: []}; var ctx = document.getElementById('chart').getContext('2d'); var chart = new Chart(ctx, {type: 'line', data: {labels: chartData.labels, datasets: [{label: 'Voltaje (V)', data: chartData.voltageData, borderColor: 'yellow', fill: false, tension: 0.1}, {label: 'Corriente (A)', data: chartData.currentData, borderColor: 'orange', fill: false, tension: 0.1}, {label: 'Potencia (W)', data: chartData.powerData, borderColor: 'green', fill: false, tension: 0.1}]}, options: { scales: { x: { display: true, grid: { color: 'yellow' }, ticks: { color: 'yellow' } }, y: { display: true, grid: { color: 'yellow' }, ticks: { color: 'yellow' } } }, legend: { labels: { color: 'yellow' } } }}); var socket = new WebSocket('ws://' + window.location.hostname + ':81/'); socket.onmessage = function(event) { var data = JSON.parse(event.data); document.getElementById('voltage').textContent = data.voltage; document.getElementById('current').textContent = data.current; document.getElementById('power').textContent = data.power; updateChart(data); updateMaxValues(data); }; function updateChart(data) { chartData.labels.push(new Date().toLocaleTimeString()); chartData.voltageData.push(data.voltage); chartData.currentData.push(data.current); chartData.powerData.push(data.power); chart.update(); } function updateMaxValues(data) { if (data.voltage > parseFloat(document.getElementById('maxVoltage').textContent)) { document.getElementById('maxVoltage').textContent = data.voltage; } if (data.current > parseFloat(document.getElementById('maxCurrent').textContent)) { document.getElementById('maxCurrent').textContent = data.current; } if (data.power > parseFloat(document.getElementById('maxPower').textContent)) { document.getElementById('maxPower').textContent = data.power; } }</script></body></html>";
    request->send(200, "text/html", respuesta);
  });

  server.begin();
}

void loop() {
  voltage = readVoltage();
  current = readCurrent();
  power = calculatePower(voltage, current);

  // Enviar los datos actualizados a través de WebSockets
  String json = "{\"voltage\": " + String(voltage) + ", \"current\": " + String(current) + ", \"power\": " + String(power) + "}";
  webSocket.broadcastTXT(json);
  
  webSocket.loop();
  delay(1000);
}

float readVoltage() {
  int rawValue = analogRead(voltagePin);
  float voltage = rawValue * (3.3 / 4095.0);
  return voltage;
}

float readCurrent() {
  int rawValue = analogRead(currentPin);
  float current = rawValue * (3.3 / 4095.0);
  return current;
}

float calculatePower(float voltage, float current) {
  float power = voltage * current;
  return power;
}