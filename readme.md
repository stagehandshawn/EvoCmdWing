# EvoCmdWing - [Wiki](https://github.com/stagehandshawn/EvoCmdWing/wiki)

<table width="100%">
  <tr>
    <td width="70%">
      <table width="100%">
        <tr>
          <td align="left">
            <strong>Author:</strong> ShawnR
          </td>
          <td align="right">
            <strong>Contact:</strong> <a href="mailto:stagehandshawn@gmail.com">stagehandshawn@gmail.com</a>
          </td>
        </tr>
      </table>
      <br>
      Control grandMA3 onPC with a custom keyboard with 5 attribute encoders and 8 XKey encoders synced using Midi, backlit Xkeys with on/off and color status (follows appearances).
      <br><br><blockquote>
        <strong>Note:</strong> This Guide is a work in progress but EvocmdWing is fully functional. I continue to work on the repo as time permits.
      </blockquote>
    </td>
    <td width="30%" align="right" valign="top">
      <img src="https://raw.githubusercontent.com/stagehandshawn/EvoCmdWing/main/images/v02built/evocmdwing_top1.png" alt="cmdwing" width="350">
    </td>
  </tr>
</table>

---
## Project Images

<table>
  <tr>
    <td width="50%">
      <img src="https://raw.githubusercontent.com/stagehandshawn/EvoCmdWing/main/images/v02built/evocmdwing_top2_right.png" alt="cmdwingtop" width="100%">
    </td>
    <td width="50%">
      <img src="https://raw.githubusercontent.com/stagehandshawn/EvoCmdWing/main/images/v02built/evocmdwing_side2.png" alt="cmdwingside" width="100%">
    </td>
  </tr>
  <tr>
    <td width="50%">
      <img src="https://raw.githubusercontent.com/stagehandshawn/EvoCmdWing/main/images/v02built/evocmdwing_back1.png" alt="cmdwingback" width="100%">
    </td>
    <td width="50%">
      <img src="https://raw.githubusercontent.com/stagehandshawn/EvoCmdWing/main/images/cmdwing.jpg" alt="faderwingcmdwing" width="100%">
    </td>
  </tr>
</table>

**Watch the demo video**

[![Watch the demo video](https://img.youtube.com/vi/7ICJlkqCzxY/0.jpg)](https://www.youtube.com/watch?v=7ICJlkqCzxY)


## Overview
 - Sends custom keys to grandMA3 onPC when paired with `/help_files/EvoCmdWingv0.2_keyboard_shortcuts.xml` you will have a nice control surface.
 - LED feedback for XKeys.  
 - 8 extra encoders for XKeys 1-8 with added Press action. 
 - Keyboard compiled using QMK firmware.  
 - Designed with
   - D-Type panel mount openings for changes/additions later without reprinting (e.g., adding Art-net to DMX module, hardware midi, or LTC generator)
   - Extra stand-offs for adding prototype boards for extra design options

## Getting started
- [Bill Of Materials](https://github.com/stagehandshawn/EvoCmdWing/wiki/Bill-of-Materials)

- [Pin Assignments](https://github.com/stagehandshawn/EvoCmdWing/wiki/Pin-Assignments)

- [Wiring Diagrams](https://github.com/stagehandshawn/EvoCmdWing/wiki/Wiring-Diagrams)


## Required Plugins
#### EvoCmdWingMidi

- Lua script `/lua/evocmdwingmidi_main.lua`
- Polls executors and sends fader value, on/on status, and color updates to EvoCmdWing using midi.  
- Tracks sequences set to the XKeys and keeps them synced between page changes and sequence moves.  
- You can set custom Encoder Press actions (toggle is default) in Midi Remotes for more control.  

  - The plugin will create required Midi Remotes automatically if they are not present.  
  - You can create them manually with the plugin and batch change the action for pressing the encoder.  
  - You can change each one seperatly in the Midi Remtoes option as well.  

![midiRemotes](https://raw.githubusercontent.com/stagehandshawn/EvoCmdWing/main/help_files/XKey_MidiRemote_setup.png)


#### [Pro Plugins MidiEncoders](https://www.ma3-pro-plugins.com/midi-encoders)
 - Required for taking Midi input and controlling the attribute encoders.  
 - Use Midi Standard and Relative Mode 3.  
 - Top backlit button used to flip from Inner/Outter encoder usage.  


## Full Instructions comming soon, for now...
 - Check the [Wiki](https://github.com/stagehandshawn/EvoCmdWing/wiki)  
 - The `/help_files` and `/images` folders have some helpful files.  

## Inspiration
 - Check out this project! I used it as a starting point but eventually redesigned it to be closer to the layout of a console and to have more control surfaces, and feedback.
 - https://www.thingiverse.com/thing:6464653
 - Thanks to Nathan_I3 for the inspiration for this project!

## Also check out [EvoFaderWing](https://github.com/stagehandshawn/EvoFaderWing) for a complete at home programming setup.
![evocmdfaderwing](https://raw.githubusercontent.com/stagehandshawn/EvoCmdWing/main/images/v02built/faderwing_cmdwing.png)
#### More to come. Thank you for checking out my project!
