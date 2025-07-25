## EvoCmdWing V0.2

Sends key custom key combos to grandma3 when paired with EvoCmdWing_keyboard_shortcuts.xml you will have a nice control surface.
You can use the provided qmk firmware or compile your own by changing the keymap and/or pin mappings.

Sends relative midi from 5 attribute encoders for use with Pro Plugins Midi Encoders plugin.

Sends absolute midi to 8 Xkeys encoders (execs 291-298) use with the LUA script, lua script sends midi back to keep the values synced.

Syncs XKeys leds with grandMA3 sequence apperences.

See the included images for MidiRemote setup and naming of remotes.

the Lua script also will reassign sequences to the encoders when they change to keep them in sync with page changes and exec changes.

Check the `/help_files` for pin assignments and wiring diagram
Uses 2.7mm ws2812 leds for XKeys and Logo

![cmdwing Image](images/cmdwing.jpg)

Instructions comming soon, for now you should check out this project

https://www.thingiverse.com/thing:6464653

which I used as a starting point, so i give thanks to Nathan_I3 for the inspiration for this project.

I redesigned it to have encoders for Xkeys and to have a closer layout to the consoles.

Also check out my EvoFaderWing for a complete at home programming setup.
