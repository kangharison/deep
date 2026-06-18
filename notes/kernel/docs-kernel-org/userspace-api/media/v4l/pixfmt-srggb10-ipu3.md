# 2.6.1.8.V4L2_PIX_FMT_IPU3_SBGGR10 (‘ip3b’), V4L2_PIX_FMT_IPU3_SGBRG10 (‘ip3g’), V4L2_PIX_FMT_IPU3_SGRBG10 (‘ip3G’), V4L2_PIX_FMT_IPU3_SRGGB10 (‘ip3r’)

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-srggb10-ipu3.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.6.1.8. V4L2\_PIX\_FMT\_IPU3\_SBGGR10 (‘ip3b’), V4L2\_PIX\_FMT\_IPU3\_SGBRG10 (‘ip3g’), V4L2\_PIX\_FMT\_IPU3\_SGRBG10 (‘ip3G’), V4L2\_PIX\_FMT\_IPU3\_SRGGB10 (‘ip3r’)

## 2.6.1.8.1. 10-bit Bayer formats

### 2.6.1.8.1.1. Description

These four pixel formats are used by Intel IPU3 driver, they are raw
sRGB / Bayer formats with 10 bits per sample with every 25 pixels packed
to 32 bytes leaving 6 most significant bits padding in the last byte.
The format is little endian.

