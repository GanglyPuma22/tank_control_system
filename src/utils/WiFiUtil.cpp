#include "WiFiUtil.h"

// Global variables implementation
struct_message myData;
esp_now_peer_info_t WiFiUtil::peerInfo;

namespace WiFiUtil {
void defaultOnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success"
                                                : "Delivery Fail");
}

void defaultOnDataRecv(const uint8_t *mac_addr, const uint8_t *data,
                       int data_len) {
  memcpy(&myData, data, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(data_len);
}
} // namespace WiFiUtil
