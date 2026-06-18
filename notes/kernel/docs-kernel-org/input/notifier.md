# 3.Keyboard notifier

> 출처(원문): https://docs.kernel.org/input/notifier.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3. Keyboard notifier

One can use register\_keyboard\_notifier to get called back on keyboard
events (see `kbd_keycode()` function for details). The passed structure is
keyboard\_notifier\_param (see <linux/keyboard.h>):

* ‘vc’ always provide the VC for which the keyboard event applies;
* ‘down’ is 1 for a key press event, 0 for a key release;
* ‘shift’ is the current modifier state, mask bit indexes are KG\_\*;
* ‘ledstate’ is the current LED state;
* ‘value’ depends on the type of event.
* KBD\_KEYCODE events are always sent before other events, value is the keycode.
* KBD\_UNBOUND\_KEYCODE events are sent if the keycode is not bound to a keysym.
  value is the keycode.
* KBD\_UNICODE events are sent if the keycode -> keysym translation produced a
  unicode character. value is the unicode value.
* KBD\_KEYSYM events are sent if the keycode -> keysym translation produced a
  non-unicode character. value is the keysym.
* KBD\_POST\_KEYSYM events are sent after the treatment of non-unicode keysyms.
  That permits one to inspect the resulting LEDs for instance.

For each kind of event but the last, the callback may return NOTIFY\_STOP in
order to “eat” the event: the notify loop is stopped and the keyboard event is
dropped.

In a rough C snippet, we have:

```
kbd_keycode(keycode) {
    ...
    params.value = keycode;
    if (notifier_call_chain(KBD_KEYCODE,&params) == NOTIFY_STOP)
        || !bound) {
            notifier_call_chain(KBD_UNBOUND_KEYCODE,&params);
            return;
    }

    if (unicode) {
            param.value = unicode;
            if (notifier_call_chain(KBD_UNICODE,&params) == NOTIFY_STOP)
                    return;
            emit unicode;
            return;
    }

    params.value = keysym;
    if (notifier_call_chain(KBD_KEYSYM,&params) == NOTIFY_STOP)
            return;
    apply keysym;
    notifier_call_chain(KBD_POST_KEYSYM,&params);
}
```

Note

This notifier is usually called from interrupt context.
