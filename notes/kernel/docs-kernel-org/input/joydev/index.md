# 6.Linux Joystick support

> 출처(원문): https://docs.kernel.org/input/joydev/index.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 6. Linux Joystick support

Copyright:
:   © 1996-2000 Vojtech Pavlik <[vojtech@ucw.cz](mailto:vojtech%40ucw.cz)> - Sponsored by SuSE

Table of Contents

* [6.1. Introduction](joystick.html)
* [6.2. Usage](joystick.html#usage)
  + [6.2.1. Utilities](joystick.html#utilities)
  + [6.2.2. Device nodes](joystick.html#device-nodes)
  + [6.2.3. Modules needed](joystick.html#modules-needed)
  + [6.2.4. Verifying that it works](joystick.html#verifying-that-it-works)
  + [6.2.5. Calibration](joystick.html#calibration)
* [6.3. Hardware-specific driver information](joystick.html#hardware-specific-driver-information)
  + [6.3.1. Analog joysticks](joystick.html#analog-joysticks)
  + [6.3.2. Microsoft SideWinder joysticks](joystick.html#microsoft-sidewinder-joysticks)
  + [6.3.3. Logitech ADI devices](joystick.html#logitech-adi-devices)
  + [6.3.4. Gravis GrIP](joystick.html#gravis-grip)
  + [6.3.5. FPGaming A3D and MadCatz A3D](joystick.html#fpgaming-a3d-and-madcatz-a3d)
  + [6.3.6. ThrustMaster DirectConnect (BSP)](joystick.html#thrustmaster-directconnect-bsp)
  + [6.3.7. Creative Labs Blaster](joystick.html#creative-labs-blaster)
  + [6.3.8. Genius Digital joysticks](joystick.html#genius-digital-joysticks)
  + [6.3.9. InterAct Digital joysticks](joystick.html#interact-digital-joysticks)
  + [6.3.10. PDPI Lightning 4 gamecards](joystick.html#pdpi-lightning-4-gamecards)
  + [6.3.11. Trident 4DWave / Aureal Vortex](joystick.html#trident-4dwave-aureal-vortex)
  + [6.3.12. Crystal SoundFusion](joystick.html#crystal-soundfusion)
  + [6.3.13. SoundBlaster Live!](joystick.html#soundblaster-live)
  + [6.3.14. SoundBlaster 64 and 128 - ES1370 and ES1371, ESS Solo1 and S3 SonicVibes](joystick.html#soundblaster-64-and-128-es1370-and-es1371-ess-solo1-and-s3-sonicvibes)
  + [6.3.15. Amiga](joystick.html#amiga)
  + [6.3.16. Game console and 8-bit pads and joysticks](joystick.html#game-console-and-8-bit-pads-and-joysticks)
  + [6.3.17. SpaceTec/LabTec devices](joystick.html#spacetec-labtec-devices)
  + [6.3.18. Logitech SWIFT devices](joystick.html#logitech-swift-devices)
  + [6.3.19. Magellan / Space Mouse](joystick.html#magellan-space-mouse)
  + [6.3.20. I-Force devices](joystick.html#i-force-devices)
  + [6.3.21. Gravis Stinger gamepad](joystick.html#gravis-stinger-gamepad)
* [6.4. Troubleshooting](joystick.html#troubleshooting)
* [6.5. FAQ](joystick.html#faq)
* [6.6. Programming Interface](joystick-api.html)
  + [6.6.1. Introduction](joystick-api.html#introduction)
  + [6.6.2. Initialization](joystick-api.html#initialization)
  + [6.6.3. Event Reading](joystick-api.html#event-reading)
    - [6.6.3.1. js\_event.type](joystick-api.html#js-event-type)
    - [6.6.3.2. js\_event.number](joystick-api.html#js-event-number)
    - [6.6.3.3. js\_event.value](joystick-api.html#js-event-value)
    - [6.6.3.4. js\_event.time](joystick-api.html#js-event-time)
  + [6.6.4. Reading](joystick-api.html#reading)
    - [6.6.4.1. O\_NONBLOCK](joystick-api.html#o-nonblock)
  + [6.6.5. IOCTLs](joystick-api.html#ioctls)
    - [6.6.5.1. JSIOGCVERSION](joystick-api.html#jsiogcversion)
    - [6.6.5.2. JSIOCGNAME](joystick-api.html#jsiocgname)
    - [6.6.5.3. JSIOC[SG]CORR](joystick-api.html#jsioc-sg-corr)
  + [6.6.6. Backward compatibility](joystick-api.html#backward-compatibility)
  + [6.6.7. Final Notes](joystick-api.html#final-notes)
