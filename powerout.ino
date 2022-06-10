String str1,str2;

bool onUSB = false;
bool onBattery = false;
bool lowBattery = false;
bool timeToSendHb = false;
unsigned long pwrCheckTimeStart;//to check power every 10sec
unsigned long lastPublishTime;
FuelGauge fuel;
float batteryVoltage = 0.0;


void setup() {
    Serial.begin(115200);//debug
    Serial1.begin(57600);//from trigBoard

    // INITIAL POWER CHECK
    int powerSource = System.powerSource();
    if (powerSource == POWER_SOURCE_BATTERY) {// ON BATTERY
        onBattery = true;
        onUSB = false;
    }
    else{// ON USB
        onBattery = false;
        onUSB = true;
    }
    
    batteryVoltage = fuel.getVCell();
    
    if(onBattery){//bootup message alert so we know things are back online
         str1 = "HOME Booting...";
         str2 = "NO AC POWER ";
         str2.concat(batteryVoltage);
         sendData();
    }else{
         str1 = "HOME Booting...";
         str2 = "AC POWER GOOD ";
         str2.concat(batteryVoltage);
         sendData();
    }

    pwrCheckTimeStart = millis();
}

void loop() {
    // FROM ESP32 SERVER
  if (Serial1.available() > 0) {// new data came in
     Serial.println("New Data");
     str1 = Serial1.readStringUntil(',');//that's the separator
     str2 = Serial1.readStringUntil('#');
     sendData();
     Serial1.flush();
  }
  //********************

  // POWER CHECK
  if(millis()-pwrCheckTimeStart>10000){
    batteryVoltage = fuel.getVCell();
    pwrCheckTimeStart = millis();
    int powerSource = System.powerSource();
    if (powerSource == POWER_SOURCE_BATTERY) {// ON BATTERY
        if(!onBattery && onUSB){// CHANGED FROM USB TO BATTERY
         onBattery = true;
         onUSB = false;
         str1 = "HOME";
         str2 = "AC POWER LOST ";
         str2.concat(batteryVoltage);
         sendData();
        }
        else if(millis() - lastPublishTime > 600000) { // Reminder that power is down every 10 minutes
            str1 = "HOME";
            str2 = "AC POWER STILL DOWN (10 MIN INTERVAL) ";
            str2.concat(batteryVoltage);
            sendData();
        }
    }else if(onBattery && !onUSB){// CHANGED FROM BATTERY TO USB
        onBattery = false;
        onUSB = true;
        str1 = "HOME";
        str2 = "AC POWER IS ON ";
        str2.concat(batteryVoltage);
        sendData();
    }

    //and also check battery voltage
    if(batteryVoltage < 3.5){// if less than this, send an alert
        if(!lowBattery){
            lowBattery=true;
            str1 = "HOME";
            str2 = "LOW BATTERY ";
            str2.concat(batteryVoltage);
            sendData();
        }
    }else if(batteryVoltage>3.7){//little hysteresis to prevent multiple messages
        lowBattery=false;
    }
    if(timeToSendHb && (millis() - lastPublishTime > (65 * 1000))) { // This might delay our heartbeat but that means other publish operations are happening so no worries here.
        str1 = "HOME";
        str2 = "Periodic Heartbeat ";
        str2.concat(batteryVoltage);
        timeToSendHb = false;
        sendData();   
    }
  }
  //********************
    
// Send heartbeat every 10 hours or so
    if(millis()%(10*60*60*1000) < (60 * 1000)) { // when count is less than 60 seconds we know the time has exceeded our modulus operation
        timeToSendHb = true;
    }

}

void sendData(){
     unsigned long startConnectTime = millis();
     char pushMessage[50], pushName[50];
     str1.toCharArray(pushName, str1.length() + 1);
     str2.toCharArray(pushMessage, str2.length() + 1);

     Serial.println(str1);
     Serial.println(str2);


     String pushoverPacket = "[{\"key\":\"title\", \"value\":\"";
     pushoverPacket.concat(str1);
     pushoverPacket.concat("\"},");
     pushoverPacket.concat("{\"key\":\"message\", \"value\":\"");
     pushoverPacket.concat(str2);
     pushoverPacket.concat("\"}]");
     Particle.publish("PowerApp", pushoverPacket, PRIVATE);//then send to push safer so we get the notifications on our mobile devices
     lastPublishTime = millis();

     Serial.print(millis() - startConnectTime);
     Serial.println("ms to connect");
}
