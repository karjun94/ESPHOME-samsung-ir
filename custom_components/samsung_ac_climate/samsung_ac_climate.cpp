#include "samsung_ac_climate.h"
#include "esphome/core/log.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Samsung.h>

namespace esphome {
namespace samsung_ac_climate {

static const char *const TAG = "samsung_ac_climate";

// Quiet period before transmitting; rapid taps coalesce into one IR frame.
static const uint32_t TX_DEBOUNCE_MS = 500;

void SamsungACClimate::setup() {
  ESP_LOGI(TAG, "Setting up Samsung AC Climate component");

  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->target_temperature = 25.0f;
    this->fan_mode = climate::CLIMATE_FAN_LOW;
    this->swing_mode = climate::CLIMATE_SWING_OFF;
  }
  // A preset is momentary; never resurrect one across a reboot (its revert
  // timer would be gone and it would stick forever).
  this->preset = climate::CLIMATE_PRESET_NONE;
  this->publish_state();
}

void SamsungACClimate::loop() {
  if (this->sensor_ != nullptr && this->sensor_->has_state()) {
    float t = this->sensor_->state;
    if (!isnan(t) && this->current_temperature != t) {
      this->current_temperature = t;
      this->publish_state();
    }
  }
}

climate::ClimateTraits SamsungACClimate::traits() {
  climate::ClimateTraits traits;
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_COOL,
      climate::CLIMATE_MODE_DRY,
      climate::CLIMATE_MODE_FAN_ONLY,
      climate::CLIMATE_MODE_AUTO,
  });
  traits.set_supported_fan_modes({
      climate::CLIMATE_FAN_AUTO,
      climate::CLIMATE_FAN_LOW,
      climate::CLIMATE_FAN_MEDIUM,
      climate::CLIMATE_FAN_HIGH,
      climate::CLIMATE_FAN_FOCUS,
  });
  traits.set_supported_swing_modes({
      climate::CLIMATE_SWING_OFF,
      climate::CLIMATE_SWING_VERTICAL,
  });
  traits.set_supported_presets({
      climate::CLIMATE_PRESET_NONE,
      climate::CLIMATE_PRESET_ECO,
      climate::CLIMATE_PRESET_BOOST,
  });
  traits.set_visual_min_temperature(16.0f);
  traits.set_visual_max_temperature(30.0f);
  traits.set_visual_temperature_step(1.0f);
  return traits;
}

void SamsungACClimate::control(const climate::ClimateCall &call) {
  const bool preset_changed = call.get_preset().has_value();

  if (call.get_mode().has_value())
    this->mode = *call.get_mode();

  if (call.get_target_temperature().has_value())
    this->target_temperature = *call.get_target_temperature();

  if (call.get_fan_mode().has_value())
    this->fan_mode = *call.get_fan_mode();

  if (call.get_swing_mode().has_value())
    this->swing_mode = *call.get_swing_mode();

  if (preset_changed)
    this->preset = *call.get_preset();

  // ── Preset lifecycle: momentary, like the remote's Turbo button ─────────
  //
  // Selecting BOOST/ECO transmits the special frame NOW and arms a revert
  // timer. When it fires, preset returns to NONE and the normal frame is
  // retransmitted -- which restores the REMEMBERED fan speed on the unit.
  // The stored fan_mode is never modified by presets: the IR frame carries
  // fan Turbo (Boost) or Auto (Eco) while active because the protocol
  // requires it, but that lives only in the frame, exactly like the remote.
  //
  if (this->mode == climate::CLIMATE_MODE_OFF) {
    this->preset = climate::CLIMATE_PRESET_NONE;
    this->cancel_timeout("preset_revert");
  } else if (preset_changed) {
    if (this->preset.value_or(climate::CLIMATE_PRESET_NONE) != climate::CLIMATE_PRESET_NONE) {
      if (this->preset == climate::CLIMATE_PRESET_BOOST) {
        // Turbo is a cool-mode feature on these units.
        this->mode = climate::CLIMATE_MODE_COOL;
      }
      // Display-sync only: the unit ends turbo/eco BY ITSELF and falls back
      // to its internally remembered fan speed. We deliberately do NOT
      // retransmit here -- sending another frame could restart the unit's
      // turbo window or fight its own fallback. This timer just flips the
      // HA preset display back to NONE at roughly the same time.
      this->set_timeout("preset_revert", this->preset_timeout_, [this]() {
        if (this->preset.value_or(climate::CLIMATE_PRESET_NONE) == climate::CLIMATE_PRESET_NONE)
          return;
        ESP_LOGI(TAG, "Preset window elapsed, clearing displayed preset (no IR sent)");
        this->preset = climate::CLIMATE_PRESET_NONE;
        this->publish_state();
      });
    } else {
      // User manually cleared the preset.
      this->cancel_timeout("preset_revert");
    }
  }

  this->publish_state();  // UI updates immediately

  // Debounced transmit: only the final coalesced state goes out over IR.
  this->set_timeout("tx", TX_DEBOUNCE_MS, [this]() { this->transmit_state_(); });
}

