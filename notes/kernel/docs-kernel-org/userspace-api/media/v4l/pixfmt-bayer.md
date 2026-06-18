# 2.6.Raw Bayer Formats

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/pixfmt-bayer.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 2.6. Raw Bayer Formats

## 2.6.1. Description

The raw Bayer formats are used by image sensors before much if any processing is
performed on the image. The formats contain green, red and blue components, with
alternating lines of red and green, and blue and green pixels in different
orders. See also [the Wikipedia article on Bayer filter](https://en.wikipedia.org/wiki/Bayer_filter).

* [2.6.1.1. V4L2\_PIX\_FMT\_RAW\_CRU10 (‘CR10’), V4L2\_PIX\_FMT\_RAW\_CRU12 (‘CR12’), V4L2\_PIX\_FMT\_RAW\_CRU14 (‘CR14’), V4L2\_PIX\_FMT\_RAW\_CRU20 (‘CR20’)](pixfmt-rawnn-cru.html)
* [2.6.1.2. V4L2\_PIX\_FMT\_SRGGB8 (‘RGGB’), V4L2\_PIX\_FMT\_SGRBG8 (‘GRBG’), V4L2\_PIX\_FMT\_SGBRG8 (‘GBRG’), V4L2\_PIX\_FMT\_SBGGR8 (‘BA81’),](pixfmt-srggb8.html)
* [2.6.1.3. V4L2\_PIX\_FMT\_PISP\_COMP1\_RGGB (‘PC1R’), V4L2\_PIX\_FMT\_PISP\_COMP1\_GRBG (‘PC1G’), V4L2\_PIX\_FMT\_PISP\_COMP1\_GBRG (‘PC1g’), V4L2\_PIX\_FMT\_PISP\_COMP1\_BGGR (‘PC1B), V4L2\_PIX\_FMT\_PISP\_COMP1\_MONO (‘PC1M’), V4L2\_PIX\_FMT\_PISP\_COMP2\_RGGB (‘PC2R’), V4L2\_PIX\_FMT\_PISP\_COMP2\_GRBG (‘PC2G’), V4L2\_PIX\_FMT\_PISP\_COMP2\_GBRG (‘PC2g’), V4L2\_PIX\_FMT\_PISP\_COMP2\_BGGR (‘PC2B), V4L2\_PIX\_FMT\_PISP\_COMP2\_MONO (‘PC2M’)](pixfmt-srggb8-pisp-comp.html)
* [2.6.1.4. V4L2\_PIX\_FMT\_SRGGB10 (‘RG10’), V4L2\_PIX\_FMT\_SGRBG10 (‘BA10’), V4L2\_PIX\_FMT\_SGBRG10 (‘GB10’), V4L2\_PIX\_FMT\_SBGGR10 (‘BG10’),](pixfmt-srggb10.html)
* [2.6.1.5. V4L2\_PIX\_FMT\_SRGGB10P (‘pRAA’), V4L2\_PIX\_FMT\_SGRBG10P (‘pgAA’), V4L2\_PIX\_FMT\_SGBRG10P (‘pGAA’), V4L2\_PIX\_FMT\_SBGGR10P (‘pBAA’),](pixfmt-srggb10p.html)
* [2.6.1.6. V4L2\_PIX\_FMT\_SBGGR10ALAW8 (‘aBA8’), V4L2\_PIX\_FMT\_SGBRG10ALAW8 (‘aGA8’), V4L2\_PIX\_FMT\_SGRBG10ALAW8 (‘agA8’), V4L2\_PIX\_FMT\_SRGGB10ALAW8 (‘aRA8’),](pixfmt-srggb10alaw8.html)
* [2.6.1.7. V4L2\_PIX\_FMT\_SBGGR10DPCM8 (‘bBA8’), V4L2\_PIX\_FMT\_SGBRG10DPCM8 (‘bGA8’), V4L2\_PIX\_FMT\_SGRBG10DPCM8 (‘BD10’), V4L2\_PIX\_FMT\_SRGGB10DPCM8 (‘bRA8’),](pixfmt-srggb10dpcm8.html)
* [2.6.1.8. V4L2\_PIX\_FMT\_IPU3\_SBGGR10 (‘ip3b’), V4L2\_PIX\_FMT\_IPU3\_SGBRG10 (‘ip3g’), V4L2\_PIX\_FMT\_IPU3\_SGRBG10 (‘ip3G’), V4L2\_PIX\_FMT\_IPU3\_SRGGB10 (‘ip3r’)](pixfmt-srggb10-ipu3.html)
* [2.6.1.9. V4L2\_PIX\_FMT\_SRGGB12 (‘RG12’), V4L2\_PIX\_FMT\_SGRBG12 (‘BA12’), V4L2\_PIX\_FMT\_SGBRG12 (‘GB12’), V4L2\_PIX\_FMT\_SBGGR12 (‘BG12’),](pixfmt-srggb12.html)
* [2.6.1.10. V4L2\_PIX\_FMT\_SRGGB12P (‘pRCC’), V4L2\_PIX\_FMT\_SGRBG12P (‘pgCC’), V4L2\_PIX\_FMT\_SGBRG12P (‘pGCC’), V4L2\_PIX\_FMT\_SBGGR12P (‘pBCC’)](pixfmt-srggb12p.html)
* [2.6.1.11. V4L2\_PIX\_FMT\_SRGGB14 (‘RG14’), V4L2\_PIX\_FMT\_SGRBG14 (‘GR14’), V4L2\_PIX\_FMT\_SGBRG14 (‘GB14’), V4L2\_PIX\_FMT\_SBGGR14 (‘BG14’),](pixfmt-srggb14.html)
* [2.6.1.12. V4L2\_PIX\_FMT\_SRGGB14P (‘pREE’), V4L2\_PIX\_FMT\_SGRBG14P (‘pgEE’), V4L2\_PIX\_FMT\_SGBRG14P (‘pGEE’), V4L2\_PIX\_FMT\_SBGGR14P (‘pBEE’),](pixfmt-srggb14p.html)
* [2.6.1.13. V4L2\_PIX\_FMT\_SRGGB16 (‘RG16’), V4L2\_PIX\_FMT\_SGRBG16 (‘GR16’), V4L2\_PIX\_FMT\_SGBRG16 (‘GB16’), V4L2\_PIX\_FMT\_SBGGR16 (‘BYR2’),](pixfmt-srggb16.html)
