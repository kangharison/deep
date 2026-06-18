# HD-Audio DP-MST Support

> 출처(원문): https://docs.kernel.org/sound/hd-audio/dp-mst.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# HD-Audio DP-MST Support

To support DP MST audio, HD Audio hdmi codec driver introduces virtual pin
and dynamic pcm assignment.

Virtual pin is an extension of per\_pin. The most difference of DP MST
from legacy is that DP MST introduces device entry. Each pin can contain
several device entries. Each device entry behaves as a pin.

As each pin may contain several device entries and each codec may contain
several pins, if we use one pcm per per\_pin, there will be many PCMs.
The new solution is to create a few PCMs and to dynamically bind pcm to
per\_pin. Driver uses spec->dyn\_pcm\_assign flag to indicate whether to use
the new solution.

## PCM

To be added

## Pin Initialization

Each pin may have several device entries (virtual pins). On Intel platform,
the device entries number is dynamically changed. If DP MST hub is connected,
it is in DP MST mode, and the device entries number is 3. Otherwise, the
device entries number is 1.

To simplify the implementation, all the device entries will be initialized
when bootup no matter whether it is in DP MST mode or not.

## Connection list

DP MST reuses connection list code. The code can be reused because
device entries on the same pin have the same connection list.

This means DP MST gets the device entry connection list without the
device entry setting.

## Jack

Presume:
:   * MST must be dyn\_pcm\_assign, and it is acomp (for Intel scenario);
    * NON-MST may or may not be dyn\_pcm\_assign, it can be acomp or !acomp;

So there are the following scenarios:
:   1. MST (&& dyn\_pcm\_assign && acomp)
    2. NON-MST && dyn\_pcm\_assign && acomp
    3. NON-MST && !dyn\_pcm\_assign && !acomp

Below discussion will ignore MST and NON-MST difference as it doesn’t
impact on jack handling too much.

Driver uses `struct hdmi_pcm` pcm[] array in hdmi\_spec and snd\_jack is
a member of hdmi\_pcm. Each pin has one `struct hdmi_pcm` \* pcm pointer.

For !dyn\_pcm\_assign, per\_pin->pcm will assigned to spec->pcm[n] statically.

For dyn\_pcm\_assign, per\_pin->pcm will assigned to spec->pcm[n]
when monitor is hotplugged.

### Build Jack

* dyn\_pcm\_assign

  Will not use hda\_jack but use snd\_jack in spec->pcm\_rec[pcm\_idx].jack directly.
* !dyn\_pcm\_assign

  Use hda\_jack and assign spec->pcm\_rec[pcm\_idx].jack = jack->jack statically.

### Unsolicited Event Enabling

Enable unsolicited event if !acomp.

### Monitor Hotplug Event Handling

* acomp

  `pin_eld_notify()` -> `check_presence_and_report()` -> `hdmi_present_sense()` ->
  `sync_eld_via_acomp()`.

  Use directly [`snd_jack_report()`](../kernel-api/alsa-driver-api.html#c.snd_jack_report "snd_jack_report") on spec->pcm\_rec[pcm\_idx].jack for
  both dyn\_pcm\_assign and !dyn\_pcm\_assign
* !acomp

  `hdmi_unsol_event()` -> `hdmi_intrinsic_event()` -> `check_presence_and_report()` ->
  `hdmi_present_sense()` -> `hdmi_prepsent_sense_via_verbs()`

  Use directly [`snd_jack_report()`](../kernel-api/alsa-driver-api.html#c.snd_jack_report "snd_jack_report") on spec->pcm\_rec[pcm\_idx].jack for dyn\_pcm\_assign.
  Use hda\_jack mechanism to handle jack events.

## Others to be added later