In other respects this format is similar to [V4L2\_PIX\_FMT\_SRGGB10 (‘RG10’), V4L2\_PIX\_FMT\_SGRBG10 (‘BA10’), V4L2\_PIX\_FMT\_SGBRG10 (‘GB10’), V4L2\_PIX\_FMT\_SBGGR10 (‘BG10’),](pixfmt-srggb10.html#v4l2-pix-fmt-srggb10).
Below is an example of a small image in V4L2\_PIX\_FMT\_IPU3\_SBGGR10 format.

**Byte Order.**
Each cell is one byte.

|  |  |  |  |  |
| --- | --- | --- | --- | --- |
| start + 0: | B0000low | G0001low(bits 7--2)  B0000high(bits 1--0) | B0002low(bits 7--4)  G0001high(bits 3--0) | G0003low(bits 7--6)  B0002high(bits 5--0) |
| start + 4: | G0003high | B0004low | G0005low(bits 7--2)  B0004high(bits 1--0) | B0006low(bits 7--4)  G0005high(bits 3--0) |
| start + 8: | G0007low(bits 7--6)  B0006high(bits 5--0) | G0007high | B0008low | G0009low(bits 7--2)  B0008high(bits 1--0) |
| start + 12: | B0010low(bits 7--4)  G0009high(bits 3--0) | G0011low(bits 7--6)  B0010high(bits 5--0) | G0011high | B0012low |
| start + 16: | G0013low(bits 7--2)  B0012high(bits 1--0) | B0014low(bits 7--4)  G0013high(bits 3--0) | G0015low(bits 7--6)  B0014high(bits 5--0) | G0015high |
| start + 20 | B0016low | G0017low(bits 7--2)  B0016high(bits 1--0) | B0018low(bits 7--4)  G0017high(bits 3--0) | G0019low(bits 7--6)  B0018high(bits 5--0) |
| start + 24: | G0019high | B0020low | G0021low(bits 7--2)  B0020high(bits 1--0) | B0022low(bits 7--4)  G0021high(bits 3--0) |
| start + 28: | G0023low(bits 7--6)  B0022high(bits 5--0) | G0023high | B0024low | B0024high(bits 1--0) |
| start + 32: | G0100low | R0101low(bits 7--2)  G0100high(bits 1--0) | G0102low(bits 7--4)  R0101high(bits 3--0) | R0103low(bits 7--6)  G0102high(bits 5--0) |
| start + 36: | R0103high | G0104low | R0105low(bits 7--2)  G0104high(bits 1--0) | G0106low(bits 7--4)  R0105high(bits 3--0) |
| start + 40: | R0107low(bits 7--6)  G0106high(bits 5--0) | R0107high | G0108low | R0109low(bits 7--2)  G0108high(bits 1--0) |
| start + 44: | G0110low(bits 7--4)  R0109high(bits 3--0) | R0111low(bits 7--6)  G0110high(bits 5--0) | R0111high | G0112low |
| start + 48: | R0113low(bits 7--2)  G0112high(bits 1--0) | G0114low(bits 7--4)  R0113high(bits 3--0) | R0115low(bits 7--6)  G0114high(bits 5--0) | R0115high |
| start + 52: | G0116low | R0117low(bits 7--2)  G0116high(bits 1--0) | G0118low(bits 7--4)  R0117high(bits 3--0) | R0119low(bits 7--6)  G0118high(bits 5--0) |
| start + 56: | R0119high | G0120low | R0121low(bits 7--2)  G0120high(bits 1--0) | G0122low(bits 7--4)  R0121high(bits 3--0) |
| start + 60: | R0123low(bits 7--6)  G0122high(bits 5--0) | R0123high | G0124low | G0124high(bits 1--0) |
| start + 64: | B0200low | G0201low(bits 7--2)  B0200high(bits 1--0) | B0202low(bits 7--4)  G0201high(bits 3--0) | G0203low(bits 7--6)  B0202high(bits 5--0) |
| start + 68: | G0203high | B0204low | G0205low(bits 7--2)  B0204high(bits 1--0) | B0206low(bits 7--4)  G0205high(bits 3--0) |
| start + 72: | G0207low(bits 7--6)  B0206high(bits 5--0) | G0207high | B0208low | G0209low(bits 7--2)  B0208high(bits 1--0) |
| start + 76: | B0210low(bits 7--4)  G0209high(bits 3--0) | G0211low(bits 7--6)  B0210high(bits 5--0) | G0211high | B0212low |
| start + 80: | G0213low(bits 7--2)  B0212high(bits 1--0) | B0214low(bits 7--4)  G0213high(bits 3--0) | G0215low(bits 7--6)  B0214high(bits 5--0) | G0215high |
| start + 84: | B0216low | G0217low(bits 7--2)  B0216high(bits 1--0) | B0218low(bits 7--4)  G0217high(bits 3--0) | G0219low(bits 7--6)  B0218high(bits 5--0) |
| start + 88: | G0219high | B0220low | G0221low(bits 7--2)  B0220high(bits 1--0) | B0222low(bits 7--4)  G0221high(bits 3--0) |
| start + 92: | G0223low(bits 7--6)  B0222high(bits 5--0) | G0223high | B0224low | B0224high(bits 1--0) |
| start + 96: | G0300low | R0301low(bits 7--2)  G0300high(bits 1--0) | G0302low(bits 7--4)  R0301high(bits 3--0) | R0303low(bits 7--6)  G0302high(bits 5--0) |
| start + 100: | R0303high | G0304low | R0305low(bits 7--2)  G0304high(bits 1--0) | G0306low(bits 7--4)  R0305high(bits 3--0) |
| start + 104: | R0307low(bits 7--6)  G0306high(bits 5--0) | R0307high | G0308low | R0309low(bits 7--2)  G0308high(bits 1--0) |
| start + 108: | G0310low(bits 7--4)  R0309high(bits 3--0) | R0311low(bits 7--6)  G0310high(bits 5--0) | R0311high | G0312low |
| start + 112: | R0313low(bits 7--2)  G0312high(bits 1--0) | G0314low(bits 7--4)  R0313high(bits 3--0) | R0315low(bits 7--6)  G0314high(bits 5--0) | R0315high |
| start + 116: | G0316low | R0317low(bits 7--2)  G0316high(bits 1--0) | G0318low(bits 7--4)  R0317high(bits 3--0) | R0319low(bits 7--6)  G0318high(bits 5--0) |
| start + 120: | R0319high | G0320low | R0321low(bits 7--2)  G0320high(bits 1--0) | G0322low(bits 7--4)  R0321high(bits 3--0) |
| start + 124: | R0323low(bits 7--6)  G0322high(bits 5--0) | R0323high | G0324low | G0324high(bits 1--0) |
