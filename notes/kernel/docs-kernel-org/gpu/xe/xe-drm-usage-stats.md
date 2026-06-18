# Xe DRM client usage stats implementation

> 출처(원문): https://docs.kernel.org/gpu/xe/xe-drm-usage-stats.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Xe DRM client usage stats implementation

The drm/xe driver implements the DRM client usage stats specification as
documented in [DRM client usage stats](../drm-usage-stats.html#drm-client-usage-stats).

Example of the output showing the implemented key value pairs and entirety of
the currently possible format options:

```
pos:    0
flags:  0100002
mnt_id: 26
ino:    685
drm-driver:     xe
drm-client-id:  3
drm-pdev:       0000:03:00.0
drm-total-system:       0
drm-shared-system:      0
drm-active-system:      0
drm-resident-system:    0
drm-purgeable-system:   0
drm-total-gtt:  192 KiB
drm-shared-gtt: 0
drm-active-gtt: 0
drm-resident-gtt:       192 KiB
drm-total-vram0:        23992 KiB
drm-shared-vram0:       16 MiB
drm-active-vram0:       0
drm-resident-vram0:     23992 KiB
drm-total-stolen:       0
drm-shared-stolen:      0
drm-active-stolen:      0
drm-resident-stolen:    0
drm-cycles-rcs: 28257900
drm-total-cycles-rcs:   7655183225
drm-cycles-bcs: 0
drm-total-cycles-bcs:   7655183225
drm-cycles-vcs: 0
drm-total-cycles-vcs:   7655183225
drm-engine-capacity-vcs:        2
drm-cycles-vecs:        0
drm-total-cycles-vecs:  7655183225
drm-engine-capacity-vecs:       2
drm-cycles-ccs: 0
drm-total-cycles-ccs:   7655183225
drm-engine-capacity-ccs:        4
```

Possible drm-cycles- key names are: rcs, ccs, bcs, vcs, vecs and
“other”.
