#pragma once

#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/core/defines.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#include "esphome/core/component.h"

#ifdef USE_ESP32

namespace esphome {
namespace bthome {

constexpr const char *const TAG = "bthome";

int decrypt(const uint8_t *data, ssize_t data_len, const uint8_t *mac_address, uint16_t uuid, const uint8_t *key,
            ssize_t keybits, uint8_t *cleartext);
uint32_t read_uint(size_t size, const uint8_t *data);
int32_t read_sint(size_t size, const uint8_t *data);

struct BoolValue {
  bool value;
  const uint8_t *next_ptr;
};

struct UInt32Value {
  uint32_t value;
  const uint8_t *next_ptr;
};

struct FloatValue {
  float value;
  const uint8_t *next_ptr;
};

struct StringValue {
  std::string value;
  const uint8_t *next_ptr;
};

struct ScanResult {
  const uint8_t *value_ptr;
  size_t value_size;
  const uint8_t *next_ptr;
};

typedef ScanResult scan_func_t(const uint8_t *data, size_t size);

template<size_t size_bytes> class OIDFixedSize {
 public:
  static ScanResult scan(const uint8_t *data, size_t size) {
    if (size < size_bytes) {
      return ScanResult {
          .value_ptr = nullptr,
          .value_size = 0,
          .next_ptr = data + size,
      };
    }
    return ScanResult{
        .value_ptr = data,
        .value_size = size_bytes,
        .next_ptr = data + size_bytes,
    };
  }
};

class OIDBool : public OIDFixedSize <1> {
 public:
  static BoolValue read(const uint8_t *data, size_t size) {
    ScanResult sr = OIDBool::scan(data, size);
    if (sr.value_ptr == nullptr || sr.value_size == 0) {
      return BoolValue{
          .value = false,
          .next_ptr = sr.next_ptr,
      };
    }
    return BoolValue {
        .value = read_uint(sr.value_size, sr.value_ptr) != 0,
        .next_ptr = sr.next_ptr,
    };
  }
};

template<size_t size_bytes> class OIDUInt : public OIDFixedSize <size_bytes> {
 public:
  static UInt32Value read(const uint8_t *data, size_t size) {
    ScanResult sr = OIDUInt::scan(data, size);
    if (sr.value_ptr == nullptr || sr.value_size == 0) {
      return UInt32Value {
          .value = 0,
          .next_ptr = sr.next_ptr,
      };
    }
    return UInt32Value {
        .value = read_uint(sr.value_size, sr.value_ptr),
        .next_ptr = sr.next_ptr,
    };
  }
};

template<size_t size_bytes> class OIDSFixedPoint : public OIDFixedSize <size_bytes> {
 public:
  constexpr OIDSFixedPoint(float factor = 1.0) : factor_(factor) {}

  FloatValue read(const uint8_t *data, size_t size) const {
    ScanResult sr = OIDSFixedPoint::scan(data, size);
    if (sr.value_ptr == nullptr || sr.value_size == 0) {
      return FloatValue {
          .value = 0.0,
          .next_ptr = sr.next_ptr,
      };
    }
    return FloatValue {
        .value = read_sint(sr.value_size, sr.value_ptr) * this->factor_,
        .next_ptr = sr.next_ptr,
    };
  }

 protected:
  float factor_;
};

template<size_t size_bytes> class OIDUFixedPoint : public OIDFixedSize<size_bytes> {
 public:
  constexpr OIDUFixedPoint(float factor = 1.0) : factor_(factor) {}

  FloatValue read(const uint8_t *data, size_t size) const {
    ScanResult sr = OIDUFixedPoint::scan(data, size);
    if (sr.value_ptr == nullptr || sr.value_size == 0) {
      return FloatValue {
          .value = 0.0,
          .next_ptr = sr.next_ptr,
      };
    }
    return FloatValue {
        .value = read_uint(sr.value_size, sr.value_ptr) * this->factor_,
        .next_ptr = sr.next_ptr,
    };
  }

 protected:
  float factor_;
};

using OIDUInt8 = OIDUInt<1>;
using OIDUInt16 = OIDUInt<2>;
using OIDUInt24 = OIDUInt<3>;
using OIDUInt32 = OIDUInt<4>;

class OIDVariableSize {
 public:
  static ScanResult scan(const uint8_t *data, size_t size) {
    if (size < 1) {
      return ScanResult {
          .value_ptr = nullptr,
          .value_size = 0,
          .next_ptr = data + size,
      };
    }
    size_t size_bytes = *data;
    size--;
    data++;
    if (size < size_bytes) {
      return ScanResult {
          .value_ptr = nullptr,
          .value_size = 0,
          .next_ptr = data + size,
      };
    }
    return ScanResult {
        .value_ptr = data,
        .value_size = size_bytes,
        .next_ptr = data + size_bytes,
    };
  }
};

class OIDBytes : public OIDVariableSize {
 public:
  StringValue read(const uint8_t *data, size_t size) const {
    ScanResult sr = OIDVariableSize::scan(data, size);
    if (sr.value_ptr == nullptr || sr.value_size == 0) {
      return StringValue {
          .next_ptr = sr.next_ptr,
      };
    }
    return StringValue {
        .value = std::string((const char*)sr.value_ptr, sr.value_size),
        .next_ptr = sr.next_ptr,
    };
  }
};

// TODO(badrpc): add inline when it's supported (c++ 17).
// 0x00    packet id   uint8 (1 byte)  0009    9
constexpr uint8_t oid_pid_id = 0x00;
constexpr OIDUInt8 oid_pid;
// 0x01    battery     uint8 (1 byte)  1   0161    97  %
constexpr OIDUFixedPoint<1> oid_0x01;
// 0x02    temperature     sint16 (2 bytes)    0.01    02CA09  25.06   °C
constexpr OIDSFixedPoint<2> oid_0x02(0.01);
// 0x03    humidity    uint16 (2 bytes)    0.01    03BF13  50.55   %
constexpr OIDUFixedPoint<2> oid_0x03(0.01);
// 0x04    pressure    uint24 (3 bytes)    0.01    04138A01    1008.83     hPa
constexpr OIDUFixedPoint<3> oid_0x04(0.01);
// 0x05    illuminance     uint24 (3 bytes)    0.01    05138A14    13460.67    lux
constexpr OIDUFixedPoint<3> oid_0x05(0.01);
// 0x06    mass (kg)   uint16 (2 byte)     0.01    065E1F  80.3    kg
constexpr OIDUFixedPoint<2> oid_0x06(0.01);
// 0x07    mass (lb)   uint16 (2 byte)     0.01    073E1D  74.86   lb
constexpr OIDUFixedPoint<2> oid_0x07(0.01);
// 0x08    dewpoint    sint16 (2 bytes)    0.01    08CA06  17.38   °C
constexpr OIDSFixedPoint<2> oid_0x08;
// 0x09    count   uint8 (1 bytes)  1   0960    96
constexpr OIDUFixedPoint<1> oid_0x09;
// 0x0A    energy  uint24 (3 bytes)    0.001   0A138A14    1346.067    kWh
constexpr OIDUFixedPoint<3> oid_0x0a(0.001);
// 0x0B    power   uint24 (3 bytes)    0.01    0B021B00    69.14   W
constexpr OIDUFixedPoint<3> oid_0x0b(0.01);
// 0x0C    voltage     uint16 (2 bytes)    0.001   0C020C  3.074   V
constexpr OIDUFixedPoint<2> oid_0x0c(0.001);
// 0x0D    pm2.5   uint16 (2 bytes)    1   0D120C  3090    ug/m3
constexpr OIDUFixedPoint<2> oid_0x0d;
// 0x0E    pm10    uint16 (2 bytes)    1   0E021C  7170    ug/m3
constexpr OIDUFixedPoint<2> oid_0x0e;
// 0x0F    generic boolean     uint8 (1 byte)  0F01    0 (False = Off) 1 (True = On)
constexpr OIDBool oid_0x0f;
// 0x10    power   uint8 (1 byte)  1001    0 (False = Off) 1 (True = On)
constexpr OIDBool oid_0x10;
// 0x11    opening     uint8 (1 byte)  1100    0 (False = Closed) 1 (True = Open)
constexpr OIDBool oid_0x11;
// 0x12    co2     uint16 (2 bytes)    1   12E204  1250    ppm
constexpr OIDUFixedPoint<2> oid_0x12;
// 0x13    tvoc    uint16 (2 bytes)    1   133301  307     ug/m3
constexpr OIDUFixedPoint<2> oid_0x13;
// 0x14    moisture    uint16 (2 bytes)    0.01    14020C  30.74   %
constexpr OIDUFixedPoint<2> oid_0x14(0.01);
// 0x15    battery     uint8 (1 byte)  1501    0 (False = Normal) 1 (True = Low)
constexpr OIDBool oid_0x15;
// 0x16    battery charging    uint8 (1 byte)  1601    0 (False = Not Charging) 1 (True = Charging)
constexpr OIDBool oid_0x16;
// 0x17    carbon monoxide     uint8 (1 byte)  1700    0 (False = Not detected) 1 (True = Detected)
constexpr OIDBool oid_0x17;
// 0x18    cold    uint8 (1 byte)  1801    0 (False = Normal) 1 (True = Cold)
constexpr OIDBool oid_0x18;
// 0x19    connectivity    uint8 (1 byte)  1900    0 (False = Disconnected) 1 (True = Connected)
constexpr OIDBool oid_0x19;
// 0x1A    door    uint8 (1 byte)  1A00    0 (False = Closed) 1 (True = Open)
constexpr OIDBool oid_0x1a;
// 0x1B    garage door     uint8 (1 byte)  1B01    0 (False = Closed) 1 (True = Open)
constexpr OIDBool oid_0x1b;
// 0x1C    gas     uint8 (1 byte)  1C01    0 (False = Clear) 1 (True = Detected)
constexpr OIDBool oid_0x1c;
// 0x1D    heat    uint8 (1 byte)  1D00    0 (False = Normal) 1 (True = Hot)
constexpr OIDBool oid_0x1d;
// 0x1E    light   uint8 (1 byte)  1E01    0 (False = No light) 1 (True = Light detected)
constexpr OIDBool oid_0x1e;
// 0x1F    lock    uint8 (1 byte)  1F01    0 (False = Locked) 1 (True = Unlocked)
constexpr OIDBool oid_0x1f;
// 0x20    moisture    uint8 (1 byte)  2001    0 (False = Dry) 1 (True = Wet)
constexpr OIDBool oid_0x20;
// 0x21    motion  uint8 (1 byte)  2100    0 (False = Clear) 1 (True = Detected)
constexpr OIDBool oid_0x21;
// 0x22    moving  uint8 (1 byte)  2201    0 (False = Not moving) 1 (True = Moving)
constexpr OIDBool oid_0x22;
// 0x23    occupancy   uint8 (1 byte)  2301    0 (False = Clear) 1 (True = Detected)
constexpr OIDBool oid_0x23;
// 0x24    plug    uint8 (1 byte)  2400    0 (False = Unplugged) 1 (True = Plugged in)
constexpr OIDBool oid_0x24;
// 0x25    presence    uint8 (1 byte)  2500    0 (False = Away) 1 (True = Home)
constexpr OIDBool oid_0x25;
// 0x26    problem     uint8 (1 byte)  2601    0 (False = OK) 1 (True = Problem)
constexpr OIDBool oid_0x26;
// 0x27    running     uint8 (1 byte)  2701    0 (False = Not Running) 1 (True = Running)
constexpr OIDBool oid_0x27;
// 0x28    safety  uint8 (1 byte)  2800    0 (False = Unsafe) 1 (True = Safe)
constexpr OIDBool oid_0x28;
// 0x29    smoke   uint8 (1 byte)  2901    0 (False = Clear) 1 (True = Detected)
constexpr OIDBool oid_0x29;
// 0x2A    sound   uint8 (1 byte)  2A00    0 (False = Clear) 1 (True = Detected)
constexpr OIDBool oid_0x2a;
// 0x2B    tamper  uint8 (1 byte)  2B00    0 (False = Off) 1 (True = On)
constexpr OIDBool oid_0x2b;
// 0x2C    vibration   uint8 (1 byte)  2C01    0 (False = Clear) 1 (True = Detected)
constexpr OIDBool oid_0x2c;
// 0x2D    window  uint8 (1 byte)  2D01    0 (False = Closed) 1 (True = Open)
constexpr OIDBool oid_0x2d;
// 0x2E    humidity    uint8 (1 byte)  1   2E23    35  %
constexpr OIDUFixedPoint<1> oid_0x2e;
// 0x2F    moisture    uint8 (1 byte)  1   2F23    35  %
constexpr OIDUFixedPoint<1> oid_0x2f;
// 0x30 - 0x39 - not defined.
// 0x3A    button
//   0x00 None (3A00)
//   0x01 press (3A01)
//   0x02 double_press (3A02)
//   0x03 triple_press (3A03)
//   0x04 long_press (3A04)
//   0x05 long_double_press (3A05)
//   0x06 long_triple_press (3A06)
//   0x80 hold_press (3A80)
// TODO(badrpc): This is not an appropriate type for this OID. Decide what to
// do about it and implement correct type.
constexpr OIDUFixedPoint<1> oid_0x3a;
// 0x3B - not defined.
// 0x3C    dimmer
//   0x00    None        3C0000
//   0x01    rotate left # steps   3C0103  rotate left 3 steps
//   0x02    rotate right # steps  3C020A  rotate right 10 steps
// TODO(badrpc): Figure out correct type for this OID and implement it.
constexpr OIDUInt16 oid_0x3c;
// 0x3D    count   uint16 (2 bytes)  1   3D0960  24585
constexpr OIDUFixedPoint<2> oid_0x3d;
// 0x3E    count   uint32 (4 bytes)  1   3E2A2C0960  1611213866
constexpr OIDUFixedPoint<4> oid_0x3e;
// 0x3F    rotation    sint16 (2 bytes)    0.1     3F020C  307.4   °
constexpr OIDSFixedPoint<2> oid_0x3f(0.1);
// 0x40    distance (mm)   uint16 (2 bytes)    1   400C00  12  mm
constexpr OIDUFixedPoint<2> oid_0x40;
// 0x41    distance (m)    uint16 (2 bytes)    0.1     414E00  7.8     m
constexpr OIDUFixedPoint<2> oid_0x41(0.1);
// 0x42    duration    uint24 (3 bytes)    0.001   424E3400    13.390  s
constexpr OIDUFixedPoint<3> oid_0x42(0.001);
// 0x43    current     uint16 (2 bytes)    0.001   434E34  13.39   A
constexpr OIDUFixedPoint<2> oid_0x43(0.001);
// 0x44    speed   uint16 (2 bytes)    0.01    444E34  133.90  m/s
constexpr OIDUFixedPoint<2> oid_0x44(0.01);
// 0x45    temperature     sint16 (2 bytes)    0.1     451101  27.3    °C
constexpr OIDSFixedPoint<2> oid_0x45(0.1);
// 0x46    UV index    uint8 (1 byte)  0.1     4632    5.0
constexpr OIDUFixedPoint<1> oid_0x46(0.1);
// 0x47    volume  uint16 (2 bytes)    0.1     478756  2215.1  L
constexpr OIDUFixedPoint<2> oid_0x47(0.1);
// 0x48    volume  uint16 (2 bytes)    1   48DC87  34780   mL
constexpr OIDUFixedPoint<2> oid_0x48;
// 0x49    volume Flow Rate    uint16 (2 bytes)    0.001   49DC87  34.780  m3/hr
constexpr OIDUFixedPoint<2> oid_0x49(0.001);
// 0x4A    voltage     uint16 (2 bytes)    0.1     4A020C  307.4   V
constexpr OIDUFixedPoint<2> oid_0x4a(0.1);
// 0x4B    gas     uint24 (3 bytes)    0.001   4B138A14    1346.067    m3
constexpr OIDUFixedPoint<3> oid_0x4b(0.001);
// 0x4C    gas     uint32 (4 bytes)    0.001   4C41018A01  25821.505   m3
constexpr OIDUFixedPoint<4> oid_0x4c(0.001);
// 0x4D    energy  uint32 (4 bytes)    0.001   4d12138a14  344593.170  kWh
constexpr OIDUFixedPoint<4> oid_0x4d(0.001);
// 0x4E    volume  uint32 (4 bytes)    0.001   4E87562A01  19551.879   L
constexpr OIDUFixedPoint<4> oid_0x4e(0.001);
// 0x4F    water   uint32 (4 bytes)    0.001   4F87562A01  19551.879
constexpr OIDUFixedPoint<4> oid_0x4f(0.001);
// 0x50    timestamp   uint48 (4 bytes)    -   505d396164  see below
constexpr OIDUFixedPoint<4> oid_0x50;
// 0x51    acceleration    uint16 (2 bytes)    0.001   518756  22.151  m/s²
constexpr OIDUFixedPoint<2> oid_0x51(0.001);
// 0x52    gyroscope   uint16 (2 bytes)    0.001   528756  22.151  °/s
constexpr OIDUFixedPoint<2> oid_0x52(0.001);
// 0x53    text    see below   -   530C48656C6C6F 20576F726C6421   Hello World!
constexpr OIDBytes oid_0x53;
// 0x54    raw     see below   -   540C48656C6C6F 20576F726C6421   48656c6c6f20 576f726c6421
constexpr OIDBytes oid_0x54;
// 0x55    volume storage  uint32 (4 bytes)    0.001   5587562A01  19551.879   L
constexpr OIDUFixedPoint<4> oid_0x55(0.001);
// 0x56 - 0xef - not defined
// 0xF0    device type id  uint16 (2 bytes)    F00100  1
// TODO(badrpc): It is not clear that this type can correctly represent device
// type ID.
constexpr OIDUFixedPoint<1> oid_0xf0;
// TODO(badrpc): Publishing both of the version OID below as a sensor (using
// float value) does not seem right. Figure out what to do about them and
// implement.
// 0xF1    firmware version    uint32 (4 bytes)    F100010204  4.2.1.0
constexpr OIDUInt32 oid_0xf1;
// 0xF2    firmware version    uint24 (3 bytes)    F2000106    6.1.0
constexpr OIDUInt24 oid_0xf2;

class Publisher {
 public:
  Publisher(uint8_t oid) : oid_(oid) {}
  virtual ~Publisher() {}
  uint8_t oid() const { return this->oid_; }
  virtual const uint8_t *publish(const uint8_t *data, size_t size) = 0;
  virtual void log(const char *prefix) const = 0;

