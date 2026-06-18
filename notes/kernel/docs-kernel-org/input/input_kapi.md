# Linux Input Subsystem kernel API

> 출처(원문): https://docs.kernel.org/input/input_kapi.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Linux Input Subsystem kernel API

Table of Contents

* [1. Creating an input device driver](input-programming.html)
  + [1.1. The simplest example](input-programming.html#the-simplest-example)
  + [1.2. What the example does](input-programming.html#what-the-example-does)
  + [1.3. dev->open() and dev->close()](input-programming.html#dev-open-and-dev-close)
  + [1.4. Inhibiting input devices](input-programming.html#inhibiting-input-devices)
  + [1.5. Basic event types](input-programming.html#basic-event-types)
  + [1.6. BITS\_TO\_LONGS(), BIT\_WORD(), BIT\_MASK()](input-programming.html#bits-to-longs-bit-word-bit-mask)
  + [1.7. The id\* and name fields](input-programming.html#the-id-and-name-fields)
  + [1.8. The keycode, keycodemax, keycodesize fields](input-programming.html#the-keycode-keycodemax-keycodesize-fields)
  + [1.9. dev->getkeycode() and dev->setkeycode()](input-programming.html#dev-getkeycode-and-dev-setkeycode)
  + [1.10. Key autorepeat](input-programming.html#key-autorepeat)
  + [1.11. Other event types, handling output events](input-programming.html#other-event-types-handling-output-events)
  + [1.12. Polled input devices](input-programming.html#polled-input-devices)
* [2. Programming gameport drivers](gameport-programming.html)
  + [2.1. A basic classic gameport](gameport-programming.html#a-basic-classic-gameport)
  + [2.2. Memory mapped gameport](gameport-programming.html#memory-mapped-gameport)
  + [2.3. Cooked mode gameport](gameport-programming.html#cooked-mode-gameport)
  + [2.4. More complex gameports](gameport-programming.html#more-complex-gameports)
  + [2.5. Unregistering a gameport](gameport-programming.html#unregistering-a-gameport)
  + [2.6. The gameport structure](gameport-programming.html#the-gameport-structure)
* [3. Keyboard notifier](notifier.html)
