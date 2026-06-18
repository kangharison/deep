# VIA82xx mixer

> 출처(원문): https://docs.kernel.org/sound/cards/via82xx-mixer.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# VIA82xx mixer

On many VIA82xx boards, the `Input Source Select` mixer control does not work.
Setting it to `Input2` on such boards will cause recording to hang, or fail
with EIO (input/output error) via OSS emulation. This control should be left
at `Input1` for such cards.
