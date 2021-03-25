// #include "globals.h"
// #include "debug.h"
// //#include "AWS_OTA.h"
// #include "esp-nimble-cpp/src/NimBLEDevice.h"
// #include "esp-nimble-cpp/src/NimBLELog.h"

// #include <stdio.h>
// #include "mySPIFFS.h"
// //#include <esp_wiFi.h>
// //#include <esp_err.h>
// //#include "Weight.h"
// #include <string>


// //TaskHandle_t bluetooth_TH;
// extern SemaphoreHandle_t systemMutex;
// //const byte numChars = 32;
// //extern volatile PAGE ePage;
// //extern SemaphoreHandle_t pageMutex;

// static NimBLEServer* pServer;


// //Units convertUnitsEnum(String v){
// //  Units retVal;
// //  if(v.equals("g")){
// //    retVal = g;
// //    return retVal;
// //  }else if(v.equals("kg")){
// //    retVal = kg;
// //    return retVal;
// //  }else if(v.equals("oz")){
// //    retVal = oz;
// //    return retVal;
// //  }else if(v.equals("lb")){
// //    retVal = lb;
// //    return retVal;
// //  }
// //  // g, kg, oz, lb
// //}
// /* Uncomment this one later
// bool isBtConnected() {
//   if (pServer->getConnectedCount() > 0) {
//     return true;
//   } else {
//     return false;
//   }
// }*/




// /* */
// /* BLE changes below this. Probably deleting all previous stuff, but will comment out for now. */
// /* */

// class ServerCallbacks: public NimBLEServerCallbacks {
//     void onConnect(NimBLEServer* pServer) {
//       debugPrintln("Client connected");
//       debugPrintln("Multi-connect support: start advertising");
//       NimBLEDevice::startAdvertising();
//     };
//     /** Alternative onConnect() method to extract details of the connection.
//         See: src/ble_gap.h for the details of the ble_gap_conn_desc struct.
//     */
//     void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
//       debugPrint("Client address: ");
//       debugPrintln(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
//       /** We can use the connection handle here to ask for different connection parameters.
//           Args: connection handle, min connection interval, max connection interval
//           latency, supervision timeout.
//           Units; Min/Max Intervals: 1.25 millisecond increments.
//           Latency: number of intervals allowed to skip.
//           Timeout: 10 millisecond increments, try for 5x interval time for best results.
//       */
//       pServer->updateConnParams(desc->conn_handle, 24, 48, 0, 60);
//     };
//     void onDisconnect(NimBLEServer* pServer) {
//       debugPrintln("Client disconnected - start advertising");
//       NimBLEDevice::startAdvertising();
//     };

//     /********************* Security handled here **********************
//     ****** Note: these are the same return values as defaults ********/
//     uint32_t onPassKeyRequest() {
//       debugPrintln("Server Passkey Request");
//       /** This should return a random 6 digit number for security
//           or make your own static passkey as done here.
//       */
//       return 123456;
//     };

//     bool onConfirmPIN(uint32_t pass_key) {
//       debugPrint("The passkey YES/NO number: "); debugPrintln((int)pass_key);
//       /** Return false if passkeys don't match. */
//       return true;
//     };

//     void onAuthenticationComplete(ble_gap_conn_desc* desc) {
//       /** Check that encryption was successful, if not we disconnect the client */
//       if (!desc->sec_state.encrypted) {
//         NimBLEDevice::getServer()->disconnect(desc->conn_handle);
//         debugPrintln("Encrypt connection failed - disconnecting client");
//         return;
//       }
//       debugPrintln("Starting BLE work!");
//     };
// };

// /** Handler class for characteristic actions */
// class DeviceCallbacks: public NimBLECharacteristicCallbacks {
//     void onRead(NimBLECharacteristic* pCharacteristic) {
//       debugPrint(pCharacteristic->getUUID().toString().c_str());
//       debugPrint(": onRead(), value: ");
//       debugPrintln(pCharacteristic->getValue().c_str());
//     };

//     void onWrite(NimBLECharacteristic* pCharacteristic) {
//       debugPrint(pCharacteristic->getUUID().toString().c_str());
//       debugPrint(": onWrite(), value: ");
//       debugPrintln(pCharacteristic->getValue().c_str());
//     };
//     /** Called before notification or indication is sent,
//         the value can be changed here before sending if desired.
//     */
//     void onNotify(NimBLECharacteristic* pCharacteristic) {
//       debugPrintln("Sending notification to clients");
//     };


