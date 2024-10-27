#include "bthome.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

#include "mbedtls/ccm.h"

namespace esphome {
namespace bthome {

static const char *const TAG = "bthome";

#define OID_ENTRY(oid) [oid.oid_] = oid.scan

static scan_func_t* oids[256] = {
  OID_ENTRY(oid_pid),
  OID_ENTRY(oid_battery_percent),
  OID_ENTRY(oid_temperature_celsius_x100),
  OID_ENTRY(oid_humidity_percent_x100),
  OID_ENTRY(oid_pressure_hpa_x100),
  OID_ENTRY(oid_illuminance_lux_x100),
  OID_ENTRY(oid_mass_kg_x100),
  OID_ENTRY(oid_mass_lb_x100),
  OID_ENTRY(oid_dewpoint_celsius_x100),
  OID_ENTRY(oid_counter8),
  OID_ENTRY(oid_energy_kwh_x1000),
  OID_ENTRY(oid_power_w_x100),
  OID_ENTRY(oid_voltage_v_x1000),
  OID_ENTRY(oid_pm2_5_ug_m3),
  OID_ENTRY(oid_pm10_ug_m3),
  OID_ENTRY(oid_bool),
  OID_ENTRY(oid_power),
  OID_ENTRY(oid_opening),
  OID_ENTRY(oid_co2_concentration_ppm),
  OID_ENTRY(oid_tvoc_ug_m3),
  OID_ENTRY(oid_moisture_percent_x100),
  OID_ENTRY(oid_battery),
  OID_ENTRY(oid_battery_charging),
  OID_ENTRY(oid_carbon_monoxide),
  OID_ENTRY(oid_cold),
  OID_ENTRY(oid_connectivity),
  OID_ENTRY(oid_door),
  OID_ENTRY(oid_garage_door),
  OID_ENTRY(oid_gas),
  OID_ENTRY(oid_heat),
  OID_ENTRY(oid_light),
  OID_ENTRY(oid_lock),
  OID_ENTRY(oid_moisture),
  OID_ENTRY(oid_motion),
  OID_ENTRY(oid_moving),
  OID_ENTRY(oid_occupancy),
  OID_ENTRY(oid_plug),
  OID_ENTRY(oid_presence),
  OID_ENTRY(oid_problem),
  OID_ENTRY(oid_running),
  OID_ENTRY(oid_safety),
  OID_ENTRY(oid_smoke),
  OID_ENTRY(oid_sound),
  OID_ENTRY(oid_tamper),
  OID_ENTRY(oid_vibration),
  OID_ENTRY(oid_window),
  [0x2e] = nullptr,
  [0x2f] = nullptr,
  [0x30] = nullptr,
  [0x31] = nullptr,
  [0x32] = nullptr,
  [0x33] = nullptr,
  [0x34] = nullptr,
  [0x35] = nullptr,
  [0x36] = nullptr,
  [0x37] = nullptr,
  [0x38] = nullptr,
  [0x39] = nullptr,
  OID_ENTRY(oid_button_event),
  [0x3b] = nullptr,
  [0x3c] = nullptr,
  OID_ENTRY(oid_counter16),
  OID_ENTRY(oid_counter32),
  OID_ENTRY(oid_angle_degrees_x10),
};

uint32_t read_uint(size_t size, const uint8_t *data) {
  if (size < 1 || size > 4) {
    ESP_LOGE(TAG, "read_uint() called with invalid size %zu, must be in range [1, 4]", size);
    return 0;
  }
  uint32_t val = 0;
  int shift = 0;
  for (int i=0; i<size; i++, shift+=8) {
    val |= data[i] << shift;
  }
  return val;
}

int32_t read_sint(size_t size, const uint8_t *data) {
  if (size < 1 || size > 4) {
    ESP_LOGE(TAG, "read_sint() called with invalid size %zu, must be in range [1, 4]", size);
    return 0;
  }
  bool positive = true;
  if (data[size-1] > 127) {
    positive = false;
  }

  int32_t val = 0;
  int shift = (size-1)*8;
  for (int i=size-1; i>=0; i--, shift-=8) {
    if (positive) {
      val |= data[i] << shift;
    } else {
      val |= (0xff - data[i]) << shift;
    }
  }
  if (!positive) {
    val = -val - 1;
  }
  return val;
}

// BTHome Device Information
// From https://bthome.io/format/
//
// The first byte after the UUID is the BTHome device info byte, which has
// several bits indicating the capabilities of the device.
//
//     bit 0: “Encryption flag”
//         The Encryption flag is telling the receiver wether the device is
//         sending non-encrypted data (bit 0 = 0) or encrypted data (bit 0 = 1).
//     bit 1: “Reserved for future use”
//     bit 2: “Trigger based device flag”
//         The trigger based device flag is telling the receiver that it should
//         expect that the device is sending BLE advertisements at a regular
//         interval (bit 2 = 0) or at an irregular interval (bit 2 = 1), e.g.
//         only when someone pushes a button. This can be useful information
//         for a receiver, e.g. to prevent the device from going to
//         unavailable.
//     bit 3-4: “Reserved for future use”
//     bit 5-7: “BTHome Version”
//         This represents the BTHome verion. Currently only BTHome version 1
//         or 2 are allowed, where 2 is the latest version (bit 5-7 = 010).
//
struct device_information {
  bool encryption;
  bool trigger_based;
  uint8_t bthome_version;

