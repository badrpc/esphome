#pragma once

#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/core/defines.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#include "esphome/core/component.h"

#ifdef USE_ESP32

namespace esphome {
namespace bthome {

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

class Publisher {
 public:
  Publisher(uint8_t oid): oid_(oid) {}
  virtual ~Publisher() {}
  uint8_t oid() const { return this->oid_; }
  virtual const uint8_t *publish(const uint8_t *data, size_t size) = 0;
  virtual void log(const char* prefix) const = 0;

 protected:
  uint8_t oid_;
};

class SensorPublisher: public Publisher {
 public:
  SensorPublisher(uint8_t oid, FloatValue (*read)(const uint8_t *data, size_t size), sensor::Sensor* sensor): Publisher(oid), sensor_(sensor), read_(read) {}
  virtual ~SensorPublisher() {}

  virtual const uint8_t *publish(const uint8_t *data, size_t size) {
    FloatValue v = this->read_(data, size);
    sensor_->publish_state(v.value);
    return v.next_ptr;
  }

  virtual void log(const char* prefix) const;

 protected:
  sensor::Sensor *sensor_;
  FloatValue (*read_)(const uint8_t *data, size_t size);
};

class BinarySensorPublisher: public Publisher {
 public:
  BinarySensorPublisher(uint8_t oid, BoolValue (*read)(const uint8_t *data, size_t size), binary_sensor::BinarySensor* sensor): Publisher(oid), sensor_(sensor), read_(read) {}
  virtual ~BinarySensorPublisher() {}

  virtual const uint8_t *publish(const uint8_t *data, size_t size) {
    BoolValue v = this->read_(data, size);
    sensor_->publish_state(v.value);
    return v.next_ptr;
  }

  virtual void log(const char* prefix) const;

 protected:
  binary_sensor::BinarySensor *sensor_;
  BoolValue (*read_)(const uint8_t *data, size_t size);
};

struct ScanResult {
  const uint8_t *value_ptr;
  size_t value_size;
  const uint8_t *next_ptr;
};

typedef ScanResult scan_func_t(const uint8_t *data, size_t size);

template <uint8_t oid, size_t size_bytes>
class OIDFixedSize {
 public:
  static ScanResult scan(const uint8_t *data, size_t size) {
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

  static constexpr uint8_t oid_ = {oid};
  static constexpr size_t size_bytes_ = {size_bytes};
};

template <uint8_t oid>
class OIDBool: public OIDFixedSize <oid, 1> {
 public:
  static BoolValue read(const uint8_t *data, size_t size) {
    ScanResult sr = OIDBool::scan(data, size);
    if (sr.value_ptr == nullptr || sr.value_size == 0) {
      return BoolValue {
        .value = false,
        .next_ptr = sr.next_ptr,
      };
    }
    return BoolValue {
      .value = read_uint(sr.value_size, sr.value_ptr) != 0,
      .next_ptr = sr.next_ptr,
    };
  }

  BinarySensorPublisher *new_publisher(binary_sensor::BinarySensor *sensor) const {
    return new BinarySensorPublisher(oid, OIDBool::read, sensor);
  }
};

template <uint8_t oid, size_t size_bytes>
class OIDUInt: public OIDFixedSize <oid, size_bytes> {
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

template <uint8_t oid, size_t size_bytes, int f_num=1, int f_denom=1>
class OIDSFixedPoint: public OIDFixedSize <oid, size_bytes> {
 public:
  static FloatValue read(const uint8_t *data, size_t size) {
    ScanResult sr = OIDSFixedPoint::scan(data, size);
    if (sr.value_ptr == nullptr || sr.value_size == 0) {
      return FloatValue {
        .value = 0.0,
        .next_ptr = sr.next_ptr,
      };
    }
    return FloatValue {
      .value = read_sint(sr.value_size, sr.value_ptr) * f_num / f_denom,
      .next_ptr = sr.next_ptr,
    };
  }

