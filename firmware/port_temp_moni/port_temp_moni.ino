#include <EEPROM.h>
#include <SPI.h>
#include "DHT.h"
#include "Timer.h"
#include "LowPower.h"

Timer pow_timer;
#define TIMER_EXP   (1 * 60000)
#define MAX_TEMP    22
#define MIN_TEMP    18

#define SLEEP_COUNT     225 //30mins ish
//#define SLEEP_COUNT       5 //5mins ish 

#define EEPROM_ADDR       0

/*wifi module*/
#define WIFI_RESET_PIN    4
#define POWER_PIN         5

#define ACT_PIN           13

#define ENABLE_NETWORK
#define PRINT_ENV
//#define ALARM

#define HIGH_TMP    25
#define LOW_TMP     17
#define DHTPIN      8   // what digital pin we're connected to


typedef struct {
  float temp;
  float humid;
  bool changed;
  bool alarm;
  bool buzzer;
  uint16_t sleep_count;
} ENV_DATA;
volatile ENV_DATA env_data;

/*temp hum sensor*/
DHT dht(DHTPIN, DHT11);

void setup() {

  /*The serial pins will now be used by the wifi module so serial debug is no
    longer an option in this project*/
  pinMode(ACT_PIN, OUTPUT);
  digitalWrite(ACT_PIN, HIGH);
  Serial.begin(115200);
  ///Serial.println("begin");

  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, HIGH);
  //setup buzzer
  pinMode(A3, OUTPUT);
  //Set low for grounding buzzer
  digitalWrite(A3, LOW);
  //analogWrite(A0, 0);

  buzzer(true);
  delay(100);
  buzzer(false);
  delay(100);
  buzzer(true);
  delay(100);
  buzzer(false);

  dht.begin();
  env_data.temp = 0;
  env_data.humid = 0;
  env_data.changed = false;
  env_data.alarm = false;
  env_data.buzzer = false;
  env_data.sleep_count = 0;
  /*init wifi then do display stuff while the wifi settles*/
  //init_wifi();

  connect_wifi();
  delay(3000);
  digitalWrite(ACT_PIN, LOW);
}


void loop() {

  if (go_to_low_power() == true)
  {
    digitalWrite(ACT_PIN, HIGH);
    get_env_data();
    report_env();
    //check_alarm();
    digitalWrite(ACT_PIN, LOW);
  }
}


bool go_to_low_power()
{
  /*Turn off wifi*/
  digitalWrite(POWER_PIN, LOW);

  //low power
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);

  if (env_data.sleep_count >= SLEEP_COUNT)
  {
    env_data.sleep_count = 0;
    return true;
  } else {
    env_data.sleep_count++;
    return false;
  }
}

void check_alarm()
{
  /*
    HIGH_TMP    25
    LOW_TMP     17
  */

  if ( (env_data.temp >= HIGH_TMP) || (env_data.temp <= LOW_TMP) ) {
    env_data.alarm = true;
  } else {
    env_data.alarm = false;
  }

  /*Do the buzzer here - seems a little pointless but we also use the alarm for net traffic*/
  if (env_data.alarm)
  {
#ifdef ALARM
    buzzer(env_data.buzzer);
    if (env_data.buzzer)
    {
      env_data.buzzer = false;
    } else {
      env_data.buzzer = true;
    }
#endif
  }
}

void report_env()
{
  init_wifi();
  connect_wifi();
  /*sends the data down wifi*/
  send_get_data();
}

float tmp, hmd;
void get_env_data()
{
  hmd = dht.readHumidity();
  tmp = dht.readTemperature();
  if ( (tmp != env_data.temp) || (hmd != env_data.humid) )
  {
    Serial.println("changed");
    env_data.humid = hmd;
    env_data.temp = tmp;
    env_data.changed = true;
  } else {
    Serial.println("not changed");
  }
}

void init_wifi()
{
  digitalWrite(POWER_PIN, HIGH);
  delay(500);
  /*Reset the module*/
  pinMode(WIFI_RESET_PIN, OUTPUT);
  digitalWrite(WIFI_RESET_PIN, LOW); /*start reset sequence*/
  delay(500);
  digitalWrite(WIFI_RESET_PIN, HIGH); /*start reset sequence*/
  /*wait for bootup*/
  delay(2000);
  /*Do restore*/
  Serial.write("AT+RESTORE\r\n");
  /*wait for bootup*/
  delay(2000);
}

void connect_wifi()
{
  Serial.write("AT+CWMODE=3\r\n");
  delay(1000);
  Serial.write("AT+CWJAP=\"Fudgemesh\",\"ancienthill347\"\r\n");
  /*wait for the connection*/
  delay(10000);
}

void send_get_data()
{
  /*
    AT+CIPSTART="TCP","shotlu.usrs0.com",80
    AT+CIPSEND=79 (length of the url)
    >GET /receiver.php?apples=56&oranges=23 HTTP/1.1\r\nHost: shotlu.usrs0.com\r\n\r\n
  */
  int t = env_data.temp;
  int h = env_data.humid;
  String tmpstr = String(t);
  String hmstr = String(h);
  if(t < 10){
    tmpstr = "00" + tmpstr;
  }else{
    tmpstr = "0" + tmpstr;
  }
  if(h < 10){
    hmstr = "00" + hmstr;
  }else{
    hmstr = "0" + hmstr;
  }
  //http://attentii.co.uk/leeweb/tempmon/tmpsub.php?tmhd=0010000023050

  Serial.write("AT+CIPSTART=\"TCP\",\"attentii.co.uk\",80\r\n");
  delay(3000);
  String para = "GET /leeweb/tempmon/tmpsub.php?tmhd=0010000" + tmpstr + hmstr + " HTTP/1.1\r\nHost: attentii.co.uk\r\nConnection: close\r\n\r\n";

  Serial.println("AT+CIPSEND=" + String(para.length()));
  delay(250);
  Serial.print(para);
  delay(5000);
}

void buzzer(bool state)
{
  if (state) {
    //analogWrite(A0, 128);
  } else {
    //analogWrite(A0, 0);
  }
}