  device_information(uint8_t v) {
    encryption = v & 0b000000001;
    trigger_based = (v & 0b00000100) >> 2;
    bthome_version = (v & 0b11100000) >> 5;
  }
};

//
// data is a full data received from device. Must be > 9 bytes log.
// mac_address has to be 6 bytes long.
// cleartext has to be at least data_len - 9 bytes long.
int decrypt(const uint8_t *data, ssize_t data_len, const uint8_t *mac_address, uint16_t uuid, const uint8_t *key, ssize_t keybits, uint8_t *cleartext) {
  mbedtls_ccm_context ctx;
  mbedtls_ccm_init(&ctx);
  int ret = mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, keybits);
  if (ret) {
    ESP_LOGE(TAG, "mbedtls_ccm_setkey() failed: %04x", ret);
    mbedtls_ccm_free(&ctx);
    return ret;
  }

  // Nonce:
  //  MAC address 6 bytes
  //  UUID 2 bytes
  //  Device data 1 byte
  //  Counter 4 bytes
  uint8_t nonce[] = {
    [0x00] = mac_address[0x00],
    [0x01] = mac_address[0x01],
    [0x02] = mac_address[0x02],
    [0x03] = mac_address[0x03],
    [0x04] = mac_address[0x04],
    [0x05] = mac_address[0x05],
    [0x06] = (uint8_t) (uuid & 0xff),
    [0x07] = (uint8_t) ((uuid >> 8) & 0xff),
    [0x08] = data[0x00],
    [0x09] = data[data_len-8],
    [0x0A] = data[data_len-7],
    [0x0B] = data[data_len-6],
    [0x0C] = data[data_len-5],
  };

  uint8_t tag[] = {
    [0x00] = data[data_len-4],
    [0x01] = data[data_len-3],
    [0x02] = data[data_len-2],
    [0x03] = data[data_len-1],
  };

  ret = mbedtls_ccm_auth_decrypt(&ctx, data_len-9, nonce, sizeof(nonce)/sizeof(nonce[0]), NULL, 0, data+1, cleartext, tag, sizeof(tag)/sizeof(tag[0]));
  if (ret != 0) {
    ESP_LOGE(TAG, "mbedtls_ccm_auth_decrypt() failed: %04x", ret);
    mbedtls_ccm_free(&ctx);
    return ret;
  }

  mbedtls_ccm_free(&ctx);
  return 0;
}

void BTHome::dump_config() {
  char bthome_mac[25];
  snprintf(bthome_mac, sizeof(bthome_mac),
           "BTHome %02X:%02X:%02X:%02X:%02X:%02X",
           uint8_t((this->address_ >> 40) & 0x00000000000000ff),
           uint8_t((this->address_ >> 32) & 0x00000000000000ff),
           uint8_t((this->address_ >> 24) & 0x00000000000000ff),
           uint8_t((this->address_ >> 16) & 0x00000000000000ff),
           uint8_t((this->address_ >> 8) & 0x00000000000000ff),
           uint8_t((this->address_) & 0x00000000000000ff));
  ESP_LOGCONFIG(TAG, bthome_mac);
  if (this->encrypted_) {
    ESP_LOGCONFIG(TAG, "  Encryption key set");
  } else {
    ESP_LOGCONFIG(TAG, "  Encryption key not set");
  }
  for (auto pub : this->publishers_) {
    pub->log("  ");
  }
}

bool BTHome::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (device.address_uint64() != this->address_) {
    ESP_LOGVV(TAG, "parse_device(): unknown MAC address.");
    return false;
  }
  ESP_LOGVV(TAG, "parse_device(): MAC address %s found.", device.address_str().c_str());

