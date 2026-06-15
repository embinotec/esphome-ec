# ESPHome DRV2605 Haptic Motor Driver Component

An ESPHome custom component for the DRV2605 haptic motor driver IC. The DRV2605 is designed to drive both ERM (Eccentric Rotating Mass) and LRA (Linear Resonant Actuator) haptic motors with a library of 123+ pre-programmed haptic effects.

## Features

- ✅ Support for both ERM and LRA motors
- ✅ 123 built-in haptic effects
- ✅ Sequence playback (up to 8 effects)
- ✅ Real-time control mode for custom vibration patterns
- ✅ I2C communication (address 0x5A)
- ✅ ESPHome automation actions
- ✅ Low power standby mode

## Hardware

This component works with:
- [Adafruit DRV2605 Haptic Motor Driver](https://www.adafruit.com/product/2305)
- Waveshare Rotary Knob with DRV2605
- Any DRV2605-based haptic driver board

### Wiring

Connect the DRV2605 to your ESP32/ESP8266 via I2C:

| DRV2605 | ESP32 | ESP8266 |
|---------|-------|---------|
| VIN     | 3.3V  | 3.3V    |
| GND     | GND   | GND     |
| SDA     | GPIO21| GPIO4   |
| SCL     | GPIO22| GPIO5   |

Connect your haptic motor to the motor terminals on the DRV2605 board.

## Installation

### External Component (Recommended)

Add to your ESPHome configuration:

```yaml
external_components:
  - source: github://RAR/esphome-drv2605
    components: [ drv2605 ]
```

### Local Component

1. Clone this repository
2. Copy the `components/drv2605` folder to your ESPHome configuration directory under `custom_components/`

## Configuration

### Basic Configuration

```yaml
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true

drv2605:
  id: haptic_driver
  motor_type: ERM  # ERM or LRA
  library: 1       # 1-5 for ERM, 6 for LRA
```

### Configuration Variables

- **id** (*Optional*, ID): Manually specify the ID used for code generation.
- **motor_type** (*Optional*, string): Type of motor connected. Either `ERM` (default) or `LRA`.
- **library** (*Optional*, int): Effect library to use. Range 0-6, default is 1.
  - 0: Empty
  - 1-5: ERM libraries (different effect sets)
  - 6: LRA library

All standard I2C configuration options are also available.

## Actions

### `drv2605.play_effect`

Play a single haptic effect by ID (1-123).

```yaml
on_press:
  - drv2605.play_effect:
      id: haptic_driver
      effect: 47  # Strong Click 100%
```

**Configuration variables:**
- **id** (*Required*, ID): The ID of the DRV2605 component.
- **effect** (*Required*, int): Effect ID (1-123). Supports templates.

### `drv2605.play_sequence`

Play a sequence of up to 8 haptic effects.

```yaml
on_press:
  - drv2605.play_sequence:
      id: haptic_driver
      effects: [14, 1, 14, 1]  # Ramp pattern
```

**Configuration variables:**
- **id** (*Required*, ID): The ID of the DRV2605 component.
- **effects** (*Required*, list): List of 1-8 effect IDs.

### `drv2605.stop`

Stop any currently playing effects.

```yaml
on_release:
  - drv2605.stop:
      id: haptic_driver
```

### `drv2605.set_realtime_value`

Set a real-time vibration value (0-255) for continuous control.

```yaml
on_...:
  - drv2605.set_realtime_value:
      id: haptic_driver
      value: 128  # Mid-level vibration
```

**Configuration variables:**
- **id** (*Required*, ID): The ID of the DRV2605 component.
- **value** (*Required*, int): Vibration intensity (0-255). Supports templates.

## Haptic Effects

The DRV2605 includes 123 built-in effects. Here are some popular ones:

| ID  | Effect Name |
|-----|-------------|
| 1   | Strong Click 100% |
| 7   | Soft Bump 100% |
| 10  | Double Click 100% |
| 11  | Triple Click 100% |
| 14  | Soft Fuzz 60% |
| 16  | Strong Buzz 100% |
| 47  | Buzz 1 100% |
| 52  | Pulsing Strong 1 100% |

See the [DRV2605 datasheet](https://www.ti.com/lit/ds/symlink/drv2605.pdf) for the complete list of effects.

## Example Use Cases

### Button Click Feedback

```yaml
binary_sensor:
  - platform: gpio
    pin: GPIO0
    name: "Button"
    on_press:
      - drv2605.play_effect:
          id: haptic_driver
          effect: 1  # Strong Click
```

### Notification Pattern

```yaml
script:
  - id: notify_haptic
    then:
      - drv2605.play_sequence:
          id: haptic_driver
          effects: [16, 0, 16, 0, 16]  # Three buzzes
```

### Rotary Encoder Feedback

```yaml
sensor:
  - platform: rotary_encoder
    name: "Volume"
    on_value:
      - drv2605.play_effect:
          id: haptic_driver
          effect: 7  # Soft Bump
```

### Variable Intensity Control

```yaml
number:
  - platform: template
    name: "Haptic Intensity"
    min_value: 0
    max_value: 255
    step: 1
    set_action:
      - drv2605.set_realtime_value:
          id: haptic_driver
          value: !lambda "return x;"
```

## Motor Type Selection

### ERM (Eccentric Rotating Mass)
Traditional vibration motors with rotating weights. Use libraries 1-5.

### LRA (Linear Resonant Actuator)  
Modern linear vibration actuators. Use library 6.

Make sure to configure the `motor_type` to match your hardware!

### How to Determine Your Motor Type

**For Waveshare ESP32-S3 Knob (Touch LCD 1.8):**
The documentation doesn't explicitly specify ERM or LRA. **Start with LRA (library 6)** as modern compact devices typically use LRA motors. If the vibration is weak or non-functional:
1. Try `motor_type: ERM` with `library: 1`
2. Test different libraries (1-5 for ERM)

**Testing procedure:**
```yaml
# Test 1: Try LRA first (most likely)
drv2605:
  motor_type: LRA
  library: 6

# Test 2: If weak/no vibration, try ERM
drv2605:
  motor_type: ERM
  library: 1  # Try libraries 1-5
```

You'll know it's correct when:
- Effect 1 (Strong Click) produces a clear, sharp feedback
- Effect 47 (Buzz) produces sustained vibration
- The haptic feels responsive and not "buzzy" or weak

## Troubleshooting

### Device not found
- Check I2C wiring (SDA/SCL)
- Verify power supply (3.3V)
- Run I2C scan to confirm address (should be 0x5A)

### No vibration
- Verify motor is properly connected to motor terminals
- Check motor type matches configuration (ERM vs LRA)
- Ensure library selection is appropriate for motor type
- Test with a simple effect like effect 1

### Weak vibration
- Check power supply can provide adequate current
- Try different libraries for ERM motors
- Adjust motor voltage rating in library selection

## License

MIT License - see LICENSE file for details

## Credits

Based on the DRV2605 datasheet from Texas Instruments and inspired by the Adafruit DRV2605 Arduino library.

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.
