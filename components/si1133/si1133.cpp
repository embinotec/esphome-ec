/*******************************************************************************
 * Si1133 ESPHome External Component
 * Portiert vom Silicon Labs Mbed-Treiber (Apache-2.0)
 * https://os.mbed.com/teams/SiliconLabs/code/Si1133/
 ******************************************************************************/

#include "si1133.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace si1133 {

static const char *const TAG = "si1133";

// ── Timing ───────────────────────────────────────────────────────────────────
static const uint8_t  CMD_RETRY        = 5;
static const uint32_t MEASURE_WAIT_MS  = 500;

// ── Polynomial-Konstanten ────────────────────────────────────────────────────
#define X_ORDER_MASK  0x70
#define Y_ORDER_MASK  0x07
#define SIGN_MASK     0x80
#define GET_X_ORDER(m) (((m) & X_ORDER_MASK) >> 4)
#define GET_Y_ORDER(m) ( (m) & Y_ORDER_MASK)
#define GET_SIGN(m)    (((m) & SIGN_MASK) >> 7)

static const uint8_t  UV_INPUT_FRACTION  = 15;
static const uint8_t  UV_OUTPUT_FRACTION = 12;
static const uint8_t  UV_NUMCOEFF        = 2;
static const int32_t  ADC_THRESHOLD      = 16000;
static const uint8_t  INPUT_FRACTION_HIGH = 7;
static const uint8_t  INPUT_FRACTION_LOW  = 15;
static const uint8_t  LUX_OUTPUT_FRACTION = 12;
static const uint8_t  NUMCOEFF_LOW        = 9;
static const uint8_t  NUMCOEFF_HIGH       = 4;

// ── Kalibrierungstabellen ────────────────────────────────────────────────────
const LuxCoeff_t Si1133Component::lk_ = {
  { {0,209}, {1665,93}, {2064,65}, {-2671,234} },
  { {0,0}, {1921,29053}, {-1022,36363}, {2320,20789},
    {-367,57909}, {-1774,38240}, {-608,46775}, {-1503,51831}, {-1886,58928} }
};

const Coeff_t Si1133Component::uk_[2] = {
  {1281, 30902},
  {-638, 46301}
};

// ── I²C-Hilfsfunktionen ──────────────────────────────────────────────────────
bool Si1133Component::read_reg_(uint8_t reg, uint8_t *data) {
  return this->read_register(reg, data, 1) == i2c::ERROR_OK;
}

bool Si1133Component::write_reg_(uint8_t reg, uint8_t data) {
  return this->write_register(reg, &data, 1) == i2c::ERROR_OK;
}

bool Si1133Component::read_block_(uint8_t reg, uint8_t len, uint8_t *data) {
  return this->read_register(reg, data, len) == i2c::ERROR_OK;
}

bool Si1133Component::write_block_(uint8_t reg, uint8_t len, const uint8_t *data) {
  return this->write_register(reg, data, len) == i2c::ERROR_OK;
}

// ── Interne Steuerfunktionen ─────────────────────────────────────────────────
bool Si1133Component::wait_until_sleep_() {
  for (uint8_t i = 0; i < CMD_RETRY; i++) {
    uint8_t resp = 0;
    if (!read_reg_(REG_RESPONSE0, &resp)) return false;
    if ((resp & RSP0_CHIPSTAT_MASK) == RSP0_SLEEP) return true;
    delay(2);
  }
  return false;
}

bool Si1133Component::send_cmd_(uint8_t cmd) {
  uint8_t response, stored;
  if (!read_reg_(REG_RESPONSE0, &stored)) return false;
  stored &= RSP0_COUNTER_MASK;

  for (uint8_t i = 0; i < CMD_RETRY; i++) {
    if (!wait_until_sleep_()) return false;
    if (cmd == CMD_RESET_CMD_CTR) break;
    if (!read_reg_(REG_RESPONSE0, &response)) return false;
    if ((response & RSP0_COUNTER_MASK) == stored) break;
    stored = response & RSP0_COUNTER_MASK;
  }

  if (!write_reg_(REG_COMMAND, cmd)) return false;

  for (uint8_t i = 0; i < CMD_RETRY; i++) {
    if (cmd == CMD_RESET_CMD_CTR) break;
    if (!read_reg_(REG_RESPONSE0, &response)) return false;
    if ((response & RSP0_COUNTER_MASK) != stored) break;
    delay(2);
  }
  return true;
}

