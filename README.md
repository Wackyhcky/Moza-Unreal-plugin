# Moza DirectInput Plugin

This Unreal Engine 5.7 plugin exposes Moza steering wheels through the legacy DirectInput API so they can be used both in the editor and in packaged builds.

## Installation
1. Copy the `Plugins/MozaDirectInput` folder into your project's `Plugins` directory (create it if needed).
2. The plugin is enabled by default; open your project in Unreal Editor and click **Yes** when prompted to rebuild modules.
3. Ensure your Moza wheel base is connected before launching the editor so the plugin can acquire the device. The plugin prefers devices whose product name contains "MOZA" and will fall back to the first available DirectInput device if no Moza hardware is present. If the output log reports that no DirectInput device was found, unplug and replug the wheel and restart the editor.

## Usage
- The plugin maps wheel axes to standard gamepad channels:
  - Wheel rotation → `Gamepad_LeftX`
  - Brake → `Gamepad_LeftY`
  - Throttle → `Gamepad_RightTrigger`
  - Clutch/secondary axis → `Gamepad_LeftTrigger`
- Buttons are routed to the `Gamepad FaceButton Bottom` key for basic binding. You can expand the mapping inside `FMozaDirectInputDevice::DispatchEvents` to bind each button individually.
- Bind to these keys in your input settings to drive in-editor Pawn or runtime vehicles.

## Notes
- Only Windows (Win64) is supported because the implementation relies on DirectInput.
- Force feedback and per-button key registration are left as extension points; the stub functions can be expanded for richer support as needed.
