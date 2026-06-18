# 3.25.V4L2 rect helper functions

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-rect.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.25. V4L2 rect helper functions

void v4l2\_rect\_set\_size\_to(struct v4l2\_rect \*r, const struct v4l2\_rect \*size)
:   copy the width/height values.

**Parameters**

`struct v4l2_rect *r`
:   rect whose width and height fields will be set

`const struct v4l2_rect *size`
:   rect containing the width and height fields you need.

void v4l2\_rect\_set\_min\_size(struct v4l2\_rect \*r, const struct v4l2\_rect \*min\_size)
:   width and height of r should be >= min\_size.

**Parameters**

`struct v4l2_rect *r`
:   rect whose width and height will be modified

`const struct v4l2_rect *min_size`
:   rect containing the minimal width and height

void v4l2\_rect\_set\_max\_size(struct v4l2\_rect \*r, const struct v4l2\_rect \*max\_size)
:   width and height of r should be <= max\_size

**Parameters**

`struct v4l2_rect *r`
:   rect whose width and height will be modified

`const struct v4l2_rect *max_size`
:   rect containing the maximum width and height

void v4l2\_rect\_map\_inside(struct v4l2\_rect \*r, const struct v4l2\_rect \*boundary)
:   r should be inside boundary.

**Parameters**

`struct v4l2_rect *r`
:   rect that will be modified

`const struct v4l2_rect *boundary`
:   rect containing the boundary for **r**

bool v4l2\_rect\_same\_size(const struct v4l2\_rect \*r1, const struct v4l2\_rect \*r2)
:   return true if r1 has the same size as r2

**Parameters**

`const struct v4l2_rect *r1`
:   rectangle.

`const struct v4l2_rect *r2`
:   rectangle.

**Description**

Return true if both rectangles have the same size.

bool v4l2\_rect\_same\_position(const struct v4l2\_rect \*r1, const struct v4l2\_rect \*r2)
:   return true if r1 has the same position as r2

**Parameters**

`const struct v4l2_rect *r1`
:   rectangle.

`const struct v4l2_rect *r2`
:   rectangle.

**Description**

Return true if both rectangles have the same position

bool v4l2\_rect\_equal(const struct v4l2\_rect \*r1, const struct v4l2\_rect \*r2)
:   return true if r1 equals r2

**Parameters**

`const struct v4l2_rect *r1`
:   rectangle.

`const struct v4l2_rect *r2`
:   rectangle.

**Description**

Return true if both rectangles have the same size and position.

void v4l2\_rect\_intersect(struct v4l2\_rect \*r, const struct v4l2\_rect \*r1, const struct v4l2\_rect \*r2)
:   calculate the intersection of two rects.

**Parameters**

`struct v4l2_rect *r`
:   intersection of **r1** and **r2**.

`const struct v4l2_rect *r1`
:   rectangle.

`const struct v4l2_rect *r2`
:   rectangle.

void v4l2\_rect\_scale(struct v4l2\_rect \*r, const struct v4l2\_rect \*from, const struct v4l2\_rect \*to)
:   scale rect r by to/from

**Parameters**

`struct v4l2_rect *r`
:   rect to be scaled.

`const struct v4l2_rect *from`
:   from rectangle.

`const struct v4l2_rect *to`
:   to rectangle.

**Description**

This scales rectangle **r** horizontally by **to->width** / **from->width** and
vertically by **to->height** / **from->height**.

Typically **r** is a rectangle inside **from** and you want the rectangle as
it would appear after scaling **from** to **to**. So the resulting **r** will
be the scaled rectangle inside **to**.

bool v4l2\_rect\_overlap(const struct v4l2\_rect \*r1, const struct v4l2\_rect \*r2)
:   do r1 and r2 overlap?

**Parameters**

`const struct v4l2_rect *r1`
:   rectangle.

`const struct v4l2_rect *r2`
:   rectangle.

**Description**

Returns true if **r1** and **r2** overlap.

bool v4l2\_rect\_enclosed(struct v4l2\_rect \*r1, struct v4l2\_rect \*r2)
:   is r1 enclosed in r2?

**Parameters**

`struct v4l2_rect *r1`
:   rectangle.

`struct v4l2_rect *r2`
:   rectangle.

**Description**

Returns true if **r1** is enclosed in **r2**.
