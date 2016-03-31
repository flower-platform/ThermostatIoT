#include <WiFi101.h>
#include <WiFiUDP.h>
#include <FlowerOTA.h>
#include <AzureIoT.h>
#include <DHT.h>
#include "Output.h"

char ssid[] = "wifi-work"; //  your network SSID (name)
char pass[] = "wifi-work-pass-123";    // your network password (use for WPA, or use as key for WEP)
IPAddress ip(192, 168, 100, 251);
IPAddress mask(255, 255, 255, 0);
IPAddress dns(192, 168, 100, 1);
IPAddress gateway(192, 168, 100, 1);
const char* connectionString = "HostName=arduino-mkr1000.azure-devices.net;DeviceId=mkr1000;SharedAccessKey=H3pZf6Y3MtKqJ1u5zQarKQ==";
int context = 0;

IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

FlowerOTA ota;
//WiFiClient client;
WiFiSSLClient client;
WiFiUDP udp;
DHT dht(6, DHT11);
int presetTemperature = 27;
Output heater;

IOTHUBMESSAGE_DISPOSITION_RESULT ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback) {
  const char* buffer;
  size_t size;
  if (IoTHubMessage_GetByteArray(message, (const unsigned char**) &buffer, &size) != IOTHUB_MESSAGE_OK) {
    printf("unable to retrieve the message data\r\n");
    return IOTHUBMESSAGE_ACCEPTED;
  }

  (void) printf("Received Message with Data: <<<%.*s>>> & Size=%d\r\n", (int) size, buffer, (int) size);
  const char* PREFIX_OTAUPLOAD = "OTAUPLOAD";
  if (strncmp(buffer, PREFIX_OTAUPLOAD, strlen(PREFIX_OTAUPLOAD)) == 0) { // perform update
      const char* url = buffer + 9; // skip "OTAUPLOAD"
      ota.getUpdate(url);
  } else {
    char* eq = strchr(buffer, '=') + 1;
    int i = (int) (eq - buffer);
    char v[8];
    
    memcpy(v, eq, size - i);
    v[i] = '\0';
    presetTemperature = atoi(v);
  }

  return IOTHUBMESSAGE_ACCEPTED;
}

void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback) {
  (void) printf("Confirmation received for message with result = %s\r\n", result);
  IoTHubMessage_Destroy(messageHandle);
  messageHandle = NULL;
}

void iotSendMessage(const char* msg) {
  if (messageHandle != NULL) {
    printf("busy\r\n");
    return;
  }
  (void) printf("sending %s \r\n", msg);
  messageHandle = IoTHubMessage_CreateFromString(msg);
  IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, SendConfirmationCallback, &context);
}

void setup() {
//  Serial.begin(115200);
//  while(!Serial);
  
  // setup and connect to WiFi
  int status = WL_IDLE_STATUS;
  WiFi.config(ip, dns, gateway, mask);
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    if (status != WL_CONNECTED) {
      delay(10000);
    }
  }
  Serial.println("Connected to wifi");
  
  // setup OTA updates
//  ota.begin(&udp, &client, "YKLdXKfs6VW9BiHewutvhiUCvB2gnf0KUOpry7qJM4g=");
  ota.begin(&udp, &client, "signature");

  // setup heater
  heater.pin = 7;
  heater.initialValue = HIGH;
  heater.setup();
  
  //setup IoT Hub
  iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, HTTP_Protocol);
  unsigned int timeout = 241000;
  IoTHubClient_LL_SetOption(iotHubClientHandle, "timeout", &timeout);
  unsigned int minimumPollingTime = 1; /*because it can poll "after 9 seconds" polls will happen effectively at ~10 seconds*/
  IoTHubClient_LL_SetOption(iotHubClientHandle, "MinimumPollingTime", &minimumPollingTime);
  IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, ReceiveMessageCallback, &context);

}

int deadline = 0;
int deadline_iot = 0;

void loop() {
  ota.loop();
  
  if (millis() > deadline_iot) {
    Serial.println("iot loop1");
    IoTHubClient_LL_DoWork(iotHubClientHandle);
    deadline_iot = millis() + 1000;

      // check temperature and set heater state
    int t = dht.readTemperature();
    int heaterValue = heater.getValue();
    if (t < presetTemperature) {
      heater.setValue(LOW); // turn on heater (relay input is active low)
    } else {
      heater.setValue(HIGH); // turn off heater
    }
  }

  if (millis() > deadline) {
    char msg[256];
    int t = dht.readTemperature();
    int h = dht.readHumidity();
    sprintf(msg, "{\"temperature\":%d, \"presetTemperature\":%d, \"heater\":%d, \"humidity\":%d}", t, presetTemperature, heater.getValue(), h);
    iotSendMessage(msg);
    deadline = millis() + 15000;
  }

}

