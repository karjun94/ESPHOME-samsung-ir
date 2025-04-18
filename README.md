# IR Blaster for Samsung AC and Panasonic Fan (ESPHome)

This project uses ESPHome to control a **Samsung air conditioner** and a **Panasonic IR fan** using a single ESP8266-based IR blaster (e.g., Wemos D1 Mini).

## 📦 Features

### ✅ Samsung AC Control
- Full climate control via IR using the `samsung_ac_climate` component.
- Supports:
  - Power ON/OFF
  - Temperature setting
  - Mode selection
  - Turbo mode

### ✅ Panasonic Fan Control
- Fan control with 3 speed levels.
- Power ON/OFF handling using raw IR codes.

## ⚙️ Hardware
- ESP8266 (e.g., Wemos D1 Mini)
- IR LED + transistor circuit (with 5V supply)

### Remote Transmitter
```yaml
remote_transmitter:
  id: ir_tx
  pin: 4
  carrier_duty_percent: 50%

Big thank you to the community that made this possible especially the team that made the ESPHOME platform and the team that maintans IRremoteESP8266 library.
