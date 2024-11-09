#include "bthome.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

#include "mbedtls/ccm.h"

namespace esphome {
namespace bthome {

#define OID_VAR(oid) oid_##oid
#define OID_ENTRY(oid) [oid] = OID_VAR(oid).scan

// TODO(badrpc): Switch to designated initializers if those become available
// in C++.
static scan_func_t *const oids[256] = {
    [0x00] = OIDUInt8::scan, OID_ENTRY(0x01),  OID_ENTRY(0x02),  OID_ENTRY(0x03),  OID_ENTRY(0x04),  OID_ENTRY(0x05),
    OID_ENTRY(0x06),         OID_ENTRY(0x07),  OID_ENTRY(0x08),  OID_ENTRY(0x09),  OID_ENTRY(0x0a),  OID_ENTRY(0x0b),
    OID_ENTRY(0x0c),         OID_ENTRY(0x0d),  OID_ENTRY(0x0e),  OID_ENTRY(0x0f),  OID_ENTRY(0x10),  OID_ENTRY(0x11),
    OID_ENTRY(0x12),         OID_ENTRY(0x13),  OID_ENTRY(0x14),  OID_ENTRY(0x15),  OID_ENTRY(0x16),  OID_ENTRY(0x17),
    OID_ENTRY(0x18),         OID_ENTRY(0x19),  OID_ENTRY(0x1a),  OID_ENTRY(0x1b),  OID_ENTRY(0x1c),  OID_ENTRY(0x1d),
    OID_ENTRY(0x1e),         OID_ENTRY(0x1f),  OID_ENTRY(0x20),  OID_ENTRY(0x21),  OID_ENTRY(0x22),  OID_ENTRY(0x23),
    OID_ENTRY(0x24),         OID_ENTRY(0x25),  OID_ENTRY(0x26),  OID_ENTRY(0x27),  OID_ENTRY(0x28),  OID_ENTRY(0x29),
    OID_ENTRY(0x2a),         OID_ENTRY(0x2b),  OID_ENTRY(0x2c),  OID_ENTRY(0x2d),  [0x2e] = nullptr, [0x2f] = nullptr,
    [0x30] = nullptr,        [0x31] = nullptr, [0x32] = nullptr, [0x33] = nullptr, [0x34] = nullptr, [0x35] = nullptr,
    [0x36] = nullptr,        [0x37] = nullptr, [0x38] = nullptr, [0x39] = nullptr, OID_ENTRY(0x3a),  [0x3b] = nullptr,
    OID_ENTRY(0x3c),         OID_ENTRY(0x3d),  OID_ENTRY(0x3e),  OID_ENTRY(0x3f),  OID_ENTRY(0x40),  OID_ENTRY(0x41),
    OID_ENTRY(0x42),         OID_ENTRY(0x43),  OID_ENTRY(0x44),  OID_ENTRY(0x45),  OID_ENTRY(0x46),  OID_ENTRY(0x47),
    OID_ENTRY(0x48),         OID_ENTRY(0x49),  OID_ENTRY(0x4a),  OID_ENTRY(0x4b),  OID_ENTRY(0x4c),  OID_ENTRY(0x4d),
    OID_ENTRY(0x4e),         OID_ENTRY(0x4f),  OID_ENTRY(0x50),  OID_ENTRY(0x51),  OID_ENTRY(0x52),  OID_ENTRY(0x53),
    OID_ENTRY(0x54),         OID_ENTRY(0x55),  [0x56] = nullptr, [0x57] = nullptr, [0x58] = nullptr, [0x59] = nullptr,
    [0x5a] = nullptr,        [0x5b] = nullptr, [0x5c] = nullptr, [0x5d] = nullptr, [0x5e] = nullptr, [0x5f] = nullptr,
    [0x60] = nullptr,        [0x61] = nullptr, [0x62] = nullptr, [0x63] = nullptr, [0x64] = nullptr, [0x65] = nullptr,
    [0x66] = nullptr,        [0x67] = nullptr, [0x68] = nullptr, [0x69] = nullptr, [0x6a] = nullptr, [0x6b] = nullptr,
    [0x6c] = nullptr,        [0x6d] = nullptr, [0x6e] = nullptr, [0x6f] = nullptr, [0x70] = nullptr, [0x71] = nullptr,
    [0x72] = nullptr,        [0x73] = nullptr, [0x74] = nullptr, [0x75] = nullptr, [0x76] = nullptr, [0x77] = nullptr,
    [0x78] = nullptr,        [0x79] = nullptr, [0x7a] = nullptr, [0x7b] = nullptr, [0x7c] = nullptr, [0x7d] = nullptr,
    [0x7e] = nullptr,        [0x7f] = nullptr, [0x80] = nullptr, [0x81] = nullptr, [0x82] = nullptr, [0x83] = nullptr,
    [0x84] = nullptr,        [0x85] = nullptr, [0x86] = nullptr, [0x87] = nullptr, [0x88] = nullptr, [0x89] = nullptr,
    [0x8a] = nullptr,        [0x8b] = nullptr, [0x8c] = nullptr, [0x8d] = nullptr, [0x8e] = nullptr, [0x8f] = nullptr,
    [0x90] = nullptr,        [0x91] = nullptr, [0x92] = nullptr, [0x93] = nullptr, [0x94] = nullptr, [0x95] = nullptr,
    [0x96] = nullptr,        [0x97] = nullptr, [0x98] = nullptr, [0x99] = nullptr, [0x9a] = nullptr, [0x9b] = nullptr,
    [0x9c] = nullptr,        [0x9d] = nullptr, [0x9e] = nullptr, [0x9f] = nullptr, [0xa0] = nullptr, [0xa1] = nullptr,
    [0xa2] = nullptr,        [0xa3] = nullptr, [0xa4] = nullptr, [0xa5] = nullptr, [0xa6] = nullptr, [0xa7] = nullptr,
    [0xa8] = nullptr,        [0xa9] = nullptr, [0xaa] = nullptr, [0xab] = nullptr, [0xac] = nullptr, [0xad] = nullptr,
    [0xae] = nullptr,        [0xaf] = nullptr, [0xb0] = nullptr, [0xb1] = nullptr, [0xb2] = nullptr, [0xb3] = nullptr,
    [0xb4] = nullptr,        [0xb5] = nullptr, [0xb6] = nullptr, [0xb7] = nullptr, [0xb8] = nullptr, [0xb9] = nullptr,
    [0xba] = nullptr,        [0xbb] = nullptr, [0xbc] = nullptr, [0xbd] = nullptr, [0xbe] = nullptr, [0xbf] = nullptr,
    [0xc0] = nullptr,        [0xc1] = nullptr, [0xc2] = nullptr, [0xc3] = nullptr, [0xc4] = nullptr, [0xc5] = nullptr,
    [0xc6] = nullptr,        [0xc7] = nullptr, [0xc8] = nullptr, [0xc9] = nullptr, [0xca] = nullptr, [0xcb] = nullptr,
    [0xcc] = nullptr,        [0xcd] = nullptr, [0xce] = nullptr, [0xcf] = nullptr, [0xd0] = nullptr, [0xd1] = nullptr,
    [0xd2] = nullptr,        [0xd3] = nullptr, [0xd4] = nullptr, [0xd5] = nullptr, [0xd6] = nullptr, [0xd7] = nullptr,
    [0xd8] = nullptr,        [0xd9] = nullptr, [0xda] = nullptr, [0xdb] = nullptr, [0xdc] = nullptr, [0xdd] = nullptr,
    [0xde] = nullptr,        [0xdf] = nullptr, [0xe0] = nullptr, [0xe1] = nullptr, [0xe2] = nullptr, [0xe3] = nullptr,
    [0xe4] = nullptr,        [0xe5] = nullptr, [0xe6] = nullptr, [0xe7] = nullptr, [0xe8] = nullptr, [0xe9] = nullptr,
    [0xea] = nullptr,        [0xeb] = nullptr, [0xec] = nullptr, [0xed] = nullptr, [0xee] = nullptr, [0xef] = nullptr,
    [0xf0] = nullptr,        OID_ENTRY(0xf1),  OID_ENTRY(0xf2),  [0xf3] = nullptr, [0xf4] = nullptr, [0xf5] = nullptr,
    [0xf6] = nullptr,        [0xf7] = nullptr, [0xf8] = nullptr, [0xf9] = nullptr, [0xfa] = nullptr, [0xfb] = nullptr,
    [0xfc] = nullptr,        [0xfd] = nullptr, [0xfe] = nullptr, [0xff] = nullptr,
};

uint32_t read_uint(size_t size, const uint8_t *data) {
  if (size < 1 || size > 4) {
    ESP_LOGE(TAG, "read_uint() called with invalid size %zu, must be in range [1, 4]", size);
    return 0;
  }
  uint32_t val = 0;
  int shift = 0;
  for (int i = 0; i < size; i++, shift += 8) {
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
  if (data[size - 1] > 127) {
    positive = false;
  }

  int32_t val = 0;
  int shift = (size - 1) * 8;
  for (int i = size - 1; i >= 0; i--, shift -= 8) {
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
struct DeviceInformation {
  bool encryption;
  bool trigger_based;
  uint8_t bthome_version;

  DeviceInformation(uint8_t v) {
    encryption = v & 0b000000001;
    trigger_based = (v & 0b00000100) >> 2;
    bthome_version = (v & 0b11100000) >> 5;
  }
};

//
// data is a full data received from device. Must be > 9 bytes log.
// mac_address has to be 6 bytes long.
// cleartext has to be at least data_len - 9 bytes long.
// returns 0 on success, anything else indicates an error.
int decrypt(const uint8_t *data, ssize_t data_len, const uint8_t *mac_address, uint16_t uuid, const uint8_t *key,
            ssize_t keybits, uint8_t *cleartext) {
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
  //
  // TODO(badrpc): Switch to designated initializers if those become available
  // in C++.
  uint8_t nonce[] = {
      mac_address[0x00],  mac_address[0x01],  mac_address[0x02],       mac_address[0x03],
      mac_address[0x04],  mac_address[0x05],  (uint8_t) (uuid & 0xff), (uint8_t) ((uuid >> 8) & 0xff),
      data[0x00],         data[data_len - 8], data[data_len - 7],      data[data_len - 6],
      data[data_len - 5],
  };

  // TODO(badrpc): Switch to designated initializers if those become available
  // in C++.
  uint8_t tag[] = {
      data[data_len - 4],
      data[data_len - 3],
      data[data_len - 2],
      data[data_len - 1],
  };

  ret = mbedtls_ccm_auth_decrypt(&ctx, data_len - 9, nonce, sizeof(nonce) / sizeof(nonce[0]), nullptr, 0, data + 1,
                                 cleartext, tag, sizeof(tag) / sizeof(tag[0]));
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
  snprintf(bthome_mac, sizeof(bthome_mac), "BTHome %02X:%02X:%02X:%02X:%02X:%02X",
           uint8_t((this->address_ >> 40) & 0x00000000000000ff), uint8_t((this->address_ >> 32) & 0x00000000000000ff),
           uint8_t((this->address_ >> 24) & 0x00000000000000ff), uint8_t((this->address_ >> 16) & 0x00000000000000ff),
           uint8_t((this->address_ >> 8) & 0x00000000000000ff), uint8_t((this->address_) & 0x00000000000000ff));
  ESP_LOGCONFIG(TAG, "%s", bthome_mac);
  if (this->encrypted_) {
    ESP_LOGCONFIG(TAG, "  Encryption key set");
  } else {
    ESP_LOGCONFIG(TAG, "  Encryption key not set");
  }
  for (auto *pub : this->publishers_) {
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
    constexpr auto bthome_service_data_uuid = 0xfcd2;
    if (service_data.uuid != esp32_ble::ESPBTUUID::from_uint16(bthome_service_data_uuid)) {
      continue;
    }
    if (service_data.data.empty()) {
      ESP_LOGW(TAG, "BTHome service data (UUID %#04x) without data (size %zd)", bthome_service_data_uuid,
               service_data.data.size());
      continue;
    }
    DeviceInformation di(service_data.data[0]);
    if (di.bthome_version != 2) {
      ESP_LOGW(TAG, "BTHome version %d is not supported (only version 2 is supported)", di.bthome_version);
      continue;
    }
    const uint8_t *data;
    size_t data_len;
    uint8_t cleartext[service_data.data.size() - 9];
    if (di.encryption) {
      if (!this->encrypted_) {
        ESP_LOGW(TAG, "Encrypted packet but encryption key is not set");
        continue;
      }
      uint32_t counter = service_data.data[service_data.data.size() - 8] +
                         ((uint32_t) (service_data.data[service_data.data.size() - 7]) << 8) +
                         ((uint32_t) (service_data.data[service_data.data.size() - 6]) << 16) +
                         ((uint32_t) (service_data.data[service_data.data.size() - 5]) << 24);
      if (this->last_pid_ != -1 && counter <= this->last_counter_) {
        ESP_LOGV(TAG, "Packet counter already seen, skipping");
        continue;
      }
      this->last_counter_ = counter;
      const auto keybits = sizeof(this->encryption_key_) / sizeof(this->encryption_key_[0]) * 8;
      int ret = decrypt(service_data.data.data(), service_data.data.size(), device.address(), bthome_service_data_uuid,
                        this->encryption_key_, keybits, cleartext);
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
      ESP_LOGVV(TAG, "Processing OID %#02x", oid);

      if (oid == oid_pid_id) {
        // NOLINTNEXTLINE(readability-static-accessed-through-instance)
        UInt32Value v = oid_pid.read(p, data + data_len - p);
        ESP_LOGVV(TAG, "Packet ID: %d", v.value);
        if (v.value == this->last_pid_) {
          ESP_LOGV(TAG, "Packet ID %d already seen, skipping", v.value);
          break;
        }
        this->last_pid_ = v.value;
        p = v.next_ptr;
        continue;
      }

      const uint8_t *next_ptr = this->publish_(oid, p, data + data_len - p);
      if (next_ptr != nullptr) {
        p = next_ptr;
        continue;
      }

      auto scan = oids[oid];
      if (scan == nullptr) {
        ESP_LOGW(TAG, "Unknown OID %#02x - parsing aborted", oid);
        break;
      }
      ESP_LOGD(TAG, "No sensors configured for OID %#02x", oid);
      ScanResult sr = scan(p, data + data_len - p);
      p = sr.next_ptr;
    }
    success = true;
  }

  return success;
}

const uint8_t *BTHome::publish_(uint8_t oid, const uint8_t *data, size_t size) {
  for (auto *pub : this->publishers_) {
    if (pub->oid() == oid) {
      return pub->publish(data, size);
    }
  }
  return nullptr;
}

void BTHome::set_publisher_(Publisher *publisher) {
  for (auto &p : this->publishers_) {
    if (p->oid() == publisher->oid()) {
      auto *old = p;
      p = publisher;
      delete old;  // NOLINT(cppcoreguidelines-owning-memory)
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

}  // namespace bthome
}  // namespace esphome

#endif
