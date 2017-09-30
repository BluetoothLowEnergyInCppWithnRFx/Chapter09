#include "mbed.h"
#include "ble/BLE.h"
#include "ble/services/DeviceInformationService.h"


/** User interface I/O **/

// instantiate USB Serial
Serial serial(USBTX, USBRX);

// Status LED
DigitalOut statusLed(LED1, 0);

// Timer for blinking the statusLed
Ticker ticker;


/** Bluetooth Peripheral Properties **/

// Broadcast name
static char BROADCAST_NAME[] = "MyDevice";

// Device Information UUID
static const uint16_t deviceInformationServiceUuid  = GattService::UUID_DEVICE_INFORMATION_SERVICE; //0x180a;

// Battery Level UUID
static const uint16_t batteryLevelServiceUuid  = GattService::UUID_BATTERY_SERVICE; //0x180f;

// array of all Service UUIDs
static const uint16_t uuid16_list[] = { GattService::UUID_DEVICE_INFORMATION_SERVICE, batteryLevelServiceUuid };

// Number of bytes in Characteristic
static const uint8_t characteristicLength = 20;

// Device Name Characteristic UUID
static const uint16_t deviceNameCharacteristicUuid = GattCharacteristic::UUID_MANUFACTURER_NAME_STRING_CHAR; //0x2a00;

// Modul Number Characteristic UUID
static const uint16_t modelNumberCharacteristicUuid = GattCharacteristic::UUID_MODEL_NUMBER_STRING_CHAR; //0x2a24;

// Serial Number Characteristic UUID
static const uint16_t serialNumberCharacteristicUuid = GattCharacteristic::UUID_SERIAL_NUMBER_STRING_CHAR; //0x2a25;

// Battery Level Characteristic UUID
static const uint16_t batteryLevelCharacteristicUuid = GattCharacteristic::UUID_BATTERY_LEVEL_CHAR; //0x2a19;

// model and serial numbers
static char modelNumber[characteristicLength] = "1AB2";
static char serialNumber[characteristicLength] = "1234";
uint8_t batteryLevel = 100;


/** Functions **/

/**
 * visually signal that program has not crashed
 */
void blinkHeartbeat(void);

/**
 * Callback triggered when the ble initialization process has finished
 *
 * @param[in] params Information about the initialized Peripheral
 */
void onBluetoothInitialized(BLE::InitializationCompleteCallbackContext *params);

/**
 * Callback handler when a Central has disconnected
 * 
 * @param[i] params Information about the connection
 */
void onCentralDisconnected(const Gap::DisconnectionCallbackParams_t *params);


/** Build Service and Characteristic Relationships **/
// Device Name
GattCharacteristic deviceNameCharacteristic(deviceNameCharacteristicUuid, (uint8_t *)BROADCAST_NAME, sizeof(BROADCAST_NAME), sizeof(BROADCAST_NAME),
                           GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);
                           
// Model
GattCharacteristic modelNumberCharacteristic(modelNumberCharacteristicUuid, (uint8_t *)modelNumber, sizeof(modelNumber), sizeof(modelNumber),
                           GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);
                           
 // Serial number
GattCharacteristic serialNumberCharacteristic(serialNumberCharacteristicUuid, (uint8_t *)serialNumber, sizeof(serialNumber), sizeof(serialNumber),
                           GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);
                           
GattCharacteristic *deviceInformationCharacteristics[] = { &deviceNameCharacteristic, &modelNumberCharacteristic, &serialNumberCharacteristic };
GattService         deviceInformationService(deviceInformationServiceUuid, deviceInformationCharacteristics, sizeof(deviceInformationCharacteristics) / sizeof(GattCharacteristic *));

GattCharacteristic batteryLevelCharacteristic(batteryLevelCharacteristicUuid, (uint8_t *) batteryLevel, sizeof(batteryLevel), sizeof(batteryLevel), 
                           GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY);
                           
GattCharacteristic *batteryLevelCharacteristics[] = { &batteryLevelCharacteristic };
GattService         batteryLevelService(batteryLevelServiceUuid, batteryLevelCharacteristics, sizeof(batteryLevelCharacteristics) / sizeof(GattCharacteristic *));



/**
 * Main program and loop
 */
int main(void) {
    serial.baud(9600);
    serial.printf("Starting Peripheral\r\n");

    ticker.attach(blinkHeartbeat, 1); // Blink status led every 1 second

    // initialized Bluetooth Radio
    BLE &ble = BLE::Instance(BLE::DEFAULT_INSTANCE);
    ble.init(onBluetoothInitialized);

    
    // wait for Bluetooth Radio to be initialized
    while (ble.hasInitialized()  == false);

    while (1) {
        // save power when possible
        ble.waitForEvent();
    }
}


void blinkHeartbeat(void) {
    statusLed = !statusLed; /* Do blinky on LED1 to indicate system aliveness. */
}


void onBluetoothInitialized(BLE::InitializationCompleteCallbackContext *params) {
    BLE&        ble   = params->ble;
    ble_error_t error = params->error;

    // quit if there's a problem
    if (error != BLE_ERROR_NONE) {
        return;
    }

    // Ensure that it is the default instance of BLE 
    if(ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }

    serial.printf("Describing Peripheral...");
    
    // Shortcut to setting up device information service
    //DeviceInformationService *deviceInformationService = new DeviceInformationService(ble, BROADCAST_NAME, modelNumber, serialNumber);
    
    // shortcut to setting up battery level service
    // BatteryService batteryService(ble);
    // batteryService.updateBatteryLevel(batteryLevel);
 
    // attach Services
    ble.addService(deviceInformationService);
    ble.addService(batteryLevelService);     
    
    
    // process disconnections with a callback
    ble.gap().onDisconnection(onCentralDisconnected);

    // advertising parametirs
    ble.gap().accumulateAdvertisingPayload(
        GapAdvertisingData::BREDR_NOT_SUPPORTED |   // Device is Peripheral only
        GapAdvertisingData::LE_GENERAL_DISCOVERABLE); // always discoverable
    // broadcast name
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)BROADCAST_NAME, sizeof(BROADCAST_NAME));
    //  advertise services
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    // allow connections
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    // advertise every 1000ms
    ble.gap().setAdvertisingInterval(1000); // 1000ms

    // begin advertising
    ble.gap().startAdvertising();
       

    serial.printf(" done\r\n");
}


void onCentralDisconnected(const Gap::DisconnectionCallbackParams_t *params) {
    BLE::Instance().gap().startAdvertising();
    serial.printf("Central disconnected\r\n");
}


