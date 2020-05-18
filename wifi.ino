///////////////////////////////////////////////////////////
//////  WI-FI  WI-FI  WI-FI  //////////////////////////////
///////////////////////////////////////////////////////////

unsigned int wifiConnectTimer = millis();     //перемнная для таймера включения WIFI
unsigned int ledBlinkTimer = millis();        //перемнная для мигания LED WIFI
bool wlConnectedMsgSend = 0;
bool wifiAP_runned = 0;

void wifi_init()
{
  //Разовое сообщение при подключении к точке доступа WIFI
  if (WiFi.status() == WL_CONNECTED && wlConnectedMsgSend == 0) {
    Serial.println(F("\nCONNECTED to WiFi AP!"));
    Serial.print(F("My IP address: "));   Serial.println(WiFi.localIP());
    Serial.print(F("Subnet mask: "));     Serial.println(WiFi.subnetMask());
    Serial.print(F("Subnet gateway: "));  Serial.println(WiFi.gatewayIP());
    Serial.print(F("My macAddress: "));   Serial.println(WiFi.macAddress());
    Serial.print(F("My default hostname: "));   Serial.println(WiFi.hostname());
    Serial.print(F("Connected to AP with SSID: "));       Serial.println(WiFi.SSID());
    Serial.print(F("Connected to AP with password: "));   Serial.println(WiFi.psk());
    startMDNS();
    wlConnectedMsgSend = 1;
    digitalWrite(LED_WIFI, 0);
  }

  if (wifiAP_mode == 0 && wifiAP_runned == 0) {
    //Мигание LED WIFI при поиске точки доступа
    if (WiFi.status() != WL_CONNECTED && millis() - ledBlinkTimer > 250) {
      digitalWrite(LED_WIFI, !digitalRead(LED_WIFI));
      ledBlinkTimer = millis();
      wlConnectedMsgSend = 0;
    }
  } else if (wifiAP_runned == 1) {
    //Мигание LED WIFI при работе точки доступа
    if (wifiAP_runned == 1) {
      if (millis() - ledBlinkTimer > 750) {
        digitalWrite(LED_WIFI, !digitalRead(LED_WIFI));
        ledBlinkTimer = millis();
      }
    }
  }
}


//Функция запуска модуля в режиме AP
void startAp(char *ap_ssid, const char *ap_password)
{
  Serial.println(F("\n\nSTART ESP in AP WIFI mode"));
  if (WiFi.getPersistent() == true)    WiFi.persistent(false);   //disable saving wifi config into SDK flash area
  WiFi.disconnect();
  WiFi.softAP(ap_ssid, ap_password);
  WiFi.persistent(true);                                      //enable saving wifi config into SDK flash area
  wifiAP_runned = 1;
  startMDNS();
  Serial.print(F("SSID AP: "));      Serial.println(ap_ssid);
  Serial.print(F("Password AP: "));  Serial.println(ap_password);
  Serial.print(F("Start AP with SSID: "));       Serial.println(WiFi.softAPSSID());
  Serial.print(F("Soft-AP IP: "));   Serial.println(WiFi.softAPIP());
  Serial.print(F("Soft-AP MAC: "));  Serial.println(WiFi.softAPmacAddress());

}


//Настройка статических параметров сети. Не записывабтся во flash память.
void set_staticIP() 
{
  IPAddress ipAdr(ip[0], ip[1], ip[2], ip[3]);
  IPAddress gateway(gtw[0], gtw[1], gtw[2], gtw[3]);
  IPAddress subnet(sbnt[0], sbnt[1], sbnt[2], sbnt[3]);
  WiFi.config(ipAdr, gateway, subnet);
}


void startMDNS() {
  String mdnsNameStr = DEVICE_TYPE + String(ESP.getChipId(), HEX);
  //Serial.print(F("Host name for mDNS: "));        Serial.println(mdnsNameStr);
  char mdnsName[mdnsNameStr.length()];
  mdnsNameStr.toCharArray(mdnsName, mdnsNameStr.length() + 1);
  Serial.print(F("Host name for mDNS: "));        Serial.println(mdnsName);
  if (!MDNS.begin(mdnsName)) {
    Serial.println(F("Error setting up MDNS responder!"));
  } else {
    Serial.println(F("mDNS responder started"));
    MDNS.addService("http", "tcp", 80);
    //MDNS.addService("ws", "tcp", 81);
  }
}


//Monitoring Status WiFi module to serial
void WifiStatus(void)
{
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F(": WiFi.status = WL_CONNECTED. ")); //3
    Serial.print(F("IP address: "));  Serial.println(WiFi.localIP());
  }
  else if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F(": WiFi.status = WL_NO_SHIELD ")); //255
  }
  else if (WiFi.status() == WL_IDLE_STATUS) {
    Serial.println(F(": WiFi.status = WL_IDLE_STATUS ")); //0
  }
  else if (WiFi.status() == WL_NO_SSID_AVAIL) {
    Serial.println(F(": WiFi.status = WL_NO_SSID_AVAIL ")); //1
  }
  else if (WiFi.status() == WL_SCAN_COMPLETED) {
    Serial.println(F(": WiFi.status = WL_SCAN_COMPLETED ")); //2
  }
  else if (WiFi.status() == WL_CONNECT_FAILED) {
    Serial.println(F(": WiFi.status = WL_CONNECT_FAILED ")); //4
  }
  else if (WiFi.status() == WL_CONNECTION_LOST) {
    Serial.println(F(": WiFi.status = WL_CONNECTION_LOST ")); //5
  }
  else if (WiFi.status() == WL_DISCONNECTED) {
    Serial.println(F(": WiFi.status = WL_DISCONNECTED ")); //6
  }
}