  bool success = false;
  for (auto &service_data : device.get_service_datas()) {
    const auto BTHomeServiceDataUUID = 0xfcd2;
    if (service_data.uuid != esp32_ble::ESPBTUUID::from_uint16(BTHomeServiceDataUUID)) {
      continue;
    }
    if (service_data.data.size() < 1) {
      ESP_LOGW(TAG, "BTHome service data (UUID %#04x) without data (size %zd)", BTHomeServiceDataUUID, service_data.data.size());
      continue;
    }
    device_information di(service_data.data[0]);
    if (di.bthome_version != 2) {
      ESP_LOGW(TAG, "BTHome version %d is not supported (only version 2 is supported)", di.bthome_version);
      continue;
    }
    const uint8_t *data;
    size_t data_len;
    uint8_t cleartext[service_data.data.size()-9];
    if (di.encryption) {
      if (!this->encrypted_) {
        ESP_LOGW(TAG, "Encrypted packet but encryption key is not set");
        continue;
      }
      uint32_t counter =
        service_data.data[service_data.data.size()-8]
        + ((uint32_t)(service_data.data[service_data.data.size()-7]) << 8)
        + ((uint32_t)(service_data.data[service_data.data.size()-6]) << 16)
        + ((uint32_t)(service_data.data[service_data.data.size()-5]) << 24);
      if (this->last_pid_ != -1 && counter <= this->last_counter_) {
        ESP_LOGV(TAG, "Packet counter already seen, skipping");
        continue;
      }
      this->last_counter_ = counter;
      const auto keybits = sizeof(this->encryption_key_) / sizeof(this->encryption_key_[0]) * 8;
      int ret = decrypt(service_data.data.data(), service_data.data.size(), device.address(), BTHomeServiceDataUUID, this->encryption_key_, keybits, cleartext);
      if (ret) {
        ESP_LOGE(TAG, "Could not decrypt packet, error code: %04x", ret);
        continue;
      }
      data = cleartext;
      data_len = sizeof(cleartext) / sizeof(cleartext[0]);
    } else {
      if (this->encrypted_) {
        ESP_LOGW(TAG, "Ignoring unencrypted packet when encryption key is set");
        continue;
      }
      data = service_data.data.data() + 1;
      data_len = service_data.data.size() - 1;
    }
    for (const uint8_t *p = data; p < data + data_len;) {
      auto oid = *p++;
      ESP_LOGVV(TAG, "OID %#02x", oid);

      if (oid == oid_pid.oid_) {
        UInt32Value v = oid_pid.read(p, data + data_len - p);
        if (v.value == this->last_pid_) {
          ESP_LOGV(TAG, "Packet ID %d already seen, skipping", v.value);
          break;
        }
        this->last_pid_ = v.value;
        p = v.next_ptr;
        continue;
      }

      const uint8_t *next_ptr = this->publish(oid, p, data + data_len - p);
      if (next_ptr != nullptr) {
        p = next_ptr;
        continue;
      }

      auto scan = oids[oid];
      if (scan == NULL) {
        ESP_LOGW(TAG, "Unknown OID %#02x - parsing aborted", oid);
        break;
      }
      ESP_LOGD(TAG, "Skipping OID %#02x", oid);
      ScanResult sr = scan(p, data + data_len - p);
      p = sr.next_ptr;
    }
    success = true;
  }

  return success;
}

const uint8_t *BTHome::publish(uint8_t oid, const uint8_t *data, size_t size) {
  for (auto pub : this->publishers_) {
    if (pub->oid() == oid) {
      return pub->publish(data, size);
    }
  }
  return nullptr;
}

void BTHome::set_publisher(Publisher *publisher) {
  for (int i = 0; i < this->publishers_.size(); i++) {
    if (this->publishers_[i]->oid() == publisher->oid()) {
      auto old = this->publishers_[i];
      this->publishers_[i] = publisher;
      delete old;
      return;
    }
  }
  this->publishers_.push_back(publisher);
}

void BTHome::set_encryption_key(const std::string &encryption_key) {
  memset(this->encryption_key_, 0, 16);
  if (encryption_key.size() != 32) {
    return;
  }
  char temp[3] = {0};
  for (int i = 0; i < 16; i++) {
    strncpy(temp, &(encryption_key.c_str()[i * 2]), 2);
    this->encryption_key_[i] = std::strtoul(temp, nullptr, 16);
  }
  this->encrypted_ = true;
}

void SensorPublisher::log(const char* prefix) const {
  char oid_str[5];
  snprintf(oid_str, sizeof(oid_str), "%#04x", this->oid_);
  LOG_SENSOR(prefix, oid_str, this->sensor_);
}

void BinarySensorPublisher::log(const char* prefix) const {
  char oid_str[5];
  snprintf(oid_str, sizeof(oid_str), "%#04x", this->oid_);
  LOG_BINARY_SENSOR(prefix, oid_str, this->sensor_);
}

}  // namespace bthome
}  // namespace esphome

#endif
