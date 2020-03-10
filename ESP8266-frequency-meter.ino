/*
   Добавил файловый менеджер
   Добавил обновление через вебстраницу "Системные настройки"
   Автозапуск АР Default, если не найден файл настроек
   Подключение внешних сигналов:
    - GPIO14 (D5) - вход импульсов от датчика стенда проверки ЦВ;
    - GPIO5 (D1) - вход импульсов от датчика стенда ТАХОГЕНЕРАТОРА;
    - GPIO16 (D0) - вход (без прерывания) от нормально-закрытого контакта ЦВ.
   Подключение внутренних элементов (для платы типа ESP8266 Witty):
    - GPIO2 (D4) - голубой wifi светодиод;
    - GPIO4 (D2) - кнопка, подключена к пину (для платы типа ESP8266 Witty);
    - GPIO12 (D6) - зеленый цвет RGB-светодиода (для платы типа ESP8266 Witty);
    - GPIO13 (D7) - синий цвет RGB-светодиода (для платы типа ESP8266 Witty);
    - GPIO15 (D8) - красный цвет RGB-светодиода (для платы типа ESP8266 Witty).
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <WebSocketsServer.h>
#include <FS.h>
#include <ArduinoJson.h>

#define LED_WIFI 2     // номер пина светодиода GPIO2 (D4)
#define LED_RED 15     // пин, красного светодиода 
#define LED_GREEN 12   // пин, зеленого светодиода 
#define LED_BLUE 13    // пин, синего светодиода
#define BUTTON 4       // номер пина кнопки GPIO4 (D2)
#define IMPULS_IN_CV 14   // пин, вход импульсов от датчика стенда проверки ЦВ
#define IMPULS_IN_TG 5    // пин, вход импульсов от датчика стенда проверки ТАХОГЕНЕРАТОРА
#define CONTACT_IN 16     // пин, вход (без прерывания) от нормально-закрытого контакта ЦВ

bool wifiAP_mode = 0;
char *p_ssidAP = "AP";             //SSID-имя вашей сети
char *p_passwordAP = "12345678";
char *p_ssid = "lamp";
char *p_password = "1234567890lamp";
byte ip[4] = {192, 168, 1, 43};
byte sbnt[4] = {255, 255, 255, 0};
byte gtw[4] = {192, 168, 1, 1};

bool sendSpeedDataEnable[] = {0, 0, 0, 0, 0};
String ping = "ping";
unsigned int speedT = 200;  //период отправки данных, миллисек

unsigned int timerPeriod = 1000000;      //1мс=5000
unsigned int impulsFreq = 0;
unsigned int impulsFreqPrev = 0;
volatile unsigned int impulsCount = 0;
unsigned int impulsCountArray[] = {0, 0, 0};
int indArray = 0;
volatile bool impulsCountedEnd = 1;
int impulsIzmerenieEnable = 0;
bool impulsTimerEnable = 0;
int impulsGpioIn = IMPULS_IN_CV;

WebSocketsServer webSocket(81);
ESP8266WebServer server(80);
//int timeT1 = millis();

void ICACHE_RAM_ATTR bitTxIsr() {
  impulsCountedEnd = 1;
}

void  ICACHE_RAM_ATTR interruptFunction() {
  impulsCount ++;
}

void setup() {
  Serial.begin(115200);
  Serial.println("");
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(IMPULS_IN_CV, INPUT_PULLUP);
  pinMode(IMPULS_IN_TG, INPUT_PULLUP);
  pinMode(CONTACT_IN, INPUT_PULLUP);
  digitalWrite(LED_WIFI, HIGH);
  digitalWrite(LED_GREEN, LOW);
  //printChipInfo();

  SPIFFS.begin();
  //scanAllFile();
  //saveConfiguration();
  //printFile("/config.txt");

  //Запуск точки доступа с параметрами поумолчанию
  if ( !loadConfiguration() ||  digitalRead(BUTTON) == 0)  startAp("ESP", "11111111");
  //Запуск точки доступа
  else if (digitalRead(BUTTON) == 1 && wifiAP_mode == 1)   startAp(p_ssidAP, p_passwordAP);
  //Запуск подключения клиента к точке доступа
  else if (digitalRead(BUTTON) == 1 && wifiAP_mode == 0) {
    if (WiFi.getPersistent() == true)    WiFi.persistent(false);
    WiFi.softAPdisconnect(true);
    WiFi.persistent(true);
    if (WiFi.SSID() != p_ssid || WiFi.psk() != p_password) {
      Serial.println(F("\nCHANGE password or ssid"));
      WiFi.disconnect();
      set_staticIP();
      WiFi.begin(p_ssid, p_password);
    } else {
      set_staticIP();
    }
  }

  webServer_init();      //инициализация HTTP интерфейса
  webSocket_init();      //инициализация webSocket интерфейса
  timer1_isr_init();     //инициализация таймера
  timer1_attachInterrupt(bitTxIsr);
}


void loop() {
  wifi_init();
  webSocket.loop();
  server.handleClient();

  // Активация/деактивация таймера с прерыванием если измерение разрешено/запрещено
  if ( impulsIzmerenieEnable != 0 && impulsTimerEnable == 0) {
    if (impulsIzmerenieEnable == 1)  impulsGpioIn = IMPULS_IN_CV;
    if (impulsIzmerenieEnable == 2)  impulsGpioIn = IMPULS_IN_TG;
    //Вкл. прерывания по таймеру
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
    timer1_write(1000000);    //5000 едениц = 1мс; 1000000 = 200 мс
    //Вкл. прерывания по обоим фронтам на входе IMPULS_IN_CV или IMPULS_IN_TG
    attachInterrupt (digitalPinToInterrupt (impulsGpioIn), interruptFunction, CHANGE  );
    impulsCountedEnd = 0;
    impulsTimerEnable = 1;    //таймер запущен
    impulsCount = 0;          //счетчик импульсов сбросить в 0
  }

  //Вычисление частоты импульсов
  if ( impulsCountedEnd == 1 && impulsTimerEnable == 1) {
    detachInterrupt(digitalPinToInterrupt (impulsGpioIn));

    //Определяем количество импульсов за 1 сек, т.е ГЦ
    //Serial.print("iCnt=");
    //Serial.println(impulsCount);
    impulsCountArray[indArray] = impulsCount * 10;
    impulsFreq = (impulsCountArray[0] + impulsCountArray[1] + impulsCountArray[2]) / 3;
    indArray ++;
    if (indArray == 3)  indArray = 0;

    //Serial.println(impulsFreq);
    //Отправка Speed данных клиентам каждые speedT миллисекунд, при условии что данныее обновились и клиенты подключены
    if (true) {
      if (sendSpeedDataEnable[0] || sendSpeedDataEnable[1] || sendSpeedDataEnable[2] || sendSpeedDataEnable[3] || sendSpeedDataEnable[4] ) {
        if (impulsFreqPrev != impulsFreq ) {
          //if ( millis() - timeT1 > speedT ) {
          String data = "{\"freq\":";
          data += impulsFreq;
          data += "}";
          int startT_broadcastTXT = micros();
          webSocket.broadcastTXT(data);
          int T_broadcastTXT = micros() - startT_broadcastTXT;
          if (T_broadcastTXT > 100000)  checkPing();
          //timeT1 = millis();
          //}
        }
      }
    }
    impulsFreqPrev = impulsFreq;
    impulsTimerEnable = 0;
  }

}


//Print configuration parameters
void printConfiguration () {
  Serial.println(F("Print ALL VARIABLE:"));
  Serial.print(F("wifiAP_mode="));  Serial.println(wifiAP_mode);
  Serial.print(F("p_ssidAP="));     Serial.println(p_ssidAP);
  Serial.print(F("p_passwordAP=")); Serial.println(p_passwordAP);
  Serial.print(F("p_ssid="));       Serial.println(p_ssid);
  Serial.print(F("p_password="));   Serial.println(p_password);
  Serial.print(F("ip="));    Serial.print(ip[0]);   Serial.print(":");  Serial.print(ip[1]);  Serial.print(":");  Serial.print(ip[2]);  Serial.print(":");  Serial.println(ip[3]);
  Serial.print(F("sbnt="));  Serial.print(sbnt[0]); Serial.print(":");  Serial.print(sbnt[1]);  Serial.print(":");  Serial.print(sbnt[2]);  Serial.print(":");  Serial.println(sbnt[3]);
  Serial.print(F("gtw="));   Serial.print(gtw[0]);  Serial.print(":");  Serial.print(gtw[1]);  Serial.print(":");  Serial.print(gtw[2]);  Serial.print(":");  Serial.println(gtw[3]);
}

void printChipInfo() {
  Serial.print(F("\n<-> LAST RESET REASON: "));  Serial.println(ESP.getResetReason());
  Serial.print(F("<-> ESP8266 CHIP ID: "));      Serial.println(ESP.getChipId());
  Serial.print(F("<-> CORE VERSION: "));         Serial.println(ESP.getCoreVersion());
  Serial.print(F("<-> SDK VERSION: "));          Serial.println(ESP.getSdkVersion());
  Serial.print(F("<-> CPU FREQ MHz: "));         Serial.println(ESP.getCpuFreqMHz());
  Serial.print(F("<-> FLASH CHIP ID: "));        Serial.println(ESP.getFlashChipId());
  Serial.print(F("<-> FLASH CHIP SIZE: "));      Serial.println(ESP.getFlashChipSize());
  Serial.print(F("<-> FLASH CHIP REAL SIZE: ")); Serial.println(ESP.getFlashChipRealSize());
  Serial.print(F("<-> FLASH CHIP SPEED: "));     Serial.println(ESP.getFlashChipSpeed());
  Serial.print(F("<-> FREE MEMORY: "));          Serial.println(ESP.getFreeHeap());
  Serial.print(F("<-> SKETCH SIZE: "));          Serial.println(ESP.getSketchSize());
  Serial.print(F("<-> FREE SKETCH SIZE: "));     Serial.println(ESP.getFreeSketchSpace());
  Serial.print(F("<-> CYCLE COUNTD: "));         Serial.println(ESP.getCycleCount());
}
