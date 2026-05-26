#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace si1133 {

// ── Register-Adressen ────────────────────────────────────────────────────────
static const uint8_t REG_PART_ID    = 0x00;
static const uint8_t REG_HOSTIN0    = 0x0A;
static const uint8_t REG_COMMAND    = 0x0B;
static const uint8_t REG_IRQ_ENABLE = 0x0F;
static const uint8_t REG_RESPONSE1  = 0x10;
static const uint8_t REG_RESPONSE0  = 0x11;
static const uint8_t REG_IRQ_STATUS = 0x12;

// ── Parameter-RAM ────────────────────────────────────────────────────────────
static const uint8_t PARAM_CH_LIST    = 0x01;
static const uint8_t PARAM_ADCCONFIG0 = 0x02;
static const uint8_t PARAM_ADCSENS0   = 0x03;
static const uint8_t PARAM_ADCPOST0   = 0x04;
static const uint8_t PARAM_ADCCONFIG1 = 0x06;
static const uint8_t PARAM_ADCSENS1   = 0x07;
static const uint8_t PARAM_ADCPOST1   = 0x08;
static const uint8_t PARAM_ADCCONFIG2 = 0x0A;
static const uint8_t PARAM_ADCSENS2   = 0x0B;
static const uint8_t PARAM_ADCPOST2   = 0x0C;
static const uint8_t PARAM_ADCCONFIG3 = 0x0E;
static const uint8_t PARAM_ADCSENS3   = 0x0F;
static const uint8_t PARAM_ADCPOST3   = 0x10;

// ── Kommandos ────────────────────────────────────────────────────────────────
static const uint8_t CMD_RESET_CMD_CTR = 0x00;
static const uint8_t CMD_RESET         = 0x01;
static const uint8_t CMD_FORCE_CH      = 0x11;
static const uint8_t CMD_PAUSE_CH      = 0x12;
static const uint8_t CMD_START         = 0x13;

// ── Response0-Flags ──────────────────────────────────────────────────────────
static const uint8_t RSP0_CHIPSTAT_MASK = 0xE0;
static const uint8_t RSP0_COUNTER_MASK  = 0x1F;
static const uint8_t RSP0_SLEEP         = 0x20;

// ── Datenstrukturen ──────────────────────────────────────────────────────────
struct Coeff_t {
  int16_t  info;
  uint16_t mag;
};

struct LuxCoeff_t {
  Coeff_t coeff_high[4];
  Coeff_t coeff_low[9];
};

struct Samples_t {
  uint8_t irq_status;
  int32_t ch0, ch1, ch2, ch3;
};

// ── Komponentenklasse ─────────────────────────────────────────────────────────
class Si1133Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_uv_index_sensor(sensor::Sensor *s)      { uv_index_sensor_ = s; }
  void set_ambient_light_sensor(sensor::Sensor *s) { ambient_light_sensor_ = s; }

  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  sensor::Sensor *uv_index_sensor_{nullptr};
  sensor::Sensor *ambient_light_sensor_{nullptr};

  // I²C
  bool read_reg_(uint8_t reg, uint8_t *data);
  bool write_reg_(uint8_t reg, uint8_t data);
  bool read_block_(uint8_t reg, uint8_t len, uint8_t *data);
  bool write_block_(uint8_t reg, uint8_t len, const uint8_t *data);

  // Sensor-Steuerung
  bool wait_until_sleep_();
  bool send_cmd_(uint8_t cmd);
  bool set_parameter_(uint8_t addr, uint8_t value);
  bool reset_sensor_();
  bool init_sensor_();

  // Messung & Berechnung
  bool    read_samples_(Samples_t *s);
  int32_t calc_lux_(int32_t vis_high, int32_t vis_low, int32_t ir);
  int32_t calc_uv_(int32_t uv);
  int32_t polynomial_(int32_t x, int32_t y, uint8_t in_frac, uint8_t out_frac,
                      uint8_t n, const Coeff_t *kp);
  int32_t poly_helper_(int32_t input, int8_t fraction, uint16_t mag, int8_t shift);

  static const LuxCoeff_t lk_;
  static const Coeff_t    uk_[2];
};

}  // namespace si1133
}  // namespace esphome