bool Si1133Component::set_parameter_(uint8_t addr, uint8_t value) {
  if (!wait_until_sleep_()) return false;

  uint8_t stored;
  if (!read_reg_(REG_RESPONSE0, &stored)) return false;
  stored &= RSP0_COUNTER_MASK;

  uint8_t buf[2] = {value, static_cast<uint8_t>(0x80 | (addr & 0x3F))};
  if (!write_block_(REG_HOSTIN0, 2, buf)) return false;

  for (uint8_t i = 0; i < CMD_RETRY; i++) {
    uint8_t resp;
    if (!read_reg_(REG_RESPONSE0, &resp)) return false;
    if ((resp & RSP0_COUNTER_MASK) != stored) return true;
    delay(2);
  }
  return false;
}

bool Si1133Component::reset_sensor_() {
  delay(30);
  if (!write_reg_(REG_COMMAND, CMD_RESET)) return false;
  delay(10);
  return true;
}

bool Si1133Component::init_sensor_() {
  delay(5);
  if (!reset_sensor_()) return false;
  delay(10);

  bool ok = true;
  ok &= set_parameter_(PARAM_CH_LIST,    0x0F);
  ok &= set_parameter_(PARAM_ADCCONFIG0, 0x78);
  ok &= set_parameter_(PARAM_ADCSENS0,   0x71);
  ok &= set_parameter_(PARAM_ADCPOST0,   0x40);
  ok &= set_parameter_(PARAM_ADCCONFIG1, 0x4D);
  ok &= set_parameter_(PARAM_ADCSENS1,   0xE1);
  ok &= set_parameter_(PARAM_ADCPOST1,   0x40);
  ok &= set_parameter_(PARAM_ADCCONFIG2, 0x41);
  ok &= set_parameter_(PARAM_ADCSENS2,   0xE1);
  ok &= set_parameter_(PARAM_ADCPOST2,   0x50);
  ok &= set_parameter_(PARAM_ADCCONFIG3, 0x4D);
  ok &= set_parameter_(PARAM_ADCSENS3,   0x87);
  ok &= set_parameter_(PARAM_ADCPOST3,   0x40);
  if (ok) ok &= write_reg_(REG_IRQ_ENABLE, 0x0F);
  return ok;
}

// ── Messung ──────────────────────────────────────────────────────────────────
bool Si1133Component::read_samples_(Samples_t *s) {
  uint8_t buf[13];
  if (!read_block_(REG_IRQ_STATUS, 13, buf)) return false;

  s->irq_status = buf[0];

  auto s24 = [](uint8_t h, uint8_t m, uint8_t l) -> int32_t {
    int32_t v = (int32_t(h) << 16) | (int32_t(m) << 8) | int32_t(l);
    return (v & 0x800000) ? (v | int32_t(0xFF000000)) : v;
  };

  s->ch0 = s24(buf[1],  buf[2],  buf[3]);
  s->ch1 = s24(buf[4],  buf[5],  buf[6]);
  s->ch2 = s24(buf[7],  buf[8],  buf[9]);
  s->ch3 = s24(buf[10], buf[11], buf[12]);
  return true;
}

// Polynom-Hilfsfunktion – exakt wie im SiLabs-Referenztreiber
int32_t Si1133Component::poly_helper_(int32_t input, int8_t fraction, uint16_t mag, int8_t shift) {
  if (shift < 0)
    return ((input << fraction) / mag) >> (-shift);
  return ((input << fraction) / mag) << shift;
}

int32_t Si1133Component::polynomial_(int32_t x, int32_t y,
                                     uint8_t in_frac, uint8_t out_frac,
                                     uint8_t n, const Coeff_t *kp) {
  int32_t out = 0;
  for (uint8_t c = 0; c < n; c++, kp++) {
    // info: low byte = x/y order + sign, high byte = shift (encoded)
    uint8_t  info    = kp->info & 0xFF;
    uint8_t  x_order = GET_X_ORDER(info);
    uint8_t  y_order = GET_Y_ORDER(info);
    int8_t   sign    = GET_SIGN(info) ? -1 : 1;
    // Shift-Dekodierung exakt wie im Original:
    // shift = -(((high_byte) ^ 0xFF) + 1)  →  entspricht -( ~high_byte + 1 ) = negativer Wert
    uint8_t  high    = (uint16_t(kp->info) >> 8) & 0xFF;
    int8_t   shift   = -(int8_t((high ^ 0xFF) + 1));
    uint16_t mag     = kp->mag;

    if (x_order == 0 && y_order == 0) {
      out += sign * int32_t(mag) << out_frac;
    } else {
      int32_t x1 = 1, x2 = 1, y1 = 1, y2 = 1;
      if (x_order > 0) {
        x1 = poly_helper_(x, in_frac, mag, shift);
        if (x_order > 1) x2 = poly_helper_(x, in_frac, mag, shift);
      }
      if (y_order > 0) {
        y1 = poly_helper_(y, in_frac, mag, shift);
        if (y_order > 1) y2 = poly_helper_(y, in_frac, mag, shift);
      }
      out += sign * x1 * x2 * y1 * y2;
    }
  }
  return (out < 0) ? -out : out;
}