//     /** The status returned in status is defined in NimBLECharacteristic.h.
//         The value returned in code is the NimBLE host return code.
//     */
//     void onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code) {
//       debugPrint("Notification/Indication status code: ");
//       debugPrint(status);
//       debugPrint(", return code: ");
//       debugPrint( code);
//       debugPrint(", ");
//       debugPrint( NimBLEUtils::returnCodeToString(code));
//       debugPrintln("");
//     };

    
// };

// /** Handler class for battery service actions */
// class BatteryCallbacks: public NimBLECharacteristicCallbacks {
//     void onRead(NimBLECharacteristic* pCharacteristic) {
//       debugPrint(pCharacteristic->getUUID().toString().c_str());
//       xSemaphoreTake(systemMutex, (TickType_t)10);
//       pCharacteristic->setValue(_sys.batteryLevel);
//       xSemaphoreGive(systemMutex);
//       debugPrint(": onRead(), value: ");
//       debugPrintln(pCharacteristic->getValue().c_str());
//       debugPrintln("getBattery received");
//     };
// };

// // class WeightCallbacks: public NimBLECharacteristicCallbacks {
// //     void onRead(NimBLECharacteristic* pCharacteristic) {
// //       debugPrint(pCharacteristic->getUUID().toString().c_str());
// //       debugPrint(": onRead(), value: ");
// //       debugPrintln(pCharacteristic->getValue().c_str());

// //     };

// //     /** Called before notification or indication is sent,
// //         the value can be changed here before sending if desired.
// //     */
// //     void onNotify(NimBLECharacteristic* pCharacteristic) {
// //       debugPrintln("Sending notification to clients");
// //     };


// //     /** The status returned in status is defined in NimBLECharacteristic.h.
// //         The value returned in code is the NimBLE host return code.
// //     */
// //     void onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code) {
// //       /*String str = ("Notification/Indication status code: ");
// //         str += status;
// //         str += ", return code: ";
// //         str += code;
// //         str += ", ";
// //         str += NimBLEUtils::returnCodeToString(code);
// //         debugPrintln(str); */ // saves 108 bytes
// //       debugPrint("Notification/Indication status code: ");
// //       debugPrint(status);
// //       debugPrint(", return code: ");
// //       debugPrint( code);
// //       debugPrint(", ");
// //       debugPrintln( NimBLEUtils::returnCodeToString(code));

// //     };

   
// // };

// // class ActionCallbacks: public NimBLECharacteristicCallbacks {
// //     void onRead(NimBLECharacteristic* pCharacteristic) {
// //       debugPrint(pCharacteristic->getUUID().toString().c_str());
// //       debugPrint(": onRead(), value: ");
// //       debugPrintln(pCharacteristic->getValue().c_str());
// //     };

// //     void onWrite(NimBLECharacteristic* pCharacteristic) {
// //       debugPrint(pCharacteristic->getUUID().toString().c_str());
// //       debugPrint(": onWrite(), value: ");
// //       debugPrintln(pCharacteristic->getValue().c_str());

// //       // if writing new wifi info
// //       if (strcmp(pCharacteristic->getUUID().toString().c_str(), "0000551d-60be-11e-ae93-0242ac130002") == 0) {
// //         char line [100];
// //         WiFiStruct w;
// //         strcpy(line, pCharacteristic->getValue().c_str());
// //         w.active = 1;
// //         strcpy(w.ssid, strtok(NULL, ","));
// //         strcpy(w.pswd, strtok(NULL, ","));
// //         setWiFiInfo(w);
// //       }
// //     };
// //     /** Called before notification or indication is sent,
// //         the value can be changed here before sending if desired.
// //     */
// //     void onNotify(NimBLECharacteristic* pCharacteristic) {
// //       debugPrintln("Sending notification to clients");
// //     };


// //     /** The status returned in status is defined in NimBLECharacteristic.h.
// //         The value returned in code is the NimBLE host return code.
// //     */
// //     void onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code) {
// //       /*String str = ("Notification/Indication status code: ");
// //         str += status;
// //         str += ", return code: ";
// //         str += code;
// //         str += ", ";
// //         str += NimBLEUtils::returnCodeToString(code);
// //         debugPrintln(str); */ // saves 108 bytes
// //       debugPrint("Notification/Indication status code: ");
// //       debugPrint(status);
// //       debugPrint(", return code: ");
// //       debugPrint( code);
// //       debugPrint(", ");
// //       debugPrintln( NimBLEUtils::returnCodeToString(code));
// //     };

   
// // };

