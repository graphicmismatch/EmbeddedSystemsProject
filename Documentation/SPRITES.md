# Required Sprite List

## Recommended Format

- Size: `16x16` pixels. The current firmware in `Images.h` declares every pet sprite as `16x16`, so replacement art should use the same dimensions unless the code is updated too.
- Color: monochrome, with the pet drawn in black on a white or transparent background.
- Keep every frame in a given animation the same canvas size and aligned to the same baseline.

## Required Pet Sprites

There are 3 health tiers and 4 behavior states.

- `pet_healthy_sleep_0.png`
- `pet_healthy_sleep_1.png`
- `pet_healthy_awake_0.png`
- `pet_healthy_awake_1.png`
- `pet_healthy_happy_0.png`
- `pet_healthy_happy_1.png`
- `pet_healthy_surprised_0.png`
- `pet_healthy_surprised_1.png`
- `pet_mid_sleep_0.png`
- `pet_mid_sleep_1.png`
- `pet_mid_awake_0.png`
- `pet_mid_awake_1.png`
- `pet_mid_happy_0.png`
- `pet_mid_happy_1.png`
- `pet_mid_surprised_0.png`
- `pet_mid_surprised_1.png`
- `pet_low_sleep_0.png`
- `pet_low_sleep_1.png`
- `pet_low_awake_0.png`
- `pet_low_awake_1.png`
- `pet_low_happy_0.png`
- `pet_low_happy_1.png`
- `pet_low_surprised_0.png`
- `pet_low_surprised_1.png`

## Optional But Useful

- `pet_dead.png`
- `pet_leave.png`
- `item_treat.png`
- `item_gift.png`
- `icon_health.png`
- `icon_affection.png`
- `icon_activity.png`
- `icon_sleep.png`

## Notes

- The checked-in placeholder sprites and the current `Bitmap` declarations are all `16x16`.
- The sample file `test.png` is `32x32`, but that is not the size the game currently expects.
- If you want to save flash, each state can start with 2 frames.
- If you later want smoother motion, add `_2`, `_3`, etc. and keep the naming pattern.
- The converter script outputs `Bitmap` objects that match the current project format.
