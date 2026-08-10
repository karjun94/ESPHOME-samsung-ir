#pragma once
#include "esphome/components/climate/climate.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "esphome/components/sensor/sensor.h"
#include "ir_Samsung.h"

namespace esphome {
namespace samsung_ac_climate {

class SamsungACClimate : public climate::Climate, public Component {
 public:
  void set_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) { transmitter_ = transmitter; }
  void set_transmitter_pin(uint8_t pin) { tx_pin_ = pin; }
  void set_turbo_support(bool turbo_supported) { turbo_supported_ = turbo_supported; }
  void set_temperature_sensor(sensor::Sensor *sensor) { this->sensor_ = sensor; }
  // How long a preset (BOOST/ECO) stays active before auto-reverting to NONE
  void set_preset_timeout(uint32_t ms) { preset_timeout_ = ms; }

  void setup() override;
  void loop() override;
  void control(const climate::ClimateCall &call) override;
  climate::ClimateTraits traits() override;

  // NOTE: no `preset` member here -- the base climate::Climate already has
  // `optional<ClimatePreset> preset`; re-declaring it shadowed the base.

 protected:
  void transmit_state_();

  remote_transmitter::RemoteTransmitterComponent *transmitter_{nullptr};
  sensor::Sensor *sensor_{nullptr};
  uint8_t tx_pin_{0};
  bool turbo_supported_{true};
  uint32_t preset_timeout_{30 * 60 * 1000};  // default 30 min, like the remote's Turbo
};

}  // namespace samsung_ac_climate
}  // namespace esphome