// // class OTACallbacks: public NimBLECharacteristicCallbacks {
// //     void onRead(NimBLECharacteristic* pCharacteristic) {
// //       debugPrint(pCharacteristic->getUUID().toString().c_str());
// //       debugPrint(": onRead(), value: ");
// //       debugPrintln(pCharacteristic->getValue().c_str());
// //     };

// //     void onWrite(NimBLECharacteristic* pCharacteristic) {
// //       debugPrint(pCharacteristic->getUUID().toString().c_str());
// //       debugPrint(": onWrite(), value: ");
// //       debugPrintln(pCharacteristic->getValue().c_str());

// //       //if OTA update is set to "yes" from bluetooth
// //       std::string s = "yes";
// //       debugPrintln(pCharacteristic->getValue().compare("yes"));
      
// //       if (strcmp(pCharacteristic->getValue().c_str(), "yes") == 0) {
// //         debugPrintln("Begin updating..........");
// //         //xSemaphoreTake(pageMutex, (TickType_t) 10);
// //         //ePage = pUPDATE;
// //         //xSemaphoreGive(pageMutex);
// //         //vTaskDelay(20);
// //         //setupOTA();
// //         //vTaskDelay(1000);
// //         //execOTA(0);
// //         //xSemaphoreTake(pageMutex, (TickType_t)10);
// //         //ePage = WEIGHTSTREAM;
// //         //xSemaphoreGive(pageMutex);
// //         //debugPrintln("page change");
// //       } else {
// //         debugPrintln("OTA Command not recognized");
// //       }
// //     };
// // };

// static DeviceCallbacks devCallbacks;
// // static BatteryCallbacks batCallbacks;
// // static WeightCallbacks wgtCallbacks;
// // static ActionCallbacks actCallbacks;
// // static OTACallbacks otaCallbacks;

// void updateBTWeight(char* w) {
//   if (pServer->getConnectedCount()) {
//     NimBLEService* pSvc = pServer->getServiceByUUID("181D");
//     if (pSvc) {
//       NimBLECharacteristic* pChr = pSvc->getCharacteristic("2A9E");
//       if (pChr) {
//         pChr->setValue(w);
//         pChr->notify(true); 
//       }
//     }
//   }
// }

// char* unitsToString(Units u) { // note: call free(returned_string) after this response is saved elsewhere
//   char* temp = (char*)malloc(3 * sizeof(char));
//   switch (u) {
//     case g:
//       strcpy(temp,"g");
//       return temp;
//       break;
//     case kg:
//       strcpy(temp,"kg");
//       return temp;
//       break;
//     case oz:
//       strcpy(temp,"oz");
//       return temp;
//       break;
//     case lb:
//       strcpy(temp,"lb");
//       return temp;
//       break;
//     default:
//       strcpy(temp,"");
//       return temp;
//       break;
//   }
// }



// void BLEsetup() {
//   //Serial.begin(115200);
//   debugPrintln("Starting NimBLE Server");
//   /** sets device name */
//   NimBLEDevice::init("SudoBoard");
//   /** Optional: set the transmit power, default is 3db */
//   NimBLEDevice::setPower(ESP_PWR_LVL_P3); /** +9db */
//   /** Set the IO capabilities of the device, each option will trigger a different pairing method.
//       BLE_HS_IO_DISPLAY_ONLY    - Passkey pairing
//       BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
//       BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
//   */
 
//   NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
//   /** 2 different ways to set security - both calls achieve the same result.
//       no bonding, no man in the middle protection, secure connections.

//       These are the default values, only shown here for demonstration.
//   */
//   //NimBLEDevice::setSecurityAuth(false, false, true);
//   //NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);

//   pServer = NimBLEDevice::createServer();
//   pServer->setCallbacks(new ServerCallbacks());

//   debugPrintln(" It got to here 7"); // mike look here
//   vTaskDelay(500);

//   //Holding semaphore until BT services are created...
//   xSemaphoreTake(systemMutex, (TickType_t)10);
//   debugPrintln(" It got to here 8"); // mike look here
//   vTaskDelay(500);

