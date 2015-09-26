#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
#define WLAN_SSID       "ASUSN"   // cannot be longer than 32 characters!
#define WLAN_PASS       "rosh12345678"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2
#define ARRLEN(x)  (sizeof(x) / sizeof((x)[0]))
#define IDLE_TIMEOUT_MS  3000

// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS,
  ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
  SPI_CLOCK_DIVIDER); // you can change this clock speed
const int TONE_DURATION = 200;
const int ERROR_LIMIT = 20;
const int sensors[6] = {0,1,2,3,4,5};
const int sensorCount = ARRLEN(sensors);
int sensordata[ARRLEN(sensors)];
const char HOSTNAME[] = "ec2-54-200-253-6.us-west-2.compute.amazonaws.com";
const char SENSORNAME[] = "ard1";
char   buffer[100];
uint32_t ip = 0L, t;
const int LED = 13;
int errorCounter = 0;

const unsigned long
  dhcpTimeout     = 60L * 1000L, // Max time to wait for address from DHCP
  connectTimeout  = 15L * 1000L, // Max time to wait for server connection
  responseTimeout = 15L * 1000L; // Max time to wait for data from server
void(* resetFunc) (void) = 0;

void setup(void) {
  Serial.begin(115200);
  pinMode(1, INPUT);
  pinMode(13, OUTPUT);
  digitalWrite(LED, HIGH);
  delay(300);
  digitalWrite(LED, LOW);
}

void loop(void) {
 if(connectToWifi()&&addData()){
    digitalWrite(LED, LOW);
    delay(60000);
 }else{
    digitalWrite(LED, HIGH);
 }
  delay(1000);
} // Not used by this code

bool connectToWifi(){
  if(cc3000.checkDHCP()) {
    return true;
  }
  if(!cc3000.begin()) {
    Serial.println(63);
    return false;
  }

  uint32_t z =1;

  cc3000.startSSIDscan(&z);
  if(!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(71);
    return false;
  }
  for(t=millis(); !cc3000.checkDHCP() && ((millis() - t) < dhcpTimeout); delay(1000));
  if(cc3000.checkDHCP()) {
    return true;
  } else {
    Serial.println(78);
    return false;
  }
  return true;
}
bool addData() {
  t = millis();
  while((0L == ip) && ((millis() - t) < connectTimeout)) {
    if(cc3000.getHostByName(HOSTNAME, &ip)) break;
    delay(1000);
  }
  if(0L == ip) {
    Serial.println(90);
    return false;
  }
  Adafruit_CC3000_Client client;
  client = cc3000.connectTCP(ip, 80);
  readSensorData();
  memset(&buffer[0], 0, sizeof(buffer));
  sprintf(buffer,"name=%s&", SENSORNAME);
  for(int i=0;i<sensorCount;i++){
    sprintf(buffer,"%sdata=%d", buffer, sensordata[i]);
    if(i<(sensorCount-1)){
      sprintf(buffer,"%s&",buffer);
    }
  }
  if(client.connected()) {
    client.print(F("POST / HTTP/1.1\r\nHost: "));
    client.print(HOSTNAME);
    client.print(F("\r\n"));
    //client.print(F("Keep-Alive: timeout=15\r\n"));
    client.print(F("Remote Address: 54.200.253.6:80\r\n"));
    client.print(F("Content-Type: application/x-www-form-urlencoded\r\n"));
    client.print(F("Content-Length: "));
    client.print(strlen(buffer));
    client.print("\r\n\r\n");
    client.print(buffer);
    Serial.println(buffer);
  } else {
    Serial.println(117);
    errorCounter++;
    if(errorCounter==ERROR_LIMIT){
      resetFunc();
    }
    return false;
  }
  
  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
  unsigned long lastRead = millis();
  while (client.connected()&& (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (client.available()) {
      client.read();
      lastRead = millis();
    }
  }
  client.close();
  return true;
}

void readSensorData(){
  for(int i=0;i<sensorCount;i++){
    sensordata[i] = analogRead(sensors[i]);
  }
}

size_t strlen(const char *str)
{
    const char *s;
    for (s = str; *s; ++s);
    return(s - str);
}
