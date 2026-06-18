# Frame Buffer Library

> 출처(원문): https://docs.kernel.org/driver-api/frame-buffer.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# Frame Buffer Library

The frame buffer drivers depend heavily on four data structures. These
structures are declared in include/linux/fb.h. They are fb\_info,
fb\_var\_screeninfo, fb\_fix\_screeninfo and fb\_monospecs. The last
three can be made available to and from userland.

fb\_info defines the current state of a particular video card. Inside
fb\_info, there exists a fb\_ops structure which is a collection of
needed functions to make fbdev and fbcon work. fb\_info is only visible
to the kernel.

fb\_var\_screeninfo is used to describe the features of a video card
that are user defined. With fb\_var\_screeninfo, things such as depth
and the resolution may be defined.

The next structure is fb\_fix\_screeninfo. This defines the properties
of a card that are created when a mode is set and can’t be changed
otherwise. A good example of this is the start of the frame buffer
memory. This “locks” the address of the frame buffer memory, so that it
cannot be changed or moved.

The last structure is fb\_monospecs. In the old API, there was little
importance for fb\_monospecs. This allowed for forbidden things such as
setting a mode of 800x600 on a fix frequency monitor. With the new API,
fb\_monospecs prevents such things, and if used correctly, can prevent a
monitor from being cooked. fb\_monospecs will not be useful until
kernels 2.5.x.

## Frame Buffer Memory

