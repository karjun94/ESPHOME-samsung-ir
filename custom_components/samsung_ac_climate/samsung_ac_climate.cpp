#include "samsung_ac_climate.h"
#include "esphome/core/log.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Samsung.h>

namespace esphome {
namespace samsung_ac_climate {

static const char *const TAG = "samsung_ac_climate";

void SamsungACClimate::setup() {
  ESP_LOGI(TAG, "Setting up Samsung AC Climate component");
  this->mode = climate::CLIMATE_MODE_OFF;
  this->target_temperature = 24.0f;
  this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
  this->swing_mode = climate::CLIMATE_SWING_OFF;  // Initialize swing mode
  this->preset = climate::CLIMATE_PRESET_NONE;// Initialize swing mode
  this->publish_state();
}

climate::ClimateTraits SamsungACClimate::traits() {
  climate::ClimateTraits traits;
  traits.set_supports_current_temperature(false);
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
  if (call.get_mode().has_value())
    this->mode = *call.get_mode();

  if (call.get_target_temperature().has_value())
    this->target_temperature = *call.get_target_temperature();

  if (call.get_fan_mode().has_value())
    this->fan_mode = *call.get_fan_mode();

  if (call.get_swing_mode().has_value())
    this->swing_mode = *call.get_swing_mode();
  
  if (call.get_preset().has_value())
    this->preset  = *call.get_preset();


  this->transmit_state_();
  this->publish_state();
}

void SamsungACClimate::transmit_state_() {
  IRSamsungAc ac(this->tx_pin_);  // Pin not used here
  ac.begin();
  ac.off();

  if (this->mode != climate::CLIMATE_MODE_OFF) {
    ac.on();
    ac.setTemp(static_cast<uint8_t>(this->target_temperature));

    switch (this->mode) {
      case climate::CLIMATE_MODE_COOL:
        ac.setMode(kSamsungAcCool);
        ac.setPowerful(true);
        break;
      case climate::CLIMATE_MODE_FAN_ONLY:
        ac.setMode(kSamsungAcFan);
	ac.setPowerful(false);
        break;
      case climate::CLIMATE_MODE_DRY:
        ac.setMode(kSamsungAcDry);
        ac.setPowerful(false);
        break;
      case climate::CLIMATE_MODE_AUTO:
        ac.setMode(kSamsungAcAuto);
        break;
      default:
        break;
    }


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


    //  Set vertical swing
    if (this->swing_mode == climate::CLIMATE_SWING_VERTICAL) {
      ac.setSwing(true);
    } else {
      ac.setSwing(false);
    }
  }

   if (this->preset == climate::CLIMATE_PRESET_BOOST) {
      this->mode = climate::CLIMATE_MODE_COOL;
      ac.setMode(kSamsungAcCool);   
      ac.setFan(kSamsungAcFanTurbo);  
      ac.setPowerful(true);  // Turbo mode
   } else {
     ac.setPowerful(false);
   }

   if (this->preset == climate::CLIMATE_PRESET_ECO) {
     this->mode = climate::CLIMATE_MODE_DRY;
     ac.setMode(kSamsungAcCool);
     ac.setEcono(true);  // if supported
   } else {
     ac.setEcono(false);
   }


  //IRsend irsend(this->tx_pin_);
  //irsend.begin();
  //irsend.sendSamsungAC(ac.getRaw(), kSamsungAcStateLength);
  ac.send();
  ESP_LOGI(TAG, "IR command sent via IRsend.sendSamsungAC : ");
}

}  // namespace samsung_ac_climate
}  // namespace esphome
