# 10.1.10.Cropping and Scaling algorithm, used in the sh_mobile_ceu_camera driver

> 출처(원문): https://docs.kernel.org/driver-api/media/drivers/sh_mobile_ceu_camera.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 10.1.10. Cropping and Scaling algorithm, used in the sh\_mobile\_ceu\_camera driver

Author: Guennadi Liakhovetski <[g.liakhovetski@gmx.de](mailto:g.liakhovetski%40gmx.de)>

## 10.1.10.1. Terminology

sensor scales: horizontal and vertical scales, configured by the sensor driver
host scales: -”- host driver
combined scales: sensor\_scale \* host\_scale

## 10.1.10.2. Generic scaling / cropping scheme

```
-1--
|
-2-- -\
|      --\
|         --\
+-5-- .      -- -3-- -\
|      `...            -\
|          `... -4-- .   - -7..
|                     `.
|                       `. .6--
|
|                        . .6'-
|                      .´
|           ... -4'- .´
|       ...´             - -7'.
+-5'- .´               -/
|            -- -3'- -/
|         --/
|      --/
-2'- -/
|
|
-1'-
```

In the above chart minuses and slashes represent “real” data amounts, points and
accents represent “useful” data, basically, CEU scaled and cropped output,
mapped back onto the client’s source plane.

Such a configuration can be produced by user requests:

S\_CROP(left / top = (5) - (1), width / height = (5’) - (5))
S\_FMT(width / height = (6’) - (6))

Here:

(1) to (1’) - whole max width or height
(1) to (2) - sensor cropped left or top
(2) to (2’) - sensor cropped width or height
(3) to (3’) - sensor scale
(3) to (4) - CEU cropped left or top
(4) to (4’) - CEU cropped width or height
(5) to (5’) - reverse sensor scale applied to CEU cropped width or height
(2) to (5) - reverse sensor scale applied to CEU cropped left or top
(6) to (6’) - CEU scale - user window

## 10.1.10.3. S\_FMT

Do not touch input rectangle - it is already optimal.

1. Calculate current sensor scales:

   > scale\_s = ((2’) - (2)) / ((3’) - (3))

2. Calculate “effective” input crop (sensor subwindow) - CEU crop scaled back at
current sensor scales onto input window - this is user S\_CROP:

> width\_u = (5’) - (5) = ((4’) - (4)) \* scale\_s

3. Calculate new combined scales from “effective” input window to requested user
window:

> scale\_comb = width\_u / ((6’) - (6))

4. Calculate sensor output window by applying combined scales to real input
window:

> width\_s\_out = ((7’) - (7)) = ((2’) - (2)) / scale\_comb

5. Apply iterative sensor S\_FMT for sensor output window.

   > subdev->video\_ops->s\_fmt(.width = width\_s\_out)
6. Retrieve sensor output window (g\_fmt)
7. Calculate new sensor scales:

   > scale\_s\_new = ((3’)\_new - (3)\_new) / ((2’) - (2))

8. Calculate new CEU crop - apply sensor scales to previously calculated
“effective” crop:

> width\_ceu = (4’)\_new - (4)\_new = width\_u / scale\_s\_new
> left\_ceu = (4)\_new - (3)\_new = ((5) - (2)) / scale\_s\_new

9. Use CEU cropping to crop to the new window:

   > ceu\_crop(.width = width\_ceu, .left = left\_ceu)
10. Use CEU scaling to scale to the requested user window:

    > scale\_ceu = width\_ceu / width

## 10.1.10.4. S\_CROP

The [V4L2 crop API](../../../userspace-api/media/v4l/crop.html#crop-scale) says:

“...specification does not define an origin or units. However by convention
drivers should horizontally count unscaled samples relative to 0H.”

We choose to follow the advise and interpret cropping units as client input
pixels.

Cropping is performed in the following 6 steps:

1. Request exactly user rectangle from the sensor.
2. If smaller - iterate until a larger one is obtained. Result: sensor cropped
   to 2 : 2’, target crop 5 : 5’, current output format 6’ - 6.
3. In the previous step the sensor has tried to preserve its output frame as
   good as possible, but it could have changed. Retrieve it again.
4. Sensor scaled to 3 : 3’. Sensor’s scale is (2’ - 2) / (3’ - 3). Calculate
   intermediate window: 4’ - 4 = (5’ - 5) \* (3’ - 3) / (2’ - 2)
5. Calculate and apply host scale = (6’ - 6) / (4’ - 4)
6. Calculate and apply host crop: 6 - 7 = (5 - 2) \* (6’ - 6) / (5’ - 5)