int register\_framebuffer(struct [fb\_info](#c.register_framebuffer "fb_info") \*fb\_info)
:   registers a frame buffer device

**Parameters**

`struct fb_info *fb_info`
:   frame buffer info structure

**Description**

> Registers a frame buffer device **fb\_info**.
>
> Returns negative errno on error, or zero for success.

void unregister\_framebuffer(struct [fb\_info](#c.unregister_framebuffer "fb_info") \*fb\_info)
:   releases a frame buffer device

**Parameters**

`struct fb_info *fb_info`
:   frame buffer info structure

**Description**

> Unregisters a frame buffer device **fb\_info**.
>
> Returns negative errno on error, or zero for success.
>
> This function will also notify the framebuffer console
> to release the driver.
>
> This is meant to be called within a driver’s [`module_exit()`](basics.html#c.module_exit "module_exit")
> function. If this is called outside [`module_exit()`](basics.html#c.module_exit "module_exit"), ensure
> that the driver implements `fb_open()` and `fb_release()` to
> check that no processes are using the device.

int devm\_register\_framebuffer(struct [device](infrastructure.html#c.device "device") \*dev, struct [fb\_info](#c.devm_register_framebuffer "fb_info") \*fb\_info)
:   resource-managed frame buffer device registration

**Parameters**

`struct device *dev`
:   device the framebuffer belongs to

`struct fb_info *fb_info`
:   frame buffer info structure

**Description**

> Registers a frame buffer device **fb\_info** to device **dev**.
>
> Returns negative errno on error, or zero for success.

void fb\_set\_suspend(struct fb\_info \*info, int state)
:   low level driver signals suspend

**Parameters**

`struct fb_info *info`
:   framebuffer affected

`int state`
:   0 = resuming, !=0 = suspending

**Description**

> This is meant to be used by low level drivers to
> signal suspend/resume to the core & clients.
> It must be called with the console semaphore held

## Frame Buffer Colormap

void fb\_dealloc\_cmap(struct fb\_cmap \*cmap)
:   deallocate a colormap

**Parameters**

`struct fb_cmap *cmap`
:   frame buffer colormap structure

**Description**

> Deallocates a colormap that was previously allocated with
> `fb_alloc_cmap()`.

int fb\_copy\_cmap(const struct fb\_cmap \*from, struct fb\_cmap \*to)
:   copy a colormap

**Parameters**

`const struct fb_cmap *from`
:   frame buffer colormap structure

`struct fb_cmap *to`
:   frame buffer colormap structure

**Description**

> Copy contents of colormap from **from** to **to**.

int fb\_set\_cmap(struct fb\_cmap \*cmap, struct fb\_info \*info)
:   set the colormap

**Parameters**

`struct fb_cmap *cmap`
:   frame buffer colormap structure

`struct fb_info *info`
:   frame buffer info structure

**Description**

> Sets the colormap **cmap** for a screen of device **info**.
>
> Returns negative errno on error, or zero on success.

const struct fb\_cmap \*fb\_default\_cmap(int len)
:   get default colormap

**Parameters**

`int len`
:   size of palette for a depth

**Description**

> Gets the default colormap for a specific screen depth. **len**
> is the size of the palette for a particular screen depth.
>
> Returns pointer to a frame buffer colormap structure.

void fb\_invert\_cmaps(void)
:   invert all defaults colormaps

**Parameters**

`void`
:   no arguments

**Description**

> Invert all default colormaps.

## Frame Buffer Video Mode Database

int fb\_try\_mode(struct fb\_var\_screeninfo \*var, struct fb\_info \*info, const struct fb\_videomode \*mode, unsigned int bpp)
:   test a video mode

**Parameters**

`struct fb_var_screeninfo *var`
:   frame buffer user defined part of display

`struct fb_info *info`
:   frame buffer info structure

`const struct fb_videomode *mode`
:   frame buffer video mode structure

`unsigned int bpp`
:   color depth in bits per pixel

**Description**

> Tries a video mode to test it’s validity for device **info**.
>
> Returns 1 on success.

void fb\_delete\_videomode(const struct fb\_videomode \*mode, struct list\_head \*head)
:   removed videomode entry from modelist

**Parameters**

`const struct fb_videomode *mode`
:   videomode to remove

`struct list_head *head`
:   `struct list_head` of modelist

**NOTES**

Will remove all matching mode entries

int fb\_find\_mode(struct fb\_var\_screeninfo \*var, struct fb\_info \*info, const char \*mode\_option, const struct fb\_videomode \*db, unsigned int dbsize, const struct fb\_videomode \*default\_mode, unsigned int default\_bpp)
:   finds a valid video mode

**Parameters**

`struct fb_var_screeninfo *var`
:   frame buffer user defined part of display

`struct fb_info *info`
:   frame buffer info structure

`const char *mode_option`
:   string video mode to find

`const struct fb_videomode *db`
:   video mode database

`unsigned int dbsize`
:   size of **db**

`const struct fb_videomode *default_mode`
:   default video mode to fall back to

`unsigned int default_bpp`
:   default color depth in bits per pixel

**Description**

Finds a suitable video mode, starting with the specified mode
in **mode\_option** with fallback to **default\_mode**. If
**default\_mode** fails, all modes in the video mode database will
be tried.

Valid mode specifiers for **mode\_option**:

```
<xres>x<yres>[M][R][-<bpp>][@<refresh>][i][p][m]
```

or

```
<name>[-<bpp>][@<refresh>]
```

with <xres>, <yres>, <bpp> and <refresh> decimal numbers and
<name> a string.

If ‘M’ is present after yres (and before refresh/bpp if present),
the function will compute the timings using VESA(tm) Coordinated
Video Timings (CVT). If ‘R’ is present after ‘M’, will compute with
reduced blanking (for flatpanels). If ‘i’ or ‘p’ are present, compute
interlaced or progressive mode. If ‘m’ is present, add margins equal
to 1.8% of xres rounded down to 8 pixels, and 1.8% of yres. The char
‘i’, ‘p’ and ‘m’ must be after ‘M’ and ‘R’. Example:

```
1024x768MR-8@60m - Reduced blank with margins at 60Hz.
```

**NOTE**

The passed struct **var** is \_not\_ cleared! This allows you
to supply values for e.g. the grayscale and accel\_flags fields.

Returns zero for failure, 1 if using specified **mode\_option**,
2 if using specified **mode\_option** with an ignored refresh rate,
3 if default mode is used, 4 if fall back to any valid mode.

void fb\_var\_to\_videomode(struct fb\_videomode \*mode, const struct fb\_var\_screeninfo \*var)
:   convert fb\_var\_screeninfo to fb\_videomode

**Parameters**

`struct fb_videomode *mode`
:   pointer to `struct fb_videomode`

`const struct fb_var_screeninfo *var`
:   pointer to `struct fb_var_screeninfo`

void fb\_videomode\_to\_var(struct fb\_var\_screeninfo \*var, const struct fb\_videomode \*mode)
:   convert fb\_videomode to fb\_var\_screeninfo

**Parameters**

`struct fb_var_screeninfo *var`
:   pointer to `struct fb_var_screeninfo`

`const struct fb_videomode *mode`
:   pointer to `struct fb_videomode`

int fb\_mode\_is\_equal(const struct fb\_videomode \*mode1, const struct fb\_videomode \*mode2)
:   compare 2 videomodes

**Parameters**

`const struct fb_videomode *mode1`
:   first videomode

`const struct fb_videomode *mode2`
:   second videomode

**Return**

1 if equal, 0 if not

const struct fb\_videomode \*fb\_find\_best\_mode(const struct fb\_var\_screeninfo \*var, struct list\_head \*head)
:   find best matching videomode

**Parameters**

`const struct fb_var_screeninfo *var`
:   pointer to `struct fb_var_screeninfo`

`struct list_head *head`
:   pointer to `struct list_head` of modelist

**Return**

`struct fb_videomode`, NULL if none found

**Description**

IMPORTANT:
This function assumes that all modelist entries in
info->modelist are valid.

**NOTES**

Finds best matching videomode which has an equal or greater dimension than
var->xres and var->yres. If more than 1 videomode is found, will return
the videomode with the highest refresh rate

const struct fb\_videomode \*fb\_find\_nearest\_mode(const struct fb\_videomode \*mode, struct list\_head \*head)
:   find closest videomode

**Parameters**

`const struct fb_videomode *mode`
:   pointer to `struct fb_videomode`

`struct list_head *head`
:   pointer to modelist

**Description**

Finds best matching videomode, smaller or greater in dimension.
If more than 1 videomode is found, will return the videomode with
the closest refresh rate.

const struct fb\_videomode \*fb\_match\_mode(const struct fb\_var\_screeninfo \*var, struct list\_head \*head)
:   find a videomode which exactly matches the timings in var

**Parameters**

`const struct fb_var_screeninfo *var`
:   pointer to `struct fb_var_screeninfo`

`struct list_head *head`
:   pointer to `struct list_head` of modelist

**Return**

`struct fb_videomode`, NULL if none found

int fb\_add\_videomode(const struct fb\_videomode \*mode, struct list\_head \*head)
:   adds videomode entry to modelist

**Parameters**

`const struct fb_videomode *mode`
:   videomode to add

`struct list_head *head`
:   `struct list_head` of modelist

**NOTES**

Will only add unmatched mode entries

void fb\_destroy\_modelist(struct list\_head \*head)
:   destroy modelist

**Parameters**

`struct list_head *head`
:   `struct list_head` of modelist

void fb\_videomode\_to\_modelist(const struct fb\_videomode \*modedb, int num, struct list\_head \*head)
:   convert mode array to mode list

**Parameters**

`const struct fb_videomode *modedb`
:   array of `struct fb_videomode`

`int num`
:   number of entries in array

`struct list_head *head`
:   `struct list_head` of modelist

## Frame Buffer Macintosh Video Mode Database

int mac\_vmode\_to\_var(int vmode, int cmode, struct fb\_var\_screeninfo \*var)
:   converts vmode/cmode pair to var structure

**Parameters**

`int vmode`
:   MacOS video mode

`int cmode`
:   MacOS color mode

`struct fb_var_screeninfo *var`
:   frame buffer video mode structure

**Description**

> Converts a MacOS vmode/cmode pair to a frame buffer video
> mode structure.
>
> Returns negative errno on error, or zero for success.

int mac\_map\_monitor\_sense(int sense)
:   Convert monitor sense to vmode

**Parameters**

`int sense`
:   Macintosh monitor sense number

**Description**

> Converts a Macintosh monitor sense number to a MacOS
> vmode number.
>
> Returns MacOS vmode video mode number.

int mac\_find\_mode(struct fb\_var\_screeninfo \*var, struct fb\_info \*info, const char \*mode\_option, unsigned int default\_bpp)
:   find a video mode

**Parameters**

`struct fb_var_screeninfo *var`
:   frame buffer user defined part of display

`struct fb_info *info`
:   frame buffer info structure

`const char *mode_option`
:   video mode name (see mac\_modedb[])

`unsigned int default_bpp`
:   default color depth in bits per pixel

**Description**

> Finds a suitable video mode. Tries to set mode specified
> by **mode\_option**. If the name of the wanted mode begins with
> ‘mac’, the Mac video mode database will be used, otherwise it
> will fall back to the standard video mode database.

**Note**

Function marked as \_\_init and can only be used during
:   system boot.

    Returns error code from fb\_find\_mode (see fb\_find\_mode
    function).

## Frame Buffer Fonts

Refer to the file lib/fonts/fonts.c for more information.