void SamsungACClimate::transmit_state_() {
  // Pure function of current state -- no mutations of this->* in here.
  IRSamsungAc ac(this->tx_pin_);
  ac.begin();

  if (this->mode == climate::CLIMATE_MODE_OFF) {
    ac.off();
    ac.send();
    ESP_LOGI(TAG, "IR sent: power OFF");
    return;
  }

  ac.on();
  ac.setTemp(static_cast<uint8_t>(this->target_temperature));

  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      ac.setMode(kSamsungAcCool);
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      ac.setMode(kSamsungAcFan);
      break;
    case climate::CLIMATE_MODE_DRY:
      ac.setMode(kSamsungAcDry);
      break;
    case climate::CLIMATE_MODE_AUTO:
      ac.setMode(kSamsungAcAuto);  // protocol: auto mode uses its own fan speed
      break;
    default:
      ac.setMode(kSamsungAcCool);
      break;
  }

  // The user's remembered fan selection. Frame-level only overrides happen
  // below for presets; this->fan_mode itself is never changed by them.
  switch (this->fan_mode.value_or(climate::CLIMATE_FAN_AUTO)) {
    case climate::CLIMATE_FAN_LOW:
      ac.setFan(kSamsungAcFanLow);
      break;
    case climate::CLIMATE_FAN_MEDIUM:
      ac.setFan(kSamsungAcFanMed);
      break;
    case climate::CLIMATE_FAN_HIGH:
      ac.setFan(kSamsungAcFanHigh);
      break;
    case climate::CLIMATE_FAN_FOCUS:
      ac.setFan(kSamsungAcFanTurbo);
      break;
    default:
      ac.setFan(kSamsungAcFanAuto);
      break;
  }

  ac.setSwing(this->swing_mode == climate::CLIMATE_SWING_VERTICAL);

  // Presets last: in the library, setPowerful(true) overrides fan to Turbo
  // in the FRAME, setEcono(true) overrides fan to Auto + swing on in the
  // FRAME. Our stored fan_mode is untouched, so when the preset reverts the
  // next frame carries the remembered speed again.
  ac.setPowerful(this->preset == climate::CLIMATE_PRESET_BOOST);
  ac.setEcono(this->preset == climate::CLIMATE_PRESET_ECO);

  ac.send();
  ESP_LOGI(TAG, "IR sent: mode=%d temp=%d fan=%d preset=%d", (int) this->mode,
           (int) this->target_temperature,
           (int) this->fan_mode.value_or(climate::CLIMATE_FAN_AUTO),
           (int) this->preset.value_or(climate::CLIMATE_PRESET_NONE));
}

}  // namespace samsung_ac_climate
}  // namespace esphome
