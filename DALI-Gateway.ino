#include "WS_Dali.h"
#include "thingProperties.h"

Dali dali;
hw_timer_t *timer = NULL;
bool dataReceived=false;
bool updateCloudValues=false;
uint8_t lastData;
unsigned long previousMillis = 0;
bool lastflur_S, lastkueche_PL, lastkueche_LED_Oben, lastkueche_LED_Unten, lastesstisch_PL;

uint8_t bus_is_high() {
  return digitalRead(RX_PIN); 
}

void bus_set_low() {
  digitalWrite(TX_PIN,LOW); 
}

void bus_set_high() {
  digitalWrite(TX_PIN,HIGH); 
}

void ARDUINO_ISR_ATTR onTimer() {
  dali.timer();
}

void DALI_Init() {
  printf("Initializing DALI ...\r\n");

  pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);
  
  timer = timerBegin(9600000);
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 1000, true, 0);

  dali.begin(bus_is_high, bus_set_high, bus_set_low);
  printf("DALI initialization completed.\r\n");
}

void setup() {

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  DALI_Init();

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  UpdateCloudDimmedLight(&lastflur_S, &flur_S, 4); //64+4
  UpdateCloudDimmedLight(&lastkueche_PL, &kueche_PL, 11); //64+0
  UpdateCloudDimmedLight(&lastkueche_LED_Oben, &kueche_LED_Oben, 7); //64+7
  UpdateCloudDimmedLight(&lastkueche_LED_Unten, &kueche_LED_Unten, 6); //64+6
  UpdateCloudDimmedLight(&lastesstisch_PL, &esstisch_PL, 38);  //64+1
}

void loop() {
  ArduinoCloud.update();

  uint8_t data[4];
  uint8_t bitcnt = dali.rx(data);
  if(bitcnt>=8) {
    updateCloudValues=false;
    for(uint8_t i=0;i<=(bitcnt-1)>>3;i++) {
      printf("%X",data[i]);
      lastData=data[i];
      printf(" ");
    }
    printf("\r\n");
    dataReceived=true;
  }
  if ((bitcnt==0) && (dataReceived) && ((lastData==0) || (lastData==5) || (lastData==2) || (lastData==1))) {
    printf("Commands received. Waiting 5 seconds to update cloud values.\r\n");
    updateCloudValues=true;
    previousMillis=millis();
    dataReceived=false;
  }
  if ((updateCloudValues) && (millis()-previousMillis >= 5000)) {
    updateCloudValues=false;
    UpdateCloudDimmedLight(&lastflur_S, &flur_S, 4); //64+4
    UpdateCloudDimmedLight(&lastkueche_PL, &kueche_PL, 11); //64+0
    UpdateCloudDimmedLight(&lastkueche_LED_Oben, &kueche_LED_Oben, 7); //64+7
    UpdateCloudDimmedLight(&lastkueche_LED_Unten, &kueche_LED_Unten, 6); //64+6
    UpdateCloudDimmedLight(&lastesstisch_PL, &esstisch_PL, 38);  //64+1
  }
}

void UpdateCloudDimmedLight(bool *lastStatus, CloudDimmedLight *light, int DALIaddress) {
  int16_t level;
  for (int i=0;i<=5;i++) {
    printf("Updating cloud value for address %d ...\r\n",DALIaddress);
    level = dali.cmd(DALI_QUERY_ACTUAL_LEVEL, DALIaddress);
    if (level>=0) {
      printf("Level: %d\r\n",level);
      level = level / 2.54;
      printf("Level: %d\r\n",level);
      (*light).setSwitch((level>0)?true:false);
      (*light).setBrightness(level);
      (*lastStatus)=(level>0)?true:false;
      break;
    } else {
      printf("Error %d .. waiting 1 second\r\n", level);
      delay(1000);
    }
  }
}

void UpdateDALILight(bool *lastStatus, CloudDimmedLight *light, int DALIaddress) {
  uint8_t Light_practical = (uint8_t)(2.55*(*light).getBrightness());
  if ((*light).getSwitch() == false) {
    Light_practical=0;
    *lastStatus=false;
  } else {
    printf("Switch is true\r\n");
    if (!(*lastStatus)) {
      printf("lastStatus is false\r\n");
      Light_practical=254;
      (*light).setBrightness(100);
      (*lastStatus)=true;
    }
  }
  dali.set_level(Light_practical,DALIaddress);  
}

void onFlurSChange() {
  printf("onFlurSChange\r\n");
  UpdateDALILight(&lastflur_S, &flur_S, 64+4);
}

/*
  Since KuechePL is READ_WRITE variable, onKuechePLChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onKuechePLChange()  {
  printf("onKuechePLChange\r\n");
  UpdateDALILight(&lastkueche_PL, &kueche_PL, 64+0);
}

/*
  Since KuecheLEDOben is READ_WRITE variable, onKuecheLEDObenChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onKuecheLEDObenChange()  {
  printf("onKuecheLEDObenChange\r\n");
  UpdateDALILight(&lastkueche_LED_Oben, &kueche_LED_Oben, 64+7);
}

/*
  Since KuecheLEDUnten is READ_WRITE variable, onKuecheLEDUntenChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onKuecheLEDUntenChange()  {
  printf("onKuecheLEDUntenChange\r\n");
  UpdateDALILight(&lastkueche_LED_Unten, &kueche_LED_Unten, 64+6);
}

/*
  Since EsstischPL is READ_WRITE variable, onEsstischPLChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onEsstischPLChange()  {
  printf("onEsstischPLChange\r\n");
  UpdateDALILight(&lastesstisch_PL, &esstisch_PL, 64+1);
}
