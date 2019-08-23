#include <WiFi.h>

const char* ssid     = "fudgeNet2G";
const char* password = "ancienthill347";

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */

const char* host = "www.attentii.co.uk";

typedef struct {
  float temp;
  float humid;
  bool changed;
  bool alarm;
  bool buzzer;
} ENV_DATA;
static ENV_DATA env_data;


void setup() {

  Serial.begin(9600);
  delay(10);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    /*Good to show this with a LED maybe?*/
  }

  Serial.println("starting");
}

void loop() {

  sendTempData();

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void sendTempData()
{
  int t = env_data.temp;
  int h = env_data.humid;
  String tmpstr = String(t);
  String hmstr = String(h);
  tmpstr = "0" + tmpstr;
  hmstr = "0" + hmstr;

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  /*Use this to send the data*/
  String para = "leeweb/tempmon/tmpsub.php?tmhd=" + tmpstr + hmstr + " HTTP/1.1\r\nHost: attentii.co.uk\r\nConnection: close\r\n\r\n";

  Serial.println(para);

  // This will send the request to the server
  client.print(String("GET /") + para);

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

}
