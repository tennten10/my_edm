#include "globals.h"
//#include <TFT_eSPI.h>
//#include <SPI.h>

//TFT_eSPI tft;
//PAGE ePage;

/*class Display {
public:
    bool                                        start();
    bool                                        end();
    bool                                        off();
    bool                                        weightScreen();
    bool                                        unitsScreen
    bool                                        settingsScreen();
    bool                                        batteryScreen();
    bool                                        logoScreen();
    bool                                        updateScreen();
    void                                        setMenu();
    void                                        setColor();
    void                                        setBrightness(int brightness = 50);



private:
    Display(const NimBLEAddress &peerAddress);
    ~Display();

    friend class            NimBLEDevice;
    friend class            NimBLERemoteService;

    static int              handleGapEvent(struct ble_gap_event *event, void *arg);
    static int              serviceDiscoveredCB(uint16_t conn_handle,
                                                const struct ble_gatt_error *error,
                                                const struct ble_gatt_svc *service,
                                                void *arg);
    static void             dcTimerCb(ble_npl_event *event);
    bool                    retrieveServices(const NimBLEUUID *uuid_filter = nullptr);

    NimBLEAddress           m_peerAddress;
    uint16_t                m_conn_id;
    bool                    m_connEstablished;
    bool                    m_deleteCallbacks;
    int32_t                 m_connectTimeout;


}; // class Display

*/


void displaySetup();
void displayWeight(float weight);
void displayUnits();
void displaySettings();
void displayDeviceInfo();
void displayUpdateScreen();
void displayLogo();
void displayBattery();
void displayOff();

void displayHandler(void * pvParameters);
void menu();