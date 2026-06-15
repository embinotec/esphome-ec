# Quick Start Guide - ESPHome DRV2605 Component

## For Waveshare Rotary Knob Users

Your Waveshare Rotary Knob has a DRV2605 haptic driver built-in. This component allows you to control the haptic feedback from ESPHome.

## Step 1: Add to Your ESPHome Configuration

Add this to your `.yaml` file:

```yaml
external_components:
  - source: github://RAR/esphome-drv2605
    components: [ drv2605 ]

i2c:
  # Check your specific board's I2C pins
  # The knob has dual MCUs, use the ESP32-S3's I2C pins
  sda: GPIO_SDA  # Verify from schematic/wiki
  scl: GPIO_SCL  # Verify from schematic/wiki
  scan: true

drv2605:
  id: haptic_driver
  motor_type: LRA  # Most likely LRA for compact design
  library: 6       # LRA library (use 1-5 if it's actually ERM)
```

**Note**: You may need to test both motor types:
- Start with `motor_type: LRA` and `library: 6`
- If vibration is weak or doesn't work, try `motor_type: ERM` and `library: 1`

## Step 2: Test the Haptic Driver

Add a simple test button:

```yaml
button:
  - platform: template
    name: "Test Haptic"
    on_press:
      - drv2605.play_effect:
          id: haptic_driver
          effect: 47  # Strong buzz
```

## Step 3: Add Rotary Encoder Feedback

If your Waveshare knob has a rotary encoder:

```yaml
sensor:
  - platform: rotary_encoder
    name: "Rotary Position"
    pin_a: GPIO_A  # Replace with actual pin
    pin_b: GPIO_B  # Replace with actual pin
    on_value:
      - drv2605.play_effect:
          id: haptic_driver
          effect: 7  # Soft bump on rotation
```

## Step 4: Add Button Click Feedback

For the rotary encoder button press:

```yaml
binary_sensor:
  - platform: gpio
    pin: GPIO_BUTTON  # Replace with actual pin
    name: "Rotary Button"
    on_press:
      - drv2605.play_effect:
          id: haptic_driver
          effect: 1  # Strong click
```

## Common Effects to Try

- Effect 1: Strong Click 100%
- Effect 7: Soft Bump 60%
- Effect 10: Double Click
- Effect 14: Soft Fuzz
- Effect 47: Buzz

## Next Steps

1. Upload the configuration to your ESP device
2. Check the logs to verify I2C communication
3. Test different effects to find what you like
4. Customize the feedback patterns for your use case

See `README.md` for complete documentation and advanced usage examples.
