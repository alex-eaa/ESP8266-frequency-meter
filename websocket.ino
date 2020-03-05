bool flag;

// функция иннициализации сервера WebSocket
void webSocket_init()
{
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}


//функция обработки входящих на сервер WebSocket сообщений
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  switch (type) {

    case WStype_DISCONNECTED:
      sendSpeedDataEnable[num] = 0;
      Serial.printf("[%u] Disconnected!\n", num);
      //Serial.printf("WStype_DISCONNECTED sendSpeedDataEnable [%u][%u][%u][%u][%u]\n", sendSpeedDataEnable[0], sendSpeedDataEnable[1], sendSpeedDataEnable[2], sendSpeedDataEnable[3], sendSpeedDataEnable[4]);
      break;

    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        if (strcmp((char *)payload, "/index.htm") == 0) {
          sendSpeedDataEnable[num] = 1;
          String data = serializationToJson_index();
          Serial.println(data);
          webSocket.sendTXT(num, data);
        } else if (strcmp((char *)payload, "/setup.htm") == 0) {
          sendSpeedDataEnable[num] = 0;
          String data = serializationToJson_setup();
          Serial.println(data);
          webSocket.sendTXT(num, data);
        }
        break;
      }

    case WStype_TEXT:
      {
        Serial.printf("[%u] get Text: %s\n", num, payload);

        if (strcmp((char *)payload, "RESET") == 0)  ESP.reset();
        else if (strcmp((char *)payload, "on") == 0)   impulsIzmerenieEnable = 1;
        else if (strcmp((char *)payload, "off") == 0)  impulsIzmerenieEnable = 0;
        else {
          DynamicJsonDocument doc(1024);
          DeserializationError error = deserializeJson(doc, payload);
          if (error) {
            Serial.println(F("Failed to deserialization data from client"));
          }

          if (doc["page"].as<String>() == "index") {
            impulsIzmerenieEnable = doc["impulsIzmerenieEnable"];      //Serial.println(impulsIzmerenieEnable);
            //saveConfiguration();
          }
          else if (doc["page"].as<String>() == "setup") {
            String stemp = doc["p_ssid"].as<String>();
            p_ssid = new char [stemp.length() + 1];
            stemp.toCharArray(p_ssid, stemp.length() + 1);
            //Serial.print(F("p_ssid="));   Serial.println(p_ssid);
            stemp = doc["p_password"].as<String>();
            p_password = new char [stemp.length() + 1];
            stemp.toCharArray(p_password, stemp.length() + 1);
            //Serial.print(F("p_password="));   Serial.println(p_password);
            stemp = doc["p_ssidAP"].as<String>();
            p_ssidAP = new char [stemp.length() + 1];
            stemp.toCharArray(p_ssidAP, stemp.length() + 1);
            //Serial.print(F("p_ssidAP="));   Serial.println(p_ssidAP);
            stemp = doc["p_passwordAP"].as<String>();
            p_passwordAP = new char [stemp.length() + 1];
            stemp.toCharArray(p_passwordAP, stemp.length() + 1);
            ip[0] = doc["ip"][0];    //Serial.println(ip[0]);
            ip[1] = doc["ip"][1];    //Serial.println(ip[1]);
            ip[2] = doc["ip"][2];    //Serial.println(ip[2]);
            ip[3] = doc["ip"][3];    //Serial.println(ip[3]);
            sbnt[0] = doc["sbnt"][0];    //Serial.println(sbnt[0]);
            sbnt[1] = doc["sbnt"][1];    //Serial.println(sbnt[1]);
            sbnt[2] = doc["sbnt"][2];    //Serial.println(sbnt[2]);
            sbnt[3] = doc["sbnt"][3];    //Serial.println(sbnt[3]);
            gtw[0] = doc["gtw"][0];    //Serial.println(gtw[0]);
            gtw[1] = doc["gtw"][1];    //Serial.println(gtw[1]);
            gtw[2] = doc["gtw"][2];    //Serial.println(gtw[2]);
            gtw[3] = doc["gtw"][3];    //Serial.println(gtw[3]);
            wifiAP_mode = doc["wifiAP_mode"];  //Serial.println(wifiAP_mode);
            saveConfiguration();
          }
        }
        break;
      }

    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\n", num, length);
      hexdump(payload, length);
      // send message to client
      // webSocket.sendBIN(num, payload, length);
      break;
  }

}


String serializationToJson_index()
{
  DynamicJsonDocument doc(1024);
  doc["impulsIzmerenieEnable"] = impulsIzmerenieEnable;
  doc["impulsFreq"] = impulsFreq;
  String output = "";
  serializeJson(doc, output);
  return output;
}


String serializationToJson_setup()
{
  DynamicJsonDocument doc(1024);
  doc["p_ssid"] = p_ssid;
  doc["p_password"] = p_password;
  doc["p_ssidAP"] = p_ssidAP;
  doc["p_passwordAP"] = p_passwordAP;
  JsonArray ipJsonArray = doc.createNestedArray("ip");
  for (int n = 0; n < 4; n++)  ipJsonArray.add(ip[n]);
  JsonArray sbntJsonArray = doc.createNestedArray("sbnt");
  for (int n = 0; n < 4; n++)  sbntJsonArray.add(sbnt[n]);
  JsonArray gtwJsonArray = doc.createNestedArray("gtw");
  for (int n = 0; n < 4; n++)  gtwJsonArray.add(gtw[n]);
  doc["wifiAP_mode"] = wifiAP_mode;

  String output = "";
  serializeJson(doc, output);
  return output;
}


// Проверка состояния соединения с websocket-клиентами. Отключение тех с которыми нет связи.
void checkPing() {
  //Serial.println("checkPing checkPing checkPing checkPing checkPing");
  for (uint8_t nums = 0; nums < 5; nums++) {
    //int timeStart = micros();
    if ( !webSocket.sendPing(nums, ping) )  webSocket.disconnect(nums);
    //int timeTotal = micros() - timeStart;
    //Serial.printf("TIME ping [%u]: %u\n", nums, timeTotal);
  }
}
