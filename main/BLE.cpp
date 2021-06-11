#include "globals.h"
#include "debug.h"
#include "myOTA.h"
#include "System.h"
#include "main.h"

#include "src/NimBLELog.h"
#include "src/NimBLEDevice.h"


#include <stdio.h>
#include "mySPIFFS.h"
//#include <esp_wiFi.h>
#include <esp_err.h>
//#include "Weight.h"
#include <string>
#include "myWiFi.h"

static NimBLEServer *pServer;
extern SystemX *_sys;

TaskHandle_t BLETask_h;

QueueHandle_t bleWeightQueue;
QueueHandle_t bleUnitsQueue;
QueueHandle_t bleBatteryQueue;
QueueHandle_t bleStatusQueue;


void BLETask(void * parameter){
    for(;;) {
        if(pServer->getConnectedCount()) {
            NimBLEService* pSvc = pServer->getServiceByUUID("181D");
            if(pSvc){
              if(uxQueueMessagesWaiting(bleUnitsQueue) > 0 ){  
                Units buff;
                NimBLECharacteristic* u = pSvc->getCharacteristic("2B46");
                xQueueReceive(bleUnitsQueue, &buff, (TickType_t) 10);
                if(u){
                  u->setValue(unitsToString(buff));
                  u->notify(true);
                }  
              }
              if(uxQueueMessagesWaiting(bleWeightQueue) > 0 ){
                  char buff[16];
                  NimBLECharacteristic* w = pSvc->getCharacteristic("2A9E");
                  xQueueReceive(bleWeightQueue, &buff, (TickType_t) 10);
                  if(w){
                    w->setValue(buff);
                    w->notify(true);
                  }
              }
              if(uxQueueMessagesWaiting(bleBatteryQueue) > 0 ){
                  int buff;
                  NimBLECharacteristic* b = pSvc->getCharacteristic("2A19");
                  xQueueReceive(bleWeightQueue, &buff, (TickType_t) 10);
                  if(b){
                    b->setValue(std::to_string(buff));
                  }
              }
              if(uxQueueMessagesWaiting(bleStatusQueue) > 0 ){
                  int buff;
                  NimBLECharacteristic* b = pSvc->getCharacteristic("000057a7-60be-11eb-ae93-0242ac130002");
                  xQueueReceive(bleWeightQueue, &buff, (TickType_t) 10);
                  if(b){
                    b->setValue(buff);
                  }
              }
            }
        }
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
    
    vTaskDelete(BLETask_h);
}

void updateBTWeight(std::string w)
{
  char* ch = (char*)w.c_str();
  if (pServer!= NULL && pServer->getConnectedCount())
  {
    xQueueSend(bleWeightQueue, &ch , (TickType_t)10);
  }
}

void updateBTUnits(Units unit){
  Units ch = unit;
  if (pServer!= NULL && pServer->getConnectedCount())
  {
    xQueueSend(bleUnitsQueue, &ch , (TickType_t)10);
  }
}

void updateBTBattery(int bat){
  int b = bat;
  if (pServer!= NULL && pServer->getConnectedCount())
  {
    xQueueSend(bleBatteryQueue, &b , (TickType_t)10);
  }
}

void updateBTStatus(int status){
  int s = status;
  if( pServer!= NULL && pServer->getConnectedCount()){
    xQueueSend(bleStatusQueue, &status, (TickType_t)10);
  }
}

bool isBtConnected()
{
  if ( pServer!= NULL && pServer->getConnectedCount() > 0 )
  {
    return true;
  }
  else
  {
    return false;
  }
}


class ServerCallbacks : public NimBLEServerCallbacks
{
  // Why does it run both onConnect functions when connecting???
  void onConnect(NimBLEServer *pServer)
  {
    debugPrintln("Client connected");
    debugPrintln("Multi-connect support: start advertising");
    NimBLEDevice::startAdvertising();
  };
  /** Alternative onConnect() method to extract details of the connection.
        See: src/ble_gap.h for the details of the ble_gap_conn_desc struct.
    */
  void onConnect(NimBLEServer *pServer, ble_gap_conn_desc *desc)
  {
    debugPrint("Client address: ");
    debugPrintln(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
    /** We can use the connection handle here to ask for different connection parameters.
          Args: connection handle, min connection interval, max connection interval
          latency, supervision timeout.
          Units; Min/Max Intervals: 1.25 millisecond increments.
          Latency: number of intervals allowed to skip.
          Timeout: 10 millisecond increments, try for 5x interval time for best results.
      */
    pServer->updateConnParams(desc->conn_handle, 24, 48, 0, 120);
  };

  void onDisconnect(NimBLEServer *pServer)
  {
    debugPrintln("Client disconnected - start advertising");
    NimBLEDevice::startAdvertising();
  };

  /********************* Security handled here **********************
    ****** Note: these are the same return values as defaults ********/
  uint32_t onPassKeyRequest()
  {
    debugPrintln("Server Passkey Request");
    /** This should return a random 6 digit number for security
          or make your own static passkey as done here.
      */
    return 123456;
  };

  bool onConfirmPIN(uint32_t pass_key)
  {
    debugPrint("The passkey YES/NO number: ");
    debugPrintln((int)pass_key);
    /** Return false if passkeys don't match. */
    return true;
  };

  void onAuthenticationComplete(ble_gap_conn_desc *desc)
  {
    /** Check that encryption was successful, if not we disconnect the client */
    if (!desc->sec_state.encrypted)
    {
      NimBLEDevice::getServer()->disconnect(desc->conn_handle);
      debugPrintln("Encrypt connection failed - disconnecting client");
      return;
    }
    debugPrintln("Starting BLE work!");
  };
};

/** Handler class for characteristic actions */
class DeviceCallbacks : public NimBLECharacteristicCallbacks
{
  void onRead(NimBLECharacteristic *pCharacteristic)
  {
    debugPrint(pCharacteristic->getUUID().toString().c_str());
    debugPrint(": onRead(), value: ");
    debugPrintln(pCharacteristic->getValue().c_str());
  };

  void onWrite(NimBLECharacteristic *pCharacteristic)
  {
    debugPrint(pCharacteristic->getUUID().toString().c_str());
    debugPrint(": onWrite(), value: ");
    debugPrintln(pCharacteristic->getValue().c_str());
  };
  /** Called before notification or indication is sent,
        the value can be changed here before sending if desired.
    */
  void onNotify(NimBLECharacteristic *pCharacteristic)
  {
    debugPrintln("Sending notification to clients");
  };

  /** The status returned in status is defined in NimBLECharacteristic.h.
        The value returned in code is the NimBLE host return code.
    */
  void onStatus(NimBLECharacteristic *pCharacteristic, Status status, int code)
  {
    debugPrint("Notification/Indication status code: ");
    debugPrint(status);
    debugPrint(", return code: ");
    debugPrint(code);
    debugPrint(", ");
    debugPrint(NimBLEUtils::returnCodeToString(code));
    debugPrintln("");
  };
};

/** Handler class for battery service actions */
class BatteryCallbacks : public NimBLECharacteristicCallbacks
{
  void onRead(NimBLECharacteristic *pCharacteristic)
  {
    debugPrint(pCharacteristic->getUUID().toString());

    pCharacteristic->setValue("80"); //_sys->getBattery());
    debugPrint(": onRead(), value: ");
    debugPrintln(pCharacteristic->getValue());
    debugPrintln("getBattery received");
  };
};

class WeightCallbacks : public NimBLECharacteristicCallbacks
{
  void onRead(NimBLECharacteristic *pCharacteristic)
  {
    debugPrint(pCharacteristic->getUUID().toString());
    debugPrint(": onRead(), value: ");
    debugPrintln(pCharacteristic->getValue());
  };

  /** Called before notification or indication is sent,
        the value can be changed here before sending if desired.
    */
  void onNotify(NimBLECharacteristic *pCharacteristic)
  {
    debugPrintln("Sending notification to clients");
    
    pCharacteristic->setValue("10.1"); //_sys->weight->getWeightStr().c_str());
  };

  /** The status returned in status is defined in NimBLECharacteristic.h.
        The value returned in code is the NimBLE host return code.
    */
  void onStatus(NimBLECharacteristic *pCharacteristic, Status status, int code)
  {
    /*String str = ("Notification/Indication status code: ");
        str += status;
        str += ", return code: ";
        str += code;
        str += ", ";
        str += NimBLEUtils::returnCodeToString(code);
        debugPrintln(str); */
    // saves 108 bytes
    debugPrint("Notification/Indication status code: ");
    debugPrint(status);
    debugPrint(", return code: ");
    debugPrint(code);
    debugPrint(", ");
    debugPrintln(NimBLEUtils::returnCodeToString(code));
  };
};

class ActionCallbacks : public NimBLECharacteristicCallbacks
{
  void onRead(NimBLECharacteristic *pCharacteristic)
  {
    debugPrint(pCharacteristic->getUUID().toString().c_str());
    debugPrint(": onRead(), value: ");
    debugPrintln(pCharacteristic->getValue().c_str());
  };

  void onWrite(NimBLECharacteristic *pCharacteristic)
  {
    debugPrint(pCharacteristic->getUUID().toString().c_str());
    debugPrint(": onWrite(), value: ");
    debugPrintln(pCharacteristic->getValue().c_str());
    static WiFiStruct w;
    static std::string sLine = "";
    static std::string pLine = "";

    // if writing new wifi ssid
    if (strcmp(pCharacteristic->getUUID().toString().c_str(), "0000551d-60be-11eb-ae93-0242ac130002") == 0)
    {
      w.active = 0;
      sLine.append(pCharacteristic->getValue());

      if(pCharacteristic->getValue().substr(pCharacteristic->getValue().length()-2).compare("]$") == 0){
        strcpy(w.ssid, sLine.substr(pLine.length()-2).c_str());
        sLine.clear();
      }
    }

    // if writing new wifi password
    if (strcmp(pCharacteristic->getUUID().toString().c_str(), "0000fa55-60be-11eb-ae93-0242ac130002") == 0)
    {
      // only check wifi and save after password is saved
      debugPrintln("writing password in callback");
      pLine.append(pCharacteristic->getValue());

      if(pCharacteristic->getValue().substr(pCharacteristic->getValue().length()-2).compare("]$") == 0){

        strcpy(w.pswd, pLine.substr(pLine.length()-2).c_str());
        if (verifyWiFiInfo(w))
        {
          w.active = 1;
          setWiFiInfo(w);
        }
        pLine.clear();
      }
    }
    
    // if turning off remotely
    if (strcmp(pCharacteristic->getUUID().toString().c_str(), "000000ff-60be-11eb-ae93-0242ac130002" ) == 0)
    {
      if(pCharacteristic->getValue().compare("yes") == 0)
      {
        _sys->goToSleep();
      }
    }

    // if restarting remotely
    if (strcmp(pCharacteristic->getUUID().toString().c_str(), "0000b007-60be-11eb-ae93-0242ac130002" ) == 0)
    {
      debugPrintln("Action onwrite callback. Correct UUID.");
      if(pCharacteristic->getValue().compare("yes") == 0)
      {
        
        debugPrintln("reboot compare is correct");
        _sys->reboot();
      }
    }

    // if starting OTA update
    if (strcmp(pCharacteristic->getUUID().toString().c_str(), "0000007a-60be-11eb-ae93-0242ac130002" ) == 0)
    {
      debugPrintln("Action onwrite callback. Correct UUID.");
      if(pCharacteristic->getValue().compare("yes") == 0)
      {
        
        debugPrintln("OTA compare is correct");
        _sys->reboot();
      }
    }

  };
  /** Called before notification or indication is sent,
        the value can be changed here before sending if desired.
    */
  void onNotify(NimBLECharacteristic *pCharacteristic)
  {
    debugPrintln("Sending notification to clients");
  };

  /** The status returned in status is defined in NimBLECharacteristic.h.
        The value returned in code is the NimBLE host return code.
    */
  void onStatus(NimBLECharacteristic *pCharacteristic, Status status, int code)
  {
    /*String str = ("Notification/Indication status code: ");
        str += status;
        str += ", return code: ";
        str += code;
        str += ", ";
        str += NimBLEUtils::returnCodeToString(code);
        debugPrintln(str); */
    // saves 108 bytes
    debugPrint("Notification/Indication status code: ");
    debugPrint(status);
    debugPrint(", return code: ");
    debugPrint(code);
    debugPrint(", ");
    debugPrintln(NimBLEUtils::returnCodeToString(code));
  };
};

class OTACallbacks : public NimBLECharacteristicCallbacks
{
  void onRead(NimBLECharacteristic *pCharacteristic)
  {
    debugPrint(pCharacteristic->getUUID().toString().c_str());
    debugPrint(": onRead(), value: ");
    debugPrintln(pCharacteristic->getValue().c_str());
  };

  void onWrite(NimBLECharacteristic *pCharacteristic)
  {
    debugPrint(pCharacteristic->getUUID().toString().c_str());
    debugPrint(": onWrite(), value: ");
    debugPrintln(pCharacteristic->getValue().c_str());

    //if OTA update is set to "yes" from bluetooth
    //std::string s = "yes";
    // debugPrintln(pCharacteristic->getValue().compare("yes"));

    if (strcmp(pCharacteristic->getValue().c_str(), "yes") == 0)
    {
      debugPrintln("Begin updating..........");
      vTaskDelay(20);
      //_sys->runUpdate();
      debugPrintln("page change");
    }
    else
    {
      debugPrintln("OTA Command not recognized");
    }
  };
};

static DeviceCallbacks devCallbacks;
static BatteryCallbacks batCallbacks;
static WeightCallbacks wgtCallbacks;
static ActionCallbacks actCallbacks;
static OTACallbacks otaCallbacks;



void preBLEsetup(){
  NimBLEDevice::init("SudoBoard2");
}

void BLEsetup(std::string SN, std::string Version, int battery, Units units, WiFiStruct w)
{
  debugPrint("Starting NimBLE Server: ");
  debugPrintln((int)esp_timer_get_time()/1000);

  bleWeightQueue = xQueueCreate(5, 10*sizeof(char));
  bleUnitsQueue = xQueueCreate(5, sizeof(Units));
  bleBatteryQueue = xQueueCreate(5, sizeof(int));
  bleStatusQueue = xQueueCreate(5, sizeof(int)); 

  /** sets device name */
  //NimBLEDevice::init("SudoBoard2");
  debugPrintln("1");
  /** Optional: set the transmit power, default is 3db */
  NimBLEDevice::setPower(ESP_PWR_LVL_P3); /** +9db */
  debugPrintln("2");
  /** Set the IO capabilities of the device, each option will trigger a different pairing method.
      BLE_HS_IO_DISPLAY_ONLY    - Passkey pairing
      BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
      BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
  */
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  debugPrintln("3");
  /** 2 different ways to set security - both calls achieve the same result.
      no bonding, no man in the middle protection, secure connections.
      These are the default values, only shown here for demonstration.
  */
  //NimBLEDevice::setSecurityAuth(false, false, true);
  NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);
  debugPrintln("4");
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  debugPrintln("5");
  NimBLEService *pDeviceService = pServer->createService("180A");
  NimBLECharacteristic *pSerialNumCharacteristic = pDeviceService->createCharacteristic(
      "2A25",
      NIMBLE_PROPERTY::READ //|
      //NIMBLE_PROPERTY::WRITE |
      /** Require a secure connection for read and write access */
      //NIMBLE_PROPERTY::READ_ENC // only allow reading if paired / encrypted
      //NIMBLE_PROPERTY::WRITE_ENC   // only allow writing if paired / encrypted
  );

  debugPrintln("6");

  pSerialNumCharacteristic->setValue(SN);//_sys->getSN());

  pSerialNumCharacteristic->setCallbacks(&devCallbacks);

  NimBLECharacteristic *pSoftwareCharacteristic = pDeviceService->createCharacteristic(
      "2A28",
      NIMBLE_PROPERTY::READ //|
          //NIMBLE_PROPERTY::WRITE |
          /** Require a secure connection for read and write access */
          //NIMBLE_PROPERTY::READ_ENC // only allow reading if paired / encrypted
      //NIMBLE_PROPERTY::WRITE_ENC   // only allow writing if paired / encrypted
  );

  pSoftwareCharacteristic->setValue(Version);//_sys->getVER());

  pSoftwareCharacteristic->setCallbacks(&devCallbacks);

  debugPrintln("7");
  NimBLECharacteristic *pMfgCharacteristic = pDeviceService->createCharacteristic(
      "2A29",
      NIMBLE_PROPERTY::READ //|

          /** Require a secure connection for read and write access */
          //NIMBLE_PROPERTY::READ_ENC // only allow reading if paired / encrypted

  );

  pMfgCharacteristic->setValue("SudoChef");
  pMfgCharacteristic->setCallbacks(&devCallbacks);
  debugPrintln("8");
  // NimBLECharacteristic *pDateTimeCharacteristic = pDeviceService->createCharacteristic(
  //     "2A11",
  //         NIMBLE_PROPERTY::READ |
  //         NIMBLE_PROPERTY::WRITE |
  //         /** Require a secure connection for read and write access */
  //         NIMBLE_PROPERTY::READ_ENC | // only allow reading if paired / encrypted
  //         NIMBLE_PROPERTY::WRITE_ENC  // only allow writing if paired / encrypted
  // );

  // pDateTimeCharacteristic->setValue("02-08-2021");
  // pDateTimeCharacteristic->setCallbacks(&devCallbacks);
  debugPrintln("9");

  /* Next Service - Battery  */
  NimBLEService *pBatteryService = pServer->createService("180F");
  NimBLECharacteristic *pBatteryCharacteristic = pBatteryService->createCharacteristic(
      "2A19",
      NIMBLE_PROPERTY::READ  );
  char srerdsf[16];
  sprintf(srerdsf, "%d %%", _sys->getBattery());

  pBatteryCharacteristic->setValue(battery); //srerdsf);
  pBatteryCharacteristic->setCallbacks(&batCallbacks);
  debugPrintln("10");
  /* Next Service - Weight Scale */
  NimBLEService *pWeightService = pServer->createService("181D");

  NimBLECharacteristic *pWeightCharacteristic = pWeightService->createCharacteristic(
      "2A9E",
      NIMBLE_PROPERTY::READ |

          /** Require a secure connection for read and write access */
          //NIMBLE_PROPERTY::READ_ENC | // only allow reading if paired / encrypted
          NIMBLE_PROPERTY::NOTIFY);

  pWeightCharacteristic->setValue("0.0");
  pWeightCharacteristic->setCallbacks(&wgtCallbacks);
  debugPrintln("11");
  NimBLECharacteristic *pUnitsCharacteristic = pWeightService->createCharacteristic(
      "2B46",
      NIMBLE_PROPERTY::READ |
          NIMBLE_PROPERTY::WRITE |
          /** Require a secure connection for read and write access */
          //NIMBLE_PROPERTY::READ_ENC | // only allow reading if paired / encrypted
          //NIMBLE_PROPERTY::WRITE_ENC  // only allow writing if paired / encrypted
      NIMBLE_PROPERTY::NOTIFY
  );
  
  pUnitsCharacteristic->setValue(unitsToString(units)); //_sys->getUnits()));

  pUnitsCharacteristic->setCallbacks(&wgtCallbacks);

  debugPrintln("12");
  /* Next Service - Actions performed through connection */

  NimBLEService *pActionService = pServer->createService("00005ac7-60be-11eb-ae93-0242ac130002");
  NimBLECharacteristic *pOTACharacteristic = pActionService->createCharacteristic(
      "0000007a-60be-11eb-ae93-0242ac130002",
          NIMBLE_PROPERTY::WRITE// |
          /** Require a secure connection for read and write access */
          //NIMBLE_PROPERTY::WRITE_ENC  // only allow writing if paired / encrypted
  );

  pOTACharacteristic->setValue("no");
  pOTACharacteristic->setCallbacks(&otaCallbacks);
  debugPrintln("13");

  NimBLECharacteristic *pRebootCharacteristic = pActionService->createCharacteristic(
      "0000b007-60be-11eb-ae93-0242ac130002",
          NIMBLE_PROPERTY::WRITE// |
          /** Require a secure connection for read and write access */
          //NIMBLE_PROPERTY::WRITE_ENC  // only allow writing if paired / encrypted
  );

  pRebootCharacteristic->setValue("no");
  pRebootCharacteristic->setCallbacks(&actCallbacks);

  debugPrintln("14");
  NimBLECharacteristic *pOffCharacteristic = pActionService->createCharacteristic(
      "000000ff-60be-11eb-ae93-0242ac130002",
          NIMBLE_PROPERTY::WRITE //|
          /** Require a secure connection for read and write access */
         // NIMBLE_PROPERTY::WRITE_ENC  // only allow writing if paired / encrypted
  );

  pOffCharacteristic->setValue("no");
  pOffCharacteristic->setCallbacks(&actCallbacks);
  debugPrintln("15");

  // char wifi_read[100] = {};
  // char w_ssid[32] = {};
  // char w_pass[64] = {};
  // // WiFiStruct w (1, "apple", "pie");//= _sys->getWiFiInfo();
  // sprintf(w_ssid, "%s", w.ssid);
  // sprintf(w_pass, "%s", w.pswd);
  //sprintf(wifi_read, "%s,%s", w.ssid, w.pswd);
  // debugPrint(" wifi_read value: ");
  // debugPrintln(wifi_read);

  NimBLECharacteristic *pSSIDCharacteristic = pActionService->createCharacteristic(
      "0000551d-60be-11eb-ae93-0242ac130002",
      NIMBLE_PROPERTY::READ |
          NIMBLE_PROPERTY::WRITE //|
          //NIMBLE_PROPERTY::READ_ENC | // only allow reading if paired / encrypted
          //NIMBLE_PROPERTY::WRITE_ENC  // only allow writing if paired / encrypted
  );

  //pSSIDCharacteristic->setValue((uint8_t*)w_ssid, strlen(w_ssid)+1);
  pSSIDCharacteristic->setValue(w.ssid);
  pSSIDCharacteristic->setCallbacks(&actCallbacks);
  debugPrintln("16");
  NimBLECharacteristic *pPASSCharacteristic = pActionService->createCharacteristic(
      "0000fa55-60be-11eb-ae93-0242ac130002",
      NIMBLE_PROPERTY::READ |
          NIMBLE_PROPERTY::WRITE //|
          //NIMBLE_PROPERTY::READ_ENC | // only allow reading if paired / encrypted
          //NIMBLE_PROPERTY::WRITE_ENC  // only allow writing if paired / encrypted
  );

  pPASSCharacteristic->setValue(w.pswd);
  //pPASSCharacteristic->setValue("shortpass");
  pPASSCharacteristic->setCallbacks(&actCallbacks);
  debugPrintln("17");

  NimBLECharacteristic *pStatusCharacteristic = pActionService->createCharacteristic(
      "000057a7-60be-11eb-ae93-0242ac130002",
      NIMBLE_PROPERTY::READ |
          //NIMBLE_PROPERTY::WRITE //|
          //NIMBLE_PROPERTY::READ_ENC | // only allow reading if paired / encrypted
          //NIMBLE_PROPERTY::WRITE_ENC  // only allow writing if paired / encrypted
          NIMBLE_PROPERTY::NOTIFY
  );

  pStatusCharacteristic->setValue("100");
  pStatusCharacteristic->setCallbacks(&actCallbacks);
  debugPrintln("17");

  /** Start the services when finished creating all Characteristics and Descriptors */
  pDeviceService->start();
  pBatteryService->start();
  pWeightService->start();
  pActionService->start();
  debugPrintln("18");
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();

  /** Add the services to the advertisment data **/
  pAdvertising->addServiceUUID(pDeviceService->getUUID());
  pAdvertising->addServiceUUID(pBatteryService->getUUID());
  pAdvertising->addServiceUUID(pWeightService->getUUID());
  pAdvertising->addServiceUUID(pActionService->getUUID());
  debugPrintln("19");
  /** If your device is battery powered you may consider setting scan response
      to false as it will extend battery life at the expense of less data sent.
  */
  pAdvertising->setScanResponse(true);
  debugPrintln("20");
  pAdvertising->start();

  debugPrint("Advertising Started: ");
  debugPrintln((int)esp_timer_get_time()/1000);

  xTaskCreatePinnedToCore(BLETask, "BLETask", 5000, NULL, 1, &BLETask_h, 0);
}


void BLEstop(){
  debugPrintln("after BLEstop");
  if(size_t s = NimBLEDevice::getClientListSize() ){
    debugPrintln("inside disconnecting ble");
  std::list<NimBLEClient *> b = *NimBLEDevice::getClientList(); 
  std::list<NimBLEClient *>::iterator it = b.begin();

  for(int i = 0; i < s; i++)
  {
      debugPrintln("Bluetooth is connected - disconnect iterator");
      
      NimBLEDevice::deleteClient(*it);
      std::advance(it,1);
  }
  debugPrintln("All bluetooth connections closed");
  //return -1;

  }
  debugPrintln("after BLEstop1");
  pServer->stopAdvertising();
  debugPrintln("after BLEstop2");
  // NimBLEDevice::deinit(true);
  debugPrintln("after BLEsto3");
  vTaskDelete(BLETask_h);
  vQueueDelete( bleWeightQueue);
  vQueueDelete(bleUnitsQueue);
  vQueueDelete (bleBatteryQueue);
  vQueueDelete( bleStatusQueue);

  debugPrintln("after BLEstop4");

}