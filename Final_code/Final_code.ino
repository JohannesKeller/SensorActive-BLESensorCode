/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "M5Stack.h"
#include "DHT.h"
#define DHTPIN 26 // what pin we're connected to
#define TFT_GREY 0x5AEB
// Uncomment whatever type you're using!
#define DHTTYPE DHT11 // DHT 11
//#define DHTTYPE DHT22 // DHT 22 (AM2302)
//#define DHTTYPE DHT21 // DHT 21 (AM2301)

#include "esp_bt_main.h"
#include "esp_bt_device.h"

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* helpCharacteristic = NULL;
bool deviceConnected = false;
bool deviceListening = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
char millies[20];
bool all_data_sent = false;



String messungen1[999];
int messungen_counter1 = 0;

String messungen2[999];
int messungen_counter2 = 0;

String messungen3[999];
int messungen_counter3 = 0;




// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define HELP_CHARACTERISTIC_UUID "8eabbd4a-7220-4c74-af1d-b45e97317595"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};



void setup() {
  Serial.begin(115200);
  Wire.begin();
  //M5.begin();
  //M5.Lcd.setRotation(3);
  setup_dht();

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
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

  helpCharacteristic->setValue("x");

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  //Serial.println("Waiting a client connection to notify...");

  xTaskCreatePinnedToCore(
    update_values,     /* Function to implement the task */
    "update_values",   /* Name of the task */
    4096,      /* Stack size in words */
    NULL,      /* Task input parameter */
    1,         /* Priority of the task */
    NULL,      /* Task handle. */
    0);        /* Core where the task should run */
}

void loop() {
  
  //Serial.println("Waiting a client connection to notify...");
  // notify changed value
  if (deviceConnected){
  std :: string rxValue = helpCharacteristic-> getValue ();
  std :: string ell = "l";
  if (rxValue[0]==ell[0]) {
    deviceListening = true;
  }
}
  if (deviceConnected && deviceListening && !all_data_sent) {
    Serial.println("connected");
    //pServer->startAdvertising();
    //deviceConnected = true;
    //delay(2000);
    if (messungen_counter3 != 0) {
      messungen_counter2 = 0;
      copy_array_to_array(3,1);
      
      String mc_help = String(messungen_counter1);
      char mc_help_char[20];
      mc_help.toCharArray(mc_help_char, 20);
      char test[20];

      String mom_timestamp =  String(millis());
      char mom_timestamp_char[20];
      mom_timestamp.toCharArray(mom_timestamp_char, 20);
      
      pCharacteristic->setValue(mc_help_char);
      pCharacteristic->notify();
      delay(33);
      pCharacteristic->setValue(mom_timestamp_char);
      pCharacteristic->notify();
      for (int i = 0; i < messungen_counter1; i++)
      {
        delay(33);
        messungen1[i].toCharArray(test, 20);
        pCharacteristic->setValue(test);
        pCharacteristic->notify();

        
      }
      
      helpCharacteristic->setValue("s");
      all_data_sent = true;

      //deviceConnected = false;
    }
  }

  if (all_data_sent) {
    delay(2000);
    std :: string rxValue = helpCharacteristic-> getValue ();
    std :: string ell = "g";
    if (rxValue[0]==ell[0]) {
     messungen_counter1 = 0;
    copy_array_to_array(2,3);
  }
  Serial.println ("worked");
    deviceListening = false;
    all_data_sent = false;
  }
  
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}

// Example testing sketch for various DHT humidity/temperature sensors


void copy_array_to_array(int array_1, int array_2){
  if(array_1 == 3){
    messungen_counter1 = messungen_counter3;
  for (int i = 0; i < messungen_counter1; i++)
      {
        messungen1[i]=messungen3[i];
      } 
  }
  if(array_1 == 2){
     messungen_counter3 = messungen_counter2;
  for (int i = 0; i < messungen_counter1; i++)
      {
        messungen3[i]=messungen2[i];
      } 
  }
  
}



void update_values(void * pvParameters) {

  for (;;) {
    delay(5000);
    loop_dht();

  }
}


void setup_dht() {
  dht.begin();
}


float last_hum = 0;
float last_bat = 0;
float last_tem = 0;

void loop_dht() {
  printDeviceAddress();
  // Wait a few seconds between measurements.

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float cur_hum = dht.readHumidity();
  cur_hum = round(cur_hum);
  
  // Read temperature as Celsius
  float cur_tem = dht.readTemperature();
  cur_tem = round(cur_tem);
  
  float cur_bat = float(M5.Power.getBatteryLevel());
  //float cur_bat = cur_bat_uint;
  cur_bat = round(cur_bat);
  

  // Check if any reads failed and exit early (to try again).
  if (isnan(cur_hum) || isnan(cur_tem)) {
    
    return;
  }

  if (cur_hum < last_hum-1 || cur_hum > last_hum+1) {
    last_hum = cur_hum;

    String helper = String(millis(), DEC) + "," + "hum" + "," + String(cur_hum);
    
    messungen3[messungen_counter3] = helper;
    messungen2[messungen_counter2] = helper;
    
    messungen_counter3++;
    messungen_counter2++;

  }
  if (cur_tem != last_tem) {
    last_tem = cur_tem;

    String helper = String(millis(), DEC) + "," + "tem" + "," + String(cur_tem);
    
    messungen3[messungen_counter3] = helper;
    messungen2[messungen_counter2] = helper;
    
    messungen_counter3++;
    messungen_counter2++;

  }

  //if(!M5.Power.canControl()){
    if (cur_bat != last_bat) {
      last_bat = cur_bat;
  
      String helper = String(millis(), DEC) + "," + "bat" + "," + String(cur_bat);
      
      messungen3[messungen_counter3] = helper;
      messungen2[messungen_counter2] = helper;
      
      messungen_counter3++;
      messungen_counter2++;
  
    }
  //}

  Serial.println(String(messungen_counter3) + " neue Messwerte");


}

void printDeviceAddress() {
 
  const uint8_t* point = esp_bt_dev_get_address();
  String address;
  
 
  for (int i = 0; i < 6; i++) {
 
    char str[3];
 
    sprintf(str, "%02X", (int)point[i]);
    address += str;
    //Serial.print(str);
 
    if (i < 5){
      address += ":";
      //Serial.print(":");
    }
 
  }
  Serial.println("b_address:("+address+"):end");
}