 protected:
  uint8_t oid_;
};

#ifdef USE_SENSOR
template<typename OID> class SensorPublisher : public Publisher {
 public:
  SensorPublisher(uint8_t oid, OID oid_def, sensor::Sensor* sensor)
      : Publisher(oid), oid_def_(oid_def), sensor_(sensor) {}
  virtual ~SensorPublisher() {}

  virtual const uint8_t *publish(const uint8_t *data, size_t size) {
    FloatValue v = this->oid_def_.read(data, size);
    sensor_->publish_state(v.value);
    return v.next_ptr;
  }

  virtual void log(const char *prefix) const {
    char oid_str[5];
    snprintf(oid_str, sizeof(oid_str), "%#04x", this->oid_);
    LOG_SENSOR(prefix, oid_str, this->sensor_);
  }

 protected:
  OID oid_def_;
  sensor::Sensor *sensor_;
};
#endif

#ifdef USE_BINARY_SENSOR
class BinarySensorPublisher : public Publisher {
 public:
  BinarySensorPublisher(uint8_t oid, OIDBool oid_def, binary_sensor::BinarySensor* sensor)
      : Publisher(oid), oid_def_(oid_def), sensor_(sensor) {}
  virtual ~BinarySensorPublisher() {}

  virtual const uint8_t *publish(const uint8_t *data, size_t size) {
    BoolValue v = this->oid_def_.read(data, size);
    sensor_->publish_state(v.value);
    return v.next_ptr;
  }