  static constexpr int f_num_ = {f_num};
  static constexpr int f_denom_ = {f_denom};
};

template <uint8_t oid, size_t size_bytes, int f_num=1, int f_denom=1>
class OIDUFixedPoint: public OIDFixedSize <oid, size_bytes> {
 public:
  static FloatValue read(const uint8_t *data, size_t size) {
    ScanResult sr = OIDUFixedPoint::scan(data, size);
    if (sr.value_ptr == nullptr || sr.value_size == 0) {
      return FloatValue {
        .value = 0.0,
        .next_ptr = sr.next_ptr,
      };
    }
    return FloatValue {
      .value = static_cast<float>(read_uint(sr.value_size, sr.value_ptr)) * f_num / f_denom,
      .next_ptr = sr.next_ptr,
    };
  }

  SensorPublisher *new_publisher(sensor::Sensor *sensor) const {
    return new SensorPublisher(oid, OIDUFixedPoint::read, sensor);
  }

  static constexpr int f_num_ = {f_num};
  static constexpr int f_denom_ = {f_denom};
};

template <uint8_t oid>
class OIDUInt8: public OIDUInt <oid, 1> {
};

// 0x00    packet id   uint8 (1 byte)  0009    9
// 0x01    battery     uint8 (1 byte)  1   0161    97  %
// 0x02    temperature     sint16 (2 bytes)    0.01    02CA09  25.06   °C
// 0x03    humidity    uint16 (2 bytes)    0.01    03BF13  50.55   %
// 0x04    pressure    uint24 (3 bytes)    0.01    04138A01    1008.83     hPa
// 0x05    illuminance     uint24 (3 bytes)    0.01    05138A14    13460.67    lux
// 0x06    mass (kg)   uint16 (2 byte)     0.01    065E1F  80.3    kg
// 0x07    mass (lb)   uint16 (2 byte)     0.01    073E1D  74.86   lb
// 0x08    dewpoint    sint16 (2 bytes)    0.01    08CA06  17.38   °C
// 0x09    count   uint (1 bytes)  1   0960    96
// 0x0A    energy  uint24 (3 bytes)    0.001   0A138A14    1346.067    kWh
// 0x0B    power   uint24 (3 bytes)    0.01    0B021B00    69.14   W
// 0x0C    voltage     uint16 (2 bytes)    0.001   0C020C  3.074   V
// 0x0D    pm2.5   uint16 (2 bytes)    1   0D120C  3090    ug/m3
// 0x0E    pm10    uint16 (2 bytes)    1   0E021C  7170    ug/m3
// 0x0F    generic boolean     uint8 (1 byte)  0F01    0 (False = Off) 1 (True = On)
// 0x10    power   uint8 (1 byte)  1001    0 (False = Off) 1 (True = On)
// 0x11    opening     uint8 (1 byte)  1100    0 (False = Closed) 1 (True = Open)
// 0x12    co2     uint16 (2 bytes)    1   12E204  1250    ppm
// 0x13    tvoc    uint16 (2 bytes)    1   133301  307     ug/m3
// 0x14    moisture    uint16 (2 bytes)    0.01    14020C  30.74   %
// 0x15    battery     uint8 (1 byte)  1501    0 (False = Normal) 1 (True = Low)
// 0x16    battery charging    uint8 (1 byte)  1601    0 (False = Not Charging) 1 (True = Charging)
// 0x17    carbon monoxide     uint8 (1 byte)  1700    0 (False = Not detected) 1 (True = Detected)
// 0x18    cold    uint8 (1 byte)  1801    0 (False = Normal) 1 (True = Cold)
// 0x19    connectivity    uint8 (1 byte)  1900    0 (False = Disconnected) 1 (True = Connected)
// 0x1A    door    uint8 (1 byte)  1A00    0 (False = Closed) 1 (True = Open)
// 0x1B    garage door     uint8 (1 byte)  1B01    0 (False = Closed) 1 (True = Open)
// 0x1C    gas     uint8 (1 byte)  1C01    0 (False = Clear) 1 (True = Detected)
// 0x1D    heat    uint8 (1 byte)  1D00    0 (False = Normal) 1 (True = Hot)
// 0x1E    light   uint8 (1 byte)  1E01    0 (False = No light) 1 (True = Light detected)
// 0x1F    lock    uint8 (1 byte)  1F01    0 (False = Locked) 1 (True = Unlocked)
// 0x20    moisture    uint8 (1 byte)  2001    0 (False = Dry) 1 (True = Wet)
// 0x21    motion  uint8 (1 byte)  2100    0 (False = Clear) 1 (True = Detected)
// 0x22    moving  uint8 (1 byte)  2201    0 (False = Not moving) 1 (True = Moving)
// 0x23    occupancy   uint8 (1 byte)  2301    0 (False = Clear) 1 (True = Detected)
// 0x24    plug    uint8 (1 byte)  2400    0 (False = Unplugged) 1 (True = Plugged in)
// 0x25    presence    uint8 (1 byte)  2500    0 (False = Away) 1 (True = Home)
// 0x26    problem     uint8 (1 byte)  2601    0 (False = OK) 1 (True = Problem)
// 0x27    running     uint8 (1 byte)  2701    0 (False = Not Running) 1 (True = Running)
// 0x28    safety  uint8 (1 byte)  2800    0 (False = Unsafe) 1 (True = Safe)
// 0x29    smoke   uint8 (1 byte)  2901    0 (False = Clear) 1 (True = Detected)
// 0x2A    sound   uint8 (1 byte)  2A00    0 (False = Clear) 1 (True = Detected)
// 0x2B    tamper  uint8 (1 byte)  2B00    0 (False = Off) 1 (True = On)
// 0x2C    vibration   uint8 (1 byte)  2C01    0 (False = Clear) 1 (True = Detected)
// 0x2D    window  uint8 (1 byte)  2D01    0 (False = Closed) 1 (True = Open)
// 0x2E    humidity    uint8 (1 byte)  1   2E23    35  %
// 0x2F    moisture    uint8 (1 byte)  1   2F23    35  %
// 0x3A    button  0x00    None        3A00    0x01    press       3A01    press 0x02    double_press        3A02    double_press 0x03    triple_press        3A03    triple_press 0x04    long_press      3A04    long_press 0x05    long_double_press       3A05    long_double_press 0x06    long_triple_press       3A06    long_triple_press 0x80    hold_press      3A80    hold_press
// 0x3C    dimmer  0x00    None        3C0000  0x01    rotate left     # steps     3C0103  rotate left 3 steps 0x02    rotate right    # steps     3C020A  rotate right 10 steps
// 0x3D    count   uint (2 bytes)  1   3D0960  24585
// 0x3E    count   uint (4 bytes)  1   3E2A2C0960  1611213866
// 0x3F    rotation    sint16 (2 bytes)    0.1     3F020C  307.4   °
// 0x40    distance (mm)   uint16 (2 bytes)    1   400C00  12  mm
// 0x41    distance (m)    uint16 (2 bytes)    0.1     414E00  7.8     m
// 0x42    duration    uint24 (3 bytes)    0.001   424E3400    13.390  s
// 0x43    current     uint16 (2 bytes)    0.001   434E34  13.39   A
// 0x44    speed   uint16 (2 bytes)    0.01    444E34  133.90  m/s
// 0x45    temperature     sint16 (2 bytes)    0.1     451101  27.3    °C
// 0x46    UV index    uint8 (1 byte)  0.1     4632    5.0
// 0x47    volume  uint16 (2 bytes)    0.1     478756  2215.1  L
// 0x48    volume  uint16 (2 bytes)    1   48DC87  34780   mL
// 0x49    volume Flow Rate    uint16 (2 bytes)    0.001   49DC87  34.780  m3/hr
// 0x4A    voltage     uint16 (2 bytes)    0.1     4A020C  307.4   V
// 0x4B    gas     uint24 (3 bytes)    0.001   4B138A14    1346.067    m3
// 0x4C    gas     uint32 (4 bytes)    0.001   4C41018A01  25821.505   m3
// 0x4D    energy  uint32 (4 bytes)    0.001   4d12138a14  344593.170  kWh
// 0x4E    volume  uint32 (4 bytes)    0.001   4E87562A01  19551.879   L
// 0x4F    water   uint32 (4 bytes)    0.001   4F87562A01  19551.879
// 0x50    timestamp   uint48 (4 bytes)    -   505d396164  see below
// 0x51    acceleration    uint16 (2 bytes)    0.001   518756  22.151  m/s²
// 0x52    gyroscope   uint16 (2 bytes)    0.001   528756  22.151  °/s
// 0x53    text    see below   -   530C48656C6C6F 20576F726C6421   Hello World!
// 0x54    raw     see below   -   540C48656C6C6F 20576F726C6421   48656c6c6f20 576f726c6421
// 0x55    volume storage  uint32 (4 bytes)    0.001   5587562A01  19551.879   L
// 0xF0    device type id  uint16 (2 bytes)    F00100  1
// 0xF1    firmware version    uint32 (4 bytes)    F100010204  4.2.1.0
// 0xF2    firmware version    uint24 (3 bytes)    F2000106    6.1.0

#define OID_DECL(oid, type, size) type<oid, size> oid_##oid

constexpr OID_DECL(0x01, OIDUFixedPoint, 1);
constexpr OIDUFixedPoint<0x05, 3, 1, 100> oid_0x05;
constexpr OIDBool<0x2d> oid_0x2d;
constexpr OIDUFixedPoint<0x3f, 2, 1, 10> oid_0x3f;

constexpr OIDUInt8<0x00> oid_pid;
constexpr OIDUFixedPoint<0x01, 1> oid_battery_percent;
constexpr OIDSFixedPoint<0x02, 2, 1, 100> oid_temperature_celsius_x100;
constexpr OIDUFixedPoint<0x03, 2, 1, 100> oid_humidity_percent_x100;
constexpr OIDUFixedPoint<0x04, 3, 1, 100> oid_pressure_hpa_x100;
constexpr OIDUFixedPoint<0x05, 3, 1, 100> oid_illuminance_lux_x100;
constexpr OIDUFixedPoint<0x06, 2, 1, 100> oid_mass_kg_x100;
constexpr OIDUFixedPoint<0x07, 2, 1, 100> oid_mass_lb_x100;
constexpr OIDSFixedPoint<0x08, 2> oid_dewpoint_celsius_x100;
constexpr OIDUFixedPoint<0x09, 1> oid_counter8;
constexpr OIDUFixedPoint<0x0a, 3, 1, 1000> oid_energy_kwh_x1000;
constexpr OIDUFixedPoint<0x0b, 3, 1, 100> oid_power_w_x100;
constexpr OIDUFixedPoint<0x0c, 2, 1, 1000> oid_voltage_v_x1000;
constexpr OIDUFixedPoint<0x0d, 2> oid_pm2_5_ug_m3;
constexpr OIDUFixedPoint<0x0e, 2> oid_pm10_ug_m3;
constexpr OIDBool<0x0f> oid_bool;
constexpr OIDBool<0x10> oid_power;
constexpr OIDBool<0x11> oid_opening;
constexpr OIDUFixedPoint<0x12, 2> oid_co2_concentration_ppm;
constexpr OIDUFixedPoint<0x13, 2> oid_tvoc_ug_m3;
constexpr OIDUFixedPoint<0x14, 2, 1, 100> oid_moisture_percent_x100;
constexpr OIDBool<0x15> oid_battery;
constexpr OIDBool<0x16> oid_battery_charging;
constexpr OIDBool<0x17> oid_carbon_monoxide;
constexpr OIDBool<0x18> oid_cold;
constexpr OIDBool<0x19> oid_connectivity;
constexpr OIDBool<0x1a> oid_door;
constexpr OIDBool<0x1b> oid_garage_door;
constexpr OIDBool<0x1c> oid_gas;
constexpr OIDBool<0x1d> oid_heat;
constexpr OIDBool<0x1e> oid_light;
constexpr OIDBool<0x1f> oid_lock;
constexpr OIDBool<0x20> oid_moisture;
constexpr OIDBool<0x21> oid_motion;
constexpr OIDBool<0x22> oid_moving;
constexpr OIDBool<0x23> oid_occupancy;
constexpr OIDBool<0x24> oid_plug;
constexpr OIDBool<0x25> oid_presence;
constexpr OIDBool<0x26> oid_problem;
constexpr OIDBool<0x27> oid_running;
constexpr OIDBool<0x28> oid_safety;
constexpr OIDBool<0x29> oid_smoke;
constexpr OIDBool<0x2a> oid_sound;
constexpr OIDBool<0x2b> oid_tamper;
constexpr OIDBool<0x2c> oid_vibration;
constexpr OIDBool<0x2d> oid_window;
constexpr OIDUFixedPoint<0x2e, 1> oid_humidity_percent;
constexpr OIDUFixedPoint<0x2f, 1> oid_moisture_percent;
// 0x30 - 0x39
constexpr OIDUFixedPoint<0x3a, 1> oid_button_event;
// 0x3b
// 0x3C    dimmer  0x00    None        3C0000  0x01    rotate left     # steps     3C0103  rotate left 3 steps 0x02    rotate right    # steps     3C020A  rotate right 10 steps
constexpr OIDUFixedPoint<0x3d, 2> oid_counter16;
constexpr OIDUFixedPoint<0x3e, 4> oid_counter32;
constexpr OIDUFixedPoint<0x3f, 2, 1, 10> oid_angle_degrees_x10;
// 0x40    distance (mm)   uint16 (2 bytes)    1   400C00  12  mm
// 0x41    distance (m)    uint16 (2 bytes)    0.1     414E00  7.8     m
// 0x42    duration    uint24 (3 bytes)    0.001   424E3400    13.390  s
// 0x43    current     uint16 (2 bytes)    0.001   434E34  13.39   A
// 0x44    speed   uint16 (2 bytes)    0.01    444E34  133.90  m/s
// 0x45    temperature     sint16 (2 bytes)    0.1     451101  27.3    °C
// 0x46    UV index    uint8 (1 byte)  0.1     4632    5.0
// 0x47    volume  uint16 (2 bytes)    0.1     478756  2215.1  L
// 0x48    volume  uint16 (2 bytes)    1   48DC87  34780   mL
// 0x49    volume Flow Rate    uint16 (2 bytes)    0.001   49DC87  34.780  m3/hr
// 0x4A    voltage     uint16 (2 bytes)    0.1     4A020C  307.4   V
// 0x4B    gas     uint24 (3 bytes)    0.001   4B138A14    1346.067    m3
// 0x4C    gas     uint32 (4 bytes)    0.001   4C41018A01  25821.505   m3
// 0x4D    energy  uint32 (4 bytes)    0.001   4d12138a14  344593.170  kWh
// 0x4E    volume  uint32 (4 bytes)    0.001   4E87562A01  19551.879   L
// 0x4F    water   uint32 (4 bytes)    0.001   4F87562A01  19551.879
// 0x50    timestamp   uint48 (4 bytes)    -   505d396164  see below
// 0x51    acceleration    uint16 (2 bytes)    0.001   518756  22.151  m/s²
// 0x52    gyroscope   uint16 (2 bytes)    0.001   528756  22.151  °/s
// 0x53    text    see below   -   530C48656C6C6F 20576F726C6421   Hello World!
// 0x54    raw     see below   -   540C48656C6C6F 20576F726C6421   48656c6c6f20 576f726c6421
// 0x55    volume storage  uint32 (4 bytes)    0.001   5587562A01  19551.879   L
// 0x56 - 0xef
// 0xF0    device type id  uint16 (2 bytes)    F00100  1
// 0xF1    firmware version    uint32 (4 bytes)    F100010204  4.2.1.0
// 0xF2    firmware version    uint24 (3 bytes)    F2000106    6.1.0

class BTHome : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  void set_address(uint64_t address) { this->address_ = address; };
  void set_encryption_key(const std::string &encryption_key);

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

#ifdef USE_BINARY_SENSOR
  template <typename t>
  void register_binary_sensor(t oid, binary_sensor::BinarySensor *binary_sensor) {
    this->set_publisher(oid.new_publisher(binary_sensor));
  }
#endif

#ifdef USE_SENSOR
  template <typename t>
  void register_sensor(t oid, sensor::Sensor *sensor) {
    this->set_publisher(oid.new_publisher(sensor));
  }
#endif

 protected:
  const uint8_t *publish(uint8_t oid, const uint8_t *data, size_t size);
  void set_publisher(Publisher *publisher);

  uint64_t address_;
  bool encrypted_{false};
  uint8_t encryption_key_[16];

  int16_t last_pid_{-1}; // -1 indicates that no packets have been received yet.
  uint32_t last_counter_{0};

  std::vector <Publisher*> publishers_;
};

}  // namespace bthome
}  // namespace esphome

#endif
