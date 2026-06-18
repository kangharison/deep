# Audio Clocking

> 출처(원문): https://docs.kernel.org/sound/soc/clocking.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Audio Clocking

This text describes the audio clocking terms in ASoC and digital audio in
general. Note: Audio clocking can be complex!

## Master Clock

Every audio subsystem is driven by a master clock (sometimes referred to as MCLK
or SYSCLK). This audio master clock can be derived from a number of sources
(e.g. crystal, PLL, CPU clock) and is responsible for producing the correct
audio playback and capture sample rates.

Some master clocks (e.g. PLLs and CPU based clocks) are configurable in that
their speed can be altered by software (depending on the system use and to save
power). Other master clocks are fixed at a set frequency (i.e. crystals).

## DAI Clocks

The Digital Audio Interface is usually driven by a Bit Clock (often referred to
as BCLK). This clock is used to drive the digital audio data across the link
between the codec and CPU.

The DAI also has a frame clock to signal the start of each audio frame. This
clock is sometimes referred to as LRC (left right clock) or FRAME. This clock
runs at exactly the sample rate (LRC = Rate).

Bit Clock can be generated as follows:-

* BCLK = MCLK / x, or
* BCLK = LRC \* x, or
* BCLK = LRC \* Channels \* Word Size

This relationship depends on the codec or SoC CPU in particular. In general
it is best to configure BCLK to the lowest possible speed (depending on your
rate, number of channels and word size) to save on power.

It is also desirable to use the codec (if possible) to drive (or master) the
audio clocks as it usually gives more accurate sample rates than the CPU.

## ASoC provided clock APIs

int snd\_soc\_dai\_set\_sysclk(struct snd\_soc\_dai \*dai, int clk\_id, unsigned int freq, int dir)
:   configure DAI system or master clock.

**Parameters**

`struct snd_soc_dai *dai`
:   DAI

`int clk_id`
:   DAI specific clock ID

`unsigned int freq`
:   new clock frequency in Hz

`int dir`
:   new clock direction (SND\_SOC\_CLOCK\_IN or SND\_SOC\_CLOCK\_OUT)

**Description**

Configures the DAI master (MCLK) or system (SYSCLK) clocking.

int snd\_soc\_dai\_set\_clkdiv(struct snd\_soc\_dai \*dai, int div\_id, int div)
:   configure DAI clock dividers.

**Parameters**

`struct snd_soc_dai *dai`
:   DAI

`int div_id`
:   DAI specific clock divider ID

`int div`
:   new clock divisor.

**Description**

Configures the clock dividers. This is used to derive the best DAI bit and
frame clocks from the system or master clock. It’s best to set the DAI bit
and frame clocks as low as possible to save system power.

int snd\_soc\_dai\_set\_pll(struct snd\_soc\_dai \*dai, int pll\_id, int source, unsigned int freq\_in, unsigned int freq\_out)
:   configure DAI PLL.

**Parameters**

`struct snd_soc_dai *dai`
:   DAI

`int pll_id`
:   DAI specific PLL ID

`int source`
:   DAI specific source for the PLL

`unsigned int freq_in`
:   PLL input clock frequency in Hz

`unsigned int freq_out`
:   requested PLL output clock frequency in Hz

**Description**

Configures and enables PLL to generate output clock based on input clock.

int snd\_soc\_dai\_set\_bclk\_ratio(struct snd\_soc\_dai \*dai, unsigned int ratio)
:   configure BCLK to sample rate ratio.

**Parameters**

`struct snd_soc_dai *dai`
:   DAI

`unsigned int ratio`
:   Ratio of BCLK to Sample rate.

**Description**

Configures the DAI for a preset BCLK to sample rate ratio.
