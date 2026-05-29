# m5_tab5_component

Tab5 board support component for ESP-IDF.

## Structure

- `include/`: public API exposed to the application.
- `src/drivers/`: individual display and sensor driver descriptors.
- `src/variants/`: variant registry that composes one board variant from one display driver and zero or more sensor drivers.

## Variant management

Use composition instead of scattering `#ifdef` branches through business logic:

1. Add one driver descriptor per physical device family, such as a specific panel IC or sensor IC.
2. Define one board variant descriptor that references the display driver and the sensor list used by that hardware revision.
3. Keep board detection in the variant registry, using GPIO straps, I2C probe, display ID reads, EEPROM data, or factory SKU information.
4. Use Kconfig only to choose the selection strategy, such as auto-detect vs force a known variant. Do not use Kconfig to model every hardware combination.

This keeps drivers reusable and makes new screen or sensor variants a registry change rather than a large refactor.