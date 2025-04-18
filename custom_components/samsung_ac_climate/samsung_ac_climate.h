#pragma once
#include "esphome/components/climate/climate.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include"ir_Samsung.h"

namespace esphome {
namespace samsung_ac_climate {

class SamsungACClimate : public climate::Climate, public Component {
 public:
  // Set the remote transmitter
  void set_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) { transmitter_ = transmitter; }
  // Set the physical transmitter pin (used for IRSamsungAc constructor)
  void set_transmitter_pin(uint8_t pin) { tx_pin_ = pin; }
  // Set whether turbo mode is supported (unused in this example)
  void set_turbo_support(bool turbo_supported) { turbo_supported_ = turbo_supported; }

  void setup() override;
  void control(const climate::ClimateCall &call) override;
  climate::ClimateTraits traits() override;
  climate::ClimatePreset preset{climate::CLIMATE_PRESET_NONE};

 protected:
  void transmit_state_();

  // Remove the preallocated IRSamsungAc member.
  // Instead, we'll construct a temporary object in transmit_state_.
  remote_transmitter::RemoteTransmitterComponent *transmitter_{nullptr};
  uint8_t tx_pin_{0};
  bool turbo_supported_{true};
};

}  // namespace samsung_ac_climate
}  // namespace esphome
