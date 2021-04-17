#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "M5Stack.h"
#include "DHT.h"
#define DHTPIN 26
#define TFT_GREY 0x5AEB
#define DHTTYPE DHT11

// Festelgung der UUIDs für den Service und die beiden Characteristics
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define DATA_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define HELP_CHARACTERISTIC_UUID "8eabbd4a-7220-4c74-af1d-b45e97317595"

// Initalisierung des DHT-Sensors
DHT sensor(DHTPIN, DHTTYPE);

// Initialisierung des Servers und der beiden Characteristics
BLEServer* m5Server = NULL;
BLECharacteristic* dataCharacteristic = NULL;
BLECharacteristic* helpCharacteristic = NULL;

bool deviceConnected = false;   // Ist ein Gerät verbunden?
bool deviceListening = false;   // Hört das verbundene Gerät zu?
uint32_t value = 0;
char millies[20];
bool valuesSent = false;        // Gibt an, ob bereits Werte in dem jeweiligen Synchronisationsverfahren abgesendet wurden

float last_hum = 0;
float last_bat = 0;
float last_tem = 0;

// Initialisierung der drei Arrays (3-Array-Konzept) und deren Counter
String messwertArray1[999];
String messwertArray2[999];
String messwertArray3[999];

int messwertCounter1 = 0;
int messwertCounter2 = 0;
int messwertCounter3 = 0;


// Callback aus "BLEServer.h"
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* m5Server) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* m5Server) {
      deviceConnected = false;
      delay(500);
      m5Server->startAdvertising();
    }
};


// Setup()-Funktion, welche einmalig beim Programmstart ausgeführt wird
void setup() {
  Serial.begin(115200);   //Dient lediglich dazu, um Information über die Serial-Schnittstelle auszugeben
  Serial.println ("Setup startet");
  Wire.begin();
  sensor.begin();

  BLEDevice::init("ESP32");   // Erstellung des BLE-Devices

  // Erstellung des BLE-Servers
  m5Server = BLEDevice::createServer();
  m5Server->setCallbacks(new MyServerCallbacks());

  // Erstellung des BLE-Services
  BLEService *pService = m5Server->createService(SERVICE_UUID);

  // Erstellung der beiden Characteristicen mit allen verfügbaren Properties
  dataCharacteristic = pService->createCharacteristic(
                         DATA_CHARACTERISTIC_UUID,
                         BLECharacteristic::PROPERTY_READ   |
                         BLECharacteristic::PROPERTY_WRITE  |
                         BLECharacteristic::PROPERTY_NOTIFY |
                         BLECharacteristic::PROPERTY_INDICATE
                       );
  helpCharacteristic = pService->createCharacteristic(
                         HELP_CHARACTERISTIC_UUID,
                         BLECharacteristic::PROPERTY_READ   |
                         BLECharacteristic::PROPERTY_WRITE  |
                         BLECharacteristic::PROPERTY_NOTIFY |
                         BLECharacteristic::PROPERTY_INDICATE
                       );


  helpCharacteristic->setValue("x");  // helpCharacteristic wird zu Beginn auf "x" gesetzt, damit sie nicht leer ist

  dataCharacteristic->addDescriptor(new BLE2902());   // Erstellung des BLE-Descriptors

  // Start the service
  pService->start();

  // Advertising wird gestartet
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
  Serial.println("Nun kann ein sich ein Client im 'notify'-Modus verbinden...");

  // Starten des Tasks, welcher auf dem Core 0 dauerhaft betrieben werden soll
  xTaskCreatePinnedToCore(
    update_values,     /* Name der Methode, welche der Task verwenden soll */
    "update_values",   /* Anzeigename des Tasks */
    4096,      /* Größe des Tasks */
    NULL,      /* Input, der dem Task mitgegeben werden soll: hier kein Input */
    1,         /* Wichtigkeit des Tasks */
    NULL,      /* Task handle */
    0);        /* Cpu-Kern, auf welchem der Task laufen soll */
  Serial.println ("Setup erfolgreich beendet");
}


