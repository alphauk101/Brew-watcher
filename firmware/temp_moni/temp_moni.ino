#include <SPI.h>
#include <Ethernet.h>
#include "DHT.h"
#include "LedControl.h"
#include "Timer.h"

Timer pow_timer;
#define TIMER_EXP   (30 * 60000)
#define MAX_TEMP    22
#define MIN_TEMP    18

/*wifi module*/
#define WIFI_RESET_PIN    8

#define POWER_PIN    13

#define DISPLAY
#define ENABLE_NETWORK
#define PRINT_ENV
//#define ALARM

#define HIGH_TMP    25
#define LOW_TMP     17
#define DHTPIN 7     // what digital pin we're connected to

#ifdef DISPLAY
/*
  Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
  pin 2 is connected to the DataIn
  pin 4 is connected to the CLK
  pin 3 is connected to LOAD
  We have only a single MAX72XX.
*/
#define CLK 4
#define LOAD 3
#define DIN 2
LedControl lc = LedControl(DIN, CLK, LOAD, 1);
#endif




typedef struct {
  float temp;
  float humid;
  bool changed;
  bool alarm;
  bool buzzer;
} ENV_DATA;
static ENV_DATA env_data;


/*temp hum sensor*/
DHT dht(DHTPIN, DHT11);

void setup() {

  /*The serial pins will now be used by the wifi module so serial debug is no
    longer an option in this project*/
  Serial.begin(115200);

  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, LOW);
  //setup buzzer
  pinMode(A3, OUTPUT);
  //Set low for grounding buzzer
  digitalWrite(A3, LOW);
  analogWrite(A0, 0);

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

  /*init wifi then do display stuff while the wifi settles*/
  init_wifi();

#ifdef DISPLAY
  lc.shutdown(0, false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0, 8);
  /* and clear the display */
  lc.clearDisplay(0);

  for (int a = 0; a < 8; a++)
  {
    lc.setChar(0, a, '-', false);
    delay(100);
  }
  delay(10000);
  lc.clearDisplay(0);
#endif


  connect_wifi();
  delay(3000);

  /*run this function periodically*/
  pow_timer.every(TIMER_EXP, check_power_state);

   check_power_state();
}


void loop() {

  delay(1000);

  pow_timer.update();
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

void check_power_state()
{
  get_env_data();
  report_env();
  check_alarm();

  /*So if the temp is above the required then switch off if below switch off*/
  if (env_data.temp > MAX_TEMP)
  {
    digitalWrite(POWER_PIN, LOW);
  }

  if (env_data.temp < MIN_TEMP) {
    digitalWrite(POWER_PIN, HIGH);
  }

  /*If we are somewhere between we are happy!*/

}

void report_env()
{
  if (env_data.changed == true)
  {


#ifdef DISPLAY
    display_env();
#endif

    /*sends the data down wifi*/
    send_get_data();

    /*set change flag back so we dont keep doing this*/
    env_data.changed = false;
  }
}

float tmp, hmd;
void get_env_data()
{
  hmd = dht.readHumidity();
  tmp = dht.readTemperature();
  if ( (tmp != env_data.temp) || (hmd != env_data.humid) )
  {
    env_data.humid = hmd;
    env_data.temp = tmp;
    env_data.changed = true;
  }
}

void display_env()
{
  lc.clearDisplay(0);

  //Set tmp
  int tmp = 0;
  tmp = env_data.temp / 10;
  lc.setDigit(0, 6, tmp, false);
  tmp = (int)env_data.temp % 10;
  lc.setDigit(0, 5, tmp, false);
  lc.setChar(0, 4, 'C', false);

  tmp = env_data.humid / 10;
  lc.setDigit(0, 2, tmp, false);
  tmp = (int)env_data.humid % 10;
  lc.setDigit(0, 1, tmp, false);
  lc.setChar(0, 0, 'H', false);

  //lc.setChar(0,0,'d',false);
  //lc.setDigit(0, 3, 1, false);
  //lc.setDigit(0, 4, 2, false);
  //lc.setChar(0, 5, 'C', false);
}

void init_wifi()
{
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
  Serial.write("AT+CWJAP=\"fudgeNet2G\",\"ancienthill347\"\r\n");
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
  tmpstr = "0" + tmpstr;
  hmstr = "0" + hmstr;


  Serial.write("AT+CIPSTART=\"TCP\",\"attentii.co.uk\",80\r\n");
  delay(3000);
  String para = "GET /leeweb/tempmon/tmpsub.php?tmhd=0020000" + tmpstr + hmstr + " HTTP/1.1\r\nHost: attentii.co.uk\r\nConnection: close";
  Serial.println("AT+CIPSEND=" + String(para.length() + 4));
  delay(1000);

  Serial.println(para);
  delay(1000);
  Serial.println("");

  //Serial.write(">GET /leeweb/tempmon/tmpsub.php?tmhd=123456 HTTP/1.1 \r\nHost: attentii.co.uk\r\n");

}

void buzzer(bool state)
{
  if (state) {
    analogWrite(A0, 128);
  } else {
    analogWrite(A0, 0);
  }
}