  virtual void log(const char *prefix) const {
    char oid_str[5];
    snprintf(oid_str, sizeof(oid_str), "%#04x", this->oid_);
    LOG_BINARY_SENSOR(prefix, oid_str, this->sensor_);
  }

 protected:
  OIDBool oid_def_;
  binary_sensor::BinarySensor *sensor_;
};
#endif

#ifdef USE_TEXT_SENSOR
class TextSensorPublisher : public Publisher {
 public:
  TextSensorPublisher(uint8_t oid, OIDBytes oid_def, text_sensor::TextSensor* sensor)
      : Publisher(oid), oid_def_(oid_def), sensor_(sensor) {}
  virtual ~TextSensorPublisher() {}

  virtual const uint8_t *publish(const uint8_t *data, size_t size) {
    StringValue v = this->oid_def_.read(data, size);
    sensor_->publish_state(v.value);
    return v.next_ptr;
  }

  virtual void log(const char *prefix) const {
    char oid_str[5];
    snprintf(oid_str, sizeof(oid_str), "%#04x", this->oid_);
    LOG_BINARY_SENSOR(prefix, oid_str, this->sensor_);
  }

 protected:
  OIDBytes oid_def_;
  text_sensor::TextSensor *sensor_;
};
#endif

class BTHome : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  void set_address(uint64_t address) { this->address_ = address; };
  void set_encryption_key(const std::string &encryption_key);

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

#ifdef USE_BINARY_SENSOR
  template<typename T> void register_binary_sensor(uint8_t oid, T oid_def, binary_sensor::BinarySensor *binary_sensor) {
    this->set_publisher(new BinarySensorPublisher(oid, oid_def, binary_sensor));
  }
#endif

#ifdef USE_SENSOR
  template<typename T> void register_sensor(uint8_t oid, T oid_def, sensor::Sensor *sensor) {
    this->set_publisher(new SensorPublisher<T>(oid, oid_def, sensor));
  }
#endif

#ifdef USE_TEXT_SENSOR
  template<typename T> void register_text_sensor(uint8_t oid, T oid_def, text_sensor::TextSensor *text_sensor) {
    this->set_publisher(new TextSensorPublisher(oid, oid_def, text_sensor));
  }
#endif

 protected:
  const uint8_t *publish(uint8_t oid, const uint8_t *data, size_t size);
  void set_publisher(Publisher *publisher);

  uint64_t address_;
  bool encrypted_{false};
  uint8_t encryption_key_[16];

  int16_t last_pid_{-1};  // -1 indicates that no PID has been observed yet.
  uint32_t last_counter_{0};

  std::vector <Publisher*> publishers_;
};

}  // namespace bthome
}  // namespace esphome

#endif