// loop()-Methode, welche dauerhaft ausgeführt wird
void loop() {
  if (deviceConnected) {                                            //Überprüfung, ob gerade ein Gerät verbunden ist
    Serial.println ("Ein Gerät ist verbunden");
    if (messwertCounter2 != 0) {                                    // Liegen neue Messwerte vor?
      Serial.println ("Neue Messwerte liegen vor");
      if (!deviceListening) {                                       // Hört das Gerät nicht zu? Falls "ja" wird erneut die value der helpChracteristic überprüft
        std :: string characteristicValue = helpCharacteristic-> getValue ();
        std :: string characteristicHelper = "l";
        if (characteristicValue[0] == characteristicHelper[0]) {
          deviceListening = true;
          Serial.println ("Das Gerät hört zu");
        }
      }
      if (deviceListening && !valuesSent) {                         //Falls das Gerät zuhört und noch keine Werte gesendet wurden, kann Übertragung starten
        Serial.println ("Die Übertragung startet");
        char sendValue[20];
        char momTimestampChar[20];
        char anzahlMesswerteChar[20];

        messwertCounter1 = 0;
        copy_array_to_array(2);
        String anzahlMesswerte = String(messwertCounter3);
        anzahlMesswerte.toCharArray(anzahlMesswerteChar, 20);

        // Zuerst wird die Anzahl der Messwerte übertragen
        String momTimestamp =  String(millis());
        momTimestamp.toCharArray(momTimestampChar, 20);
        dataCharacteristic->setValue(anzahlMesswerteChar);
        dataCharacteristic->notify();
        delay(33);

        // Nach kurzem Delay wird der momentane Timestamp abgesendet
        dataCharacteristic->setValue(momTimestampChar);
        dataCharacteristic->notify();

        // In der for-Schleife werden alle Messwerte nun abgesendet und anschließend die helpCharctersitic auf "s" gesetzt
        for (int i = 0; i < messwertCounter3; i++)
        {
          delay(33);
          messwertArray3[i].toCharArray(sendValue, 20);
          dataCharacteristic->setValue(sendValue);
          dataCharacteristic->notify();
        }
        helpCharacteristic->setValue("s");
        valuesSent = true;
        Serial.println ("Alle Werte wurden gesendet");
      }
    }

    // Falls alle Werte gesendet wurden und dies über die helpCharacteristic rückbestätigt wurde, ist die Übertragung erfolgreich beendet
    if (valuesSent) {
      delay(2000);
      std :: string characteristicValue = helpCharacteristic-> getValue ();
      std :: string characteristicHelper = "g";
      if (characteristicValue[0] == characteristicHelper[0]) {
        messwertCounter3 = 0;
        copy_array_to_array(1);
      }
      Serial.println ("worked");
      deviceListening = false;
      valuesSent = false;
      Serial.println ("Die Übertragung war ein voller Erfolg");
    }
  }
}



// Diese Methode dient lediglich dazu, um die Arrays zu "clonen"
void copy_array_to_array(int array_1) {
  if (array_1 == 2) {
    messwertCounter3 = messwertCounter2;
    for (int i = 0; i < messwertCounter3; i++)
    {
      messwertArray3[i] = messwertArray2[i];
    }
  }
  if (array_1 == 1) {
    messwertCounter2 = messwertCounter1;
    for (int i = 0; i < messwertCounter3; i++)
    {
      messwertArray2[i] = messwertArray1[i];
    }
  }
}



// die Methode, welche von "xTaskCreatePinnedToCore" dauerhaft betriebn wird
void update_values(void * pvParameters) {

  for (;;) {
    delay(5000);

    // Im Folgenden werden Feuchtigkeit und Temperatur vom Sensor, sowie Batteriestand vom Gerät ausgelesen
    float cur_hum = sensor.readHumidity();
    float cur_tem = sensor.readTemperature();
    float cur_bat = float(M5.Power.getBatteryLevel());

    cur_hum = round(cur_hum);
    cur_tem = round(cur_tem);
    cur_bat = round(cur_bat);

    // Falls die Abfragen fehlerhaft waren, wird vorzeitig abgebrochen
    if (isnan(cur_hum) || isnan(cur_tem)) {
      return;
    }

    // Alle Messwerte, werden mit den zuletzt gemessenen Werten verglichen und nur gespeichert, wenn eine Differenz besteht
    if (cur_hum < last_hum - 1 || cur_hum > last_hum + 1) {
      last_hum = cur_hum;
      String helper = String(millis(), DEC) + "," + "hum" + "," + String(cur_hum);
      messwertArray2[messwertCounter2] = helper;
      messwertArray1[messwertCounter1] = helper;
      messwertCounter2++;
      messwertCounter1++;
    }

    if (cur_tem != last_tem) {
      last_tem = cur_tem;
      String helper = String(millis(), DEC) + "," + "tem" + "," + String(cur_tem);
      messwertArray2[messwertCounter2] = helper;
      messwertArray1[messwertCounter1] = helper;
      messwertCounter2++;
      messwertCounter1++;
    }

    if (cur_bat != last_bat) {
      last_bat = cur_bat;
      String helper = String(millis(), DEC) + "," + "bat" + "," + String(cur_bat);
      messwertArray2[messwertCounter2] = helper;
      messwertArray1[messwertCounter1] = helper;
      messwertCounter2++;
      messwertCounter1++;
    }
  }
}
