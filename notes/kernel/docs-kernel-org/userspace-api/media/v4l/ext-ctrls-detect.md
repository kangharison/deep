# 1.22.Detect Control Reference

> 출처(원문): https://docs.kernel.org/userspace-api/media/v4l/ext-ctrls-detect.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 1.22. Detect Control Reference

The Detect class includes controls for common features of various motion
or object detection capable devices.

## 1.22.1. Detect Control IDs

`V4L2_CID_DETECT_CLASS (class)`
:   The Detect class descriptor. Calling
    [ioctls VIDIOC\_QUERYCTRL, VIDIOC\_QUERY\_EXT\_CTRL and VIDIOC\_QUERYMENU](vidioc-queryctrl.html#vidioc-queryctrl) for this control will
    return a description of this control class.

`V4L2_CID_DETECT_MD_MODE (menu)`
:   Sets the motion detection mode.

|  |  |
| --- | --- |
| `V4L2_DETECT_MD_MODE_DISABLED` | Disable motion detection. |
| `V4L2_DETECT_MD_MODE_GLOBAL` | Use a single motion detection threshold. |
| `V4L2_DETECT_MD_MODE_THRESHOLD_GRID` | The image is divided into a grid, each cell with its own motion detection threshold. These thresholds are set through the `V4L2_CID_DETECT_MD_THRESHOLD_GRID` matrix control. |
| `V4L2_DETECT_MD_MODE_REGION_GRID` | The image is divided into a grid, each cell with its own region value that specifies which per-region motion detection thresholds should be used. Each region has its own thresholds. How these per-region thresholds are set up is driver-specific. The region values for the grid are set through the `V4L2_CID_DETECT_MD_REGION_GRID` matrix control. |

`V4L2_CID_DETECT_MD_GLOBAL_THRESHOLD (integer)`
:   Sets the global motion detection threshold to be used with the
    `V4L2_DETECT_MD_MODE_GLOBAL` motion detection mode.

`V4L2_CID_DETECT_MD_THRESHOLD_GRID (__u16 matrix)`
:   Sets the motion detection thresholds for each cell in the grid. To
    be used with the `V4L2_DETECT_MD_MODE_THRESHOLD_GRID` motion
    detection mode. Matrix element (0, 0) represents the cell at the
    top-left of the grid.

`V4L2_CID_DETECT_MD_REGION_GRID (__u8 matrix)`
:   Sets the motion detection region value for each cell in the grid. To
    be used with the `V4L2_DETECT_MD_MODE_REGION_GRID` motion
    detection mode. Matrix element (0, 0) represents the cell at the
    top-left of the grid.