//   NimBLEService* pDeviceService = pServer->createService("180A");
//   NimBLECharacteristic* pSerialNumCharacteristic = pDeviceService->createCharacteristic(
//         "2A25",
//         NIMBLE_PROPERTY::READ |
//         //NIMBLE_PROPERTY::WRITE |
//         /** Require a secure connection for read and write access */
//         NIMBLE_PROPERTY::READ_ENC  // only allow reading if paired / encrypted
//         //NIMBLE_PROPERTY::WRITE_ENC   // only allow writing if paired / encrypted
//       );
//   pSerialNumCharacteristic->setValue(_sys.SN);
//   pSerialNumCharacteristic->setCallbacks(&devCallbacks);

//   debugPrintln(" It got to here 9"); // mike look here
//   vTaskDelay(500);

//   NimBLECharacteristic* pSoftwareCharacteristic = pDeviceService->createCharacteristic(
//         "2A28",
//         NIMBLE_PROPERTY::READ |
//         //NIMBLE_PROPERTY::WRITE |
//         /** Require a secure connection for read and write access */
//         NIMBLE_PROPERTY::READ_ENC  // only allow reading if paired / encrypted
//         //NIMBLE_PROPERTY::WRITE_ENC   // only allow writing if paired / encrypted
//       );
//   pSoftwareCharacteristic->setValue(_sys.VER);
//   pSoftwareCharacteristic->setCallbacks(&devCallbacks);
//   NimBLECharacteristic* pMfgCharacteristic = pDeviceService->createCharacteristic(
//         "2A29",
//         NIMBLE_PROPERTY::READ |

//         /** Require a secure connection for read and write access */
//         NIMBLE_PROPERTY::READ_ENC  // only allow reading if paired / encrypted

//       );

//   pMfgCharacteristic->setValue("SudoChef");
//   pMfgCharacteristic->setCallbacks(&devCallbacks);

//   debugPrintln(" It got to here 10"); // mike look here
//   vTaskDelay(500);

//   NimBLECharacteristic* pDateTimeCharacteristic = pDeviceService->createCharacteristic(
//         "2A11",
//         NIMBLE_PROPERTY::READ |
//         NIMBLE_PROPERTY::WRITE |
//         /** Require a secure connection for read and write access */
//         NIMBLE_PROPERTY::READ_ENC |  // only allow reading if paired / encrypted
//         NIMBLE_PROPERTY::WRITE_ENC   // only allow writing if paired / encrypted
//       );

//   pDateTimeCharacteristic->setValue("02-08-2021");
//   pDateTimeCharacteristic->setCallbacks(&devCallbacks);

//   debugPrintln(" It got to here 11"); // mike look here
//   vTaskDelay(500);

//   // /* Next Service - Battery  */ 
//   // NimBLEService* pBatteryService = pServer->createService("180F");
//   // NimBLECharacteristic* pBatteryCharacteristic = pBatteryService->createCharacteristic(
//   //       "2A19",
//   //       NIMBLE_PROPERTY::READ
//   //     );
//   // char srerdsf [16];
//   // sprintf(srerdsf, "%d %%", _sys.batteryLevel);
//   // //srerdsf = String.format("%d %%", _sys.batteryLevel);
//   // pBatteryCharacteristic->setValue(srerdsf);
//   // pBatteryCharacteristic->setCallbacks(&batCallbacks);

//   // debugPrintln(" It got to here 12"); // mike look here
//   // vTaskDelay(500);

//   // /* Next Service - Weight Scale */
//   // NimBLEService* pWeightService = pServer->createService("181D");

//   // NimBLECharacteristic* pWeightCharacteristic = pWeightService->createCharacteristic(
//   //       "2A9E",
//   //       NIMBLE_PROPERTY::READ |

//   //       /** Require a secure connection for read and write access */
//   //       NIMBLE_PROPERTY::READ_ENC  | // only allow reading if paired / encrypted
//   //       NIMBLE_PROPERTY::NOTIFY
//   //     );

//   // pWeightCharacteristic->setValue("0.0");
//   // pWeightCharacteristic->setCallbacks(&wgtCallbacks);

//   // NimBLECharacteristic* pUnitsCharacteristic = pWeightService->createCharacteristic(
//   //       "2B46",
//   //       NIMBLE_PROPERTY::READ |
//   //       NIMBLE_PROPERTY::WRITE |
//   //       /** Require a secure connection for read and write access */
//   //       NIMBLE_PROPERTY::READ_ENC |  // only allow reading if paired / encrypted
//   //       NIMBLE_PROPERTY::WRITE_ENC   // only allow writing if paired / encrypted
//   //     );
//   // char *u2s = unitsToString(_sys.eUnits);
//   // pUnitsCharacteristic->setValue(u2s);
//   // free(u2s);
//   // pUnitsCharacteristic->setCallbacks(&wgtCallbacks);