int32_t Si1133Component::calc_uv_(int32_t uv) {
  return polynomial_(0, uv, UV_INPUT_FRACTION, UV_OUTPUT_FRACTION, UV_NUMCOEFF, uk_);
}

int32_t Si1133Component::calc_lux_(int32_t vis_high, int32_t vis_low, int32_t ir) {
  if (vis_high > ADC_THRESHOLD || ir > ADC_THRESHOLD)
    return polynomial_(vis_high, ir, INPUT_FRACTION_HIGH, LUX_OUTPUT_FRACTION, NUMCOEFF_HIGH, lk_.coeff_high);
  return polynomial_(vis_low, ir, INPUT_FRACTION_LOW, LUX_OUTPUT_FRACTION, NUMCOEFF_LOW, lk_.coeff_low);
}

// ── ESPHome Lifecycle ─────────────────────────────────────────────────────────
void Si1133Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Si1133...");

  uint8_t part_id = 0;
  if (!read_reg_(REG_PART_ID, &part_id)) {
    ESP_LOGE(TAG, "I2C-Lesefehler – Verkabelung und Adresse prüfen");
    this->mark_failed();
    return;
  }
  if (part_id != 0x33) {
    ESP_LOGE(TAG, "Falsche PART_ID: 0x%02X (erwartet 0x33)", part_id);
    this->mark_failed();
    return;
  }
  if (!init_sensor_()) {
    ESP_LOGE(TAG, "Sensor-Initialisierung fehlgeschlagen");
    this->mark_failed();
    return;
  }
  ESP_LOGD(TAG, "Si1133 bereit (PART_ID=0x%02X)", part_id);
}

void Si1133Component::update() {
  // Force-Measurement auslösen
  if (!send_cmd_(CMD_FORCE_CH)) {
    ESP_LOGW(TAG, "FORCE_CH fehlgeschlagen");
    this->status_set_warning();
    return;
  }

  // Auf alle 4 Kanäle warten (IRQ_STATUS == 0x0F)
  uint8_t irq = 0;
  uint32_t deadline = millis() + MEASURE_WAIT_MS;
  while (millis() < deadline) {
    if (!read_reg_(REG_IRQ_STATUS, &irq)) break;
    if (irq == 0x0F) break;
    delay(5);
  }
  if (irq != 0x0F) {
    ESP_LOGW(TAG, "Mess-Timeout (IRQ=0x%02X)", irq);
    this->status_set_warning();
    return;
  }

  Samples_t s{};
  if (!read_samples_(&s)) {
    ESP_LOGW(TAG, "Lesefehler Messdaten");
    this->status_set_warning();
    return;
  }

  this->status_clear_warning();

  if (ambient_light_sensor_ != nullptr) {
    float lux = float(calc_lux_(s.ch1, s.ch3, s.ch2)) / float(1 << LUX_OUTPUT_FRACTION);
    if (lux < 0.0f) lux = 0.0f;
    ESP_LOGD(TAG, "Lux: %.2f", lux);
    ambient_light_sensor_->publish_state(lux);
  }

  if (uv_index_sensor_ != nullptr) {
    float uvi = float(calc_uv_(s.ch0)) / float(1 << UV_OUTPUT_FRACTION);
    if (uvi < 0.0f) uvi = 0.0f;
    ESP_LOGD(TAG, "UV-Index: %.2f", uvi);
    uv_index_sensor_->publish_state(uvi);
  }
}

void Si1133Component::dump_config() {
  ESP_LOGCONFIG(TAG, "Si1133:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed())
    ESP_LOGCONFIG(TAG, "  Kommunikation fehlgeschlagen!");
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Ambient Light", ambient_light_sensor_);
  LOG_SENSOR("  ", "UV Index",      uv_index_sensor_);
}

}  // namespace si1133
}  // namespace esphome
