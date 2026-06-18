# 7.Arm Mali-C55 ISP driver

> 출처(원문): https://docs.kernel.org/userspace-api/media/drivers/mali-c55.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 7. Arm Mali-C55 ISP driver

The Arm Mali-C55 ISP driver implements a single driver-specific control:

`V4L2_CID_MALI_C55_CAPABILITIES (bitmask)`
:   Detail the capabilities of the ISP by giving detail about the fitted blocks.

    Bitmask meaning definitions

    | Bit | Macro | Meaning |
    | --- | --- | --- |
    | 0 | MALI\_C55\_PONG | Pong configuration space is fitted in the ISP |
    | 1 | MALI\_C55\_WDR | WDR Framestitch, offset and gain is fitted in the ISP |
    | 2 | MALI\_C55\_COMPRESSION | Temper compression is fitted in the ISP |
    | 3 | MALI\_C55\_TEMPER | Temper is fitted in the ISP |
    | 4 | MALI\_C55\_SINTER\_LITE | Sinter Lite is fitted in the ISP instead of the full Sinter version |
    | 5 | MALI\_C55\_SINTER | Sinter is fitted in the ISP |
    | 6 | MALI\_C55\_IRIDIX\_LTM | Iridix local tone mappine is fitted in the ISP |
    | 7 | MALI\_C55\_IRIDIX\_GTM | Iridix global tone mapping is fitted in the ISP |
    | 8 | MALI\_C55\_CNR | Colour noise reduction is fitted in the ISP |
    | 9 | MALI\_C55\_FRSCALER | The full resolution pipe scaler is fitted in the ISP |
    | 10 | MALI\_C55\_DS\_PIPE | The downscale pipe is fitted in the ISP |

    The Mali-C55 ISP can be configured in a number of ways to include or exclude
    blocks which may not be necessary. This control provides a way for the
    driver to communicate to userspace which of the blocks are fitted in the
    design.