//   // //Release Semaphore
//   // xSemaphoreGive(systemMutex);

//   // debugPrintln(" It got to here 13"); // mike look here
//   // vTaskDelay(500);

//   // /* Next Service - Actions performed through connection */

//   // NimBLEService* pActionService = pServer->createService("00005AC7-60be-11eb-ae93-0242ac130002");
//   // NimBLECharacteristic* pOTACharacteristic = pActionService->createCharacteristic(
//   //       "0000007A-60be-11eb-ae93-0242ac130002",
//   //       NIMBLE_PROPERTY::READ |
//   //       NIMBLE_PROPERTY::WRITE |
//   //       /** Require a secure connection for read and write access */
//   //       NIMBLE_PROPERTY::READ_ENC |  // only allow reading if paired / encrypted
//   //       NIMBLE_PROPERTY::WRITE_ENC   // only allow writing if paired / encrypted
//   //     );

//   // pOTACharacteristic->setValue("no");
//   // pOTACharacteristic->setCallbacks(&otaCallbacks);

//   // NimBLECharacteristic* pRebootCharacteristic = pActionService->createCharacteristic(
//   //       "0000B007-60be-11eb-ae93-0242ac130002",
//   //       NIMBLE_PROPERTY::READ |
//   //       NIMBLE_PROPERTY::WRITE |
//   //       /** Require a secure connection for read and write access */
//   //       NIMBLE_PROPERTY::READ_ENC |  // only allow reading if paired / encrypted
//   //       NIMBLE_PROPERTY::WRITE_ENC   // only allow writing if paired / encrypted
//   //     );

//   // pRebootCharacteristic->setValue("no");
//   // pRebootCharacteristic->setCallbacks(&actCallbacks);

//   // NimBLECharacteristic* pOffCharacteristic = pActionService->createCharacteristic(
//   //       "000000FF-60be-11eb-ae93-0242ac130002",
//   //       NIMBLE_PROPERTY::READ |
//   //       NIMBLE_PROPERTY::WRITE |
//   //       /** Require a secure connection for read and write access */
//   //       NIMBLE_PROPERTY::READ_ENC |  // only allow reading if paired / encrypted
//   //       NIMBLE_PROPERTY::WRITE_ENC   // only allow writing if paired / encrypted
//   //     );

//   // pOffCharacteristic->setValue("no");
//   // pOffCharacteristic->setCallbacks(&actCallbacks);

//   // //make sure these are commented out/deleted after testing
//   // //File file = SPIFFS.open("/config.txt", FILE_WRITE);
//   // //file.print("0,ATT232,7272032758,\n");
//   // //file.print("0,EchoLink,schoolhouse,\n");
//   // //file.close();

//   // char wifi_read[100];
//   // WiFiStruct w = defaultWiFiInfo();
//   // sprintf(wifi_read, "%s,%s", w.ssid, w.pswd);
//   // debugPrint(" wifi_read value: ");
//   // debugPrintln(wifi_read);
//   // //File file = SPIFFS.open("/config.txt");
//   // //while (file.available()){
//   // //    debugPrintln(file.readStringUntil('\n'));
//   // //}
//   // //file.close();
//   // NimBLECharacteristic* pWiFiCharacteristic = pActionService->createCharacteristic(
//   //       "0000F1F1-60be-11eb-ae93-0242ac130002",
//   //       NIMBLE_PROPERTY::READ |
//   //       NIMBLE_PROPERTY::WRITE |

//   //       NIMBLE_PROPERTY::READ_ENC |  // only allow reading if paired / encrypted
//   //       NIMBLE_PROPERTY::WRITE_ENC   // only allow writing if paired / encrypted
//   //     );

//   // pWiFiCharacteristic->setValue((uint8_t*)wifi_read, strlen(wifi_read));
//   // pWiFiCharacteristic->setCallbacks(&actCallbacks); 

  


//   /** Start the services when finished creating all Characteristics and Descriptors */
//   pDeviceService->start();
//   //pBatteryService->start();
//   //pWeightService->start();
//   //pActionService->start();

//   NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
//   /** Add the services to the advertisment data **/
//   pAdvertising->addServiceUUID(pDeviceService->getUUID());
//   /** If your device is battery powered you may consider setting scan response
//       to false as it will extend battery life at the expense of less data sent.
//   */
//   pAdvertising->setScanResponse(true);
//   pAdvertising->start();

//   debugPrintln("Advertising Started");

// }