# 3.15.V4L2 Controls

> 출처(원문): https://docs.kernel.org/driver-api/media/v4l2-controls.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 3.15. V4L2 Controls

## 3.15.1. Introduction

The V4L2 control API seems simple enough, but quickly becomes very hard to
implement correctly in drivers. But much of the code needed to handle controls
is actually not driver specific and can be moved to the V4L core framework.

After all, the only part that a driver developer is interested in is:

1. How do I add a control?
2. How do I set the control’s value? (i.e. s\_ctrl)

And occasionally:

3. How do I get the control’s value? (i.e. g\_volatile\_ctrl)
4. How do I validate the user’s proposed control value? (i.e. try\_ctrl)

All the rest is something that can be done centrally.

The control framework was created in order to implement all the rules of the
V4L2 specification with respect to controls in a central place. And to make
life as easy as possible for the driver developer.

Note that the control framework relies on the presence of a struct
[`v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device") for V4L2 drivers and [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") for
sub-device drivers.

## 3.15.2. Objects in the framework

There are two main objects:

The [`v4l2_ctrl`](#c.v4l2_ctrl "v4l2_ctrl") object describes the control properties and keeps
track of the control’s value (both the current value and the proposed new
value).

[`v4l2_ctrl_handler`](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") is the object that keeps track of controls. It
maintains a list of v4l2\_ctrl objects that it owns and another list of
references to controls, possibly to controls owned by other handlers.

## 3.15.3. Basic usage for V4L2 and sub-device drivers

1. Prepare the driver:

```
#include <media/v4l2-ctrls.h>
```

1.1) Add the handler to your driver’s top-level struct:

For V4L2 drivers:

```
struct foo_dev {
        ...
        struct v4l2_device v4l2_dev;
        ...
        struct v4l2_ctrl_handler ctrl_handler;
        ...
};
```

For sub-device drivers:

```
struct foo_dev {
        ...
        struct v4l2_subdev sd;
        ...
        struct v4l2_ctrl_handler ctrl_handler;
        ...
};
```

1.2) Initialize the handler:

```
v4l2_ctrl_handler_init(&foo->ctrl_handler, nr_of_controls);
```

The second argument is a hint telling the function how many controls this
handler is expected to handle. It will allocate a hashtable based on this
information. It is a hint only.

1.3) Hook the control handler into the driver:

For V4L2 drivers:

```
foo->v4l2_dev.ctrl_handler = &foo->ctrl_handler;
```

For sub-device drivers:

```
foo->sd.ctrl_handler = &foo->ctrl_handler;
```

1.4) Clean up the handler at the end:

```
v4l2_ctrl_handler_free(&foo->ctrl_handler);
```

[`v4l2_ctrl_handler_free()`](#c.v4l2_ctrl_handler_free "v4l2_ctrl_handler_free") does not touch the handler’s `error` field.

2. Add controls:

You add non-menu controls by calling [`v4l2_ctrl_new_std()`](#c.v4l2_ctrl_new_std "v4l2_ctrl_new_std"):

```
struct v4l2_ctrl *v4l2_ctrl_new_std(struct v4l2_ctrl_handler *hdl,
                const struct v4l2_ctrl_ops *ops,
                u32 id, s32 min, s32 max, u32 step, s32 def);
```

Menu and integer menu controls are added by calling
[`v4l2_ctrl_new_std_menu()`](#c.v4l2_ctrl_new_std_menu "v4l2_ctrl_new_std_menu"):

```
struct v4l2_ctrl *v4l2_ctrl_new_std_menu(struct v4l2_ctrl_handler *hdl,
                const struct v4l2_ctrl_ops *ops,
                u32 id, s32 max, s32 skip_mask, s32 def);
```

Menu controls with a driver specific menu are added by calling
[`v4l2_ctrl_new_std_menu_items()`](#c.v4l2_ctrl_new_std_menu_items "v4l2_ctrl_new_std_menu_items"):

```
struct v4l2_ctrl *v4l2_ctrl_new_std_menu_items(
                struct v4l2_ctrl_handler *hdl,
                const struct v4l2_ctrl_ops *ops, u32 id, s32 max,
                s32 skip_mask, s32 def, const char * const *qmenu);
```

Standard compound controls can be added by calling
[`v4l2_ctrl_new_std_compound()`](#c.v4l2_ctrl_new_std_compound "v4l2_ctrl_new_std_compound"):

```
struct v4l2_ctrl *v4l2_ctrl_new_std_compound(struct v4l2_ctrl_handler *hdl,
                const struct v4l2_ctrl_ops *ops, u32 id,
                const union v4l2_ctrl_ptr p_def);
```

Integer menu controls with a driver specific menu can be added by calling
[`v4l2_ctrl_new_int_menu()`](#c.v4l2_ctrl_new_int_menu "v4l2_ctrl_new_int_menu"):

```
struct v4l2_ctrl *v4l2_ctrl_new_int_menu(struct v4l2_ctrl_handler *hdl,
                const struct v4l2_ctrl_ops *ops,
                u32 id, s32 max, s32 def, const s64 *qmenu_int);
```

These functions are typically called right after the
[`v4l2_ctrl_handler_init()`](#c.v4l2_ctrl_handler_init "v4l2_ctrl_handler_init"):

```
static const s64 exp_bias_qmenu[] = {
       -2, -1, 0, 1, 2
};
static const char * const test_pattern[] = {
        "Disabled",
        "Vertical Bars",
        "Solid Black",
        "Solid White",
};

v4l2_ctrl_handler_init(&foo->ctrl_handler, nr_of_controls);
v4l2_ctrl_new_std(&foo->ctrl_handler, &foo_ctrl_ops,
                V4L2_CID_BRIGHTNESS, 0, 255, 1, 128);
v4l2_ctrl_new_std(&foo->ctrl_handler, &foo_ctrl_ops,
                V4L2_CID_CONTRAST, 0, 255, 1, 128);
v4l2_ctrl_new_std_menu(&foo->ctrl_handler, &foo_ctrl_ops,
                V4L2_CID_POWER_LINE_FREQUENCY,
                V4L2_CID_POWER_LINE_FREQUENCY_60HZ, 0,
                V4L2_CID_POWER_LINE_FREQUENCY_DISABLED);
v4l2_ctrl_new_int_menu(&foo->ctrl_handler, &foo_ctrl_ops,
                V4L2_CID_EXPOSURE_BIAS,
                ARRAY_SIZE(exp_bias_qmenu) - 1,
                ARRAY_SIZE(exp_bias_qmenu) / 2 - 1,
                exp_bias_qmenu);
v4l2_ctrl_new_std_menu_items(&foo->ctrl_handler, &foo_ctrl_ops,
                V4L2_CID_TEST_PATTERN, ARRAY_SIZE(test_pattern) - 1, 0,
                0, test_pattern);
...
if (foo->ctrl_handler.error)
        return v4l2_ctrl_handler_free(&foo->ctrl_handler);
```

The [`v4l2_ctrl_new_std()`](#c.v4l2_ctrl_new_std "v4l2_ctrl_new_std") function returns the v4l2\_ctrl pointer to
the new control, but if you do not need to access the pointer outside the
control ops, then there is no need to store it.

The [`v4l2_ctrl_new_std()`](#c.v4l2_ctrl_new_std "v4l2_ctrl_new_std") function will fill in most fields based on
the control ID except for the min, max, step and default values. These are
passed in the last four arguments. These values are driver specific while
control attributes like type, name, flags are all global. The control’s
current value will be set to the default value.

The [`v4l2_ctrl_new_std_menu()`](#c.v4l2_ctrl_new_std_menu "v4l2_ctrl_new_std_menu") function is very similar but it is
used for menu controls. There is no min argument since that is always 0 for
menu controls, and instead of a step there is a skip\_mask argument: if bit
X is 1, then menu item X is skipped.

The [`v4l2_ctrl_new_int_menu()`](#c.v4l2_ctrl_new_int_menu "v4l2_ctrl_new_int_menu") function creates a new standard
integer menu control with driver-specific items in the menu. It differs
from v4l2\_ctrl\_new\_std\_menu in that it doesn’t have the mask argument and
takes as the last argument an array of signed 64-bit integers that form an
exact menu item list.

The [`v4l2_ctrl_new_std_menu_items()`](#c.v4l2_ctrl_new_std_menu_items "v4l2_ctrl_new_std_menu_items") function is very similar to
v4l2\_ctrl\_new\_std\_menu but takes an extra parameter qmenu, which is the
driver specific menu for an otherwise standard menu control. A good example
for this control is the test pattern control for capture/display/sensors
devices that have the capability to generate test patterns. These test
patterns are hardware specific, so the contents of the menu will vary from
device to device.

Note that if something fails, the function will return NULL or an error and
set ctrl\_handler->error to the error code. If ctrl\_handler->error was already
set, then it will just return and do nothing. This is also true for
v4l2\_ctrl\_handler\_init if it cannot allocate the internal data structure.

This makes it easy to init the handler and just add all controls and only check
the error code at the end. Saves a lot of repetitive error checking.

It is recommended to add controls in ascending control ID order: it will be
a bit faster that way.

3. Optionally force initial control setup:

```
v4l2_ctrl_handler_setup(&foo->ctrl_handler);
```

This will call s\_ctrl for all controls unconditionally. Effectively this
initializes the hardware to the default control values. It is recommended
that you do this as this ensures that both the internal data structures and
the hardware are in sync.

4. Finally: implement the [`v4l2_ctrl_ops`](#c.v4l2_ctrl_ops "v4l2_ctrl_ops")

```
static const struct v4l2_ctrl_ops foo_ctrl_ops = {
        .s_ctrl = foo_s_ctrl,
};
```

Usually all you need is s\_ctrl:

```
static int foo_s_ctrl(struct v4l2_ctrl *ctrl)
{
        struct foo *state = container_of(ctrl->handler, struct foo, ctrl_handler);

        switch (ctrl->id) {
        case V4L2_CID_BRIGHTNESS:
                write_reg(0x123, ctrl->val);
                break;
        case V4L2_CID_CONTRAST:
                write_reg(0x456, ctrl->val);
                break;
        }
        return 0;
}
```

The control ops are called with the v4l2\_ctrl pointer as argument.
The new control value has already been validated, so all you need to do is
to actually update the hardware registers.

You’re done! And this is sufficient for most of the drivers we have. No need
to do any validation of control values, or implement QUERYCTRL, QUERY\_EXT\_CTRL
and QUERYMENU. And G/S\_CTRL as well as G/TRY/S\_EXT\_CTRLS are automatically supported.

Note

The remainder sections deal with more advanced controls topics and scenarios.
In practice the basic usage as described above is sufficient for most drivers.

## 3.15.4. Inheriting Sub-device Controls

When a sub-device is registered with a V4L2 driver by calling
[`v4l2_device_register_subdev()`](v4l2-device.html#c.v4l2_device_register_subdev "v4l2_device_register_subdev") and the ctrl\_handler fields of both v4l2\_subdev
and v4l2\_device are set, then the controls of the subdev will become
automatically available in the V4L2 driver as well. If the subdev driver
contains controls that already exist in the V4L2 driver, then those will be
skipped (so a V4L2 driver can always override a subdev control).

What happens here is that [`v4l2_device_register_subdev()`](v4l2-device.html#c.v4l2_device_register_subdev "v4l2_device_register_subdev") calls
[`v4l2_ctrl_add_handler()`](#c.v4l2_ctrl_add_handler "v4l2_ctrl_add_handler") adding the controls of the subdev to the controls
of v4l2\_device.

## 3.15.5. Accessing Control Values

The following `union is` used inside the control framework to access control
values:

```
union v4l2_ctrl_ptr {
        s32 *p_s32;
        s64 *p_s64;
        char *p_char;
        void *p;
};
```

The v4l2\_ctrl `struct contains` these fields that can be used to access both
current and new values:

```
s32 val;
struct {
        s32 val;
} cur;


union v4l2_ctrl_ptr p_new;
union v4l2_ctrl_ptr p_cur;
```

If the control has a simple s32 type, then:

```
&ctrl->val == ctrl->p_new.p_s32
&ctrl->cur.val == ctrl->p_cur.p_s32
```

For all other types use ctrl->p\_cur.p<something>. Basically the val
and cur.val fields can be considered an alias since these are used so often.

Within the control ops you can freely use these. The val and cur.val speak for
themselves. The p\_char pointers point to character buffers of length
ctrl->maximum + 1, and are always 0-terminated.

Unless the control is marked volatile the p\_cur field points to the
current cached control value. When you create a new control this value is made
identical to the default value. After calling [`v4l2_ctrl_handler_setup()`](#c.v4l2_ctrl_handler_setup "v4l2_ctrl_handler_setup") this
value is passed to the hardware. It is generally a good idea to call this
function.

Whenever a new value is set that new value is automatically cached. This means
that most drivers do not need to implement the `g_volatile_ctrl()` op. The
exception is for controls that return a volatile register such as a signal
strength read-out that changes continuously. In that case you will need to
implement g\_volatile\_ctrl like this:

```
static int foo_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
        switch (ctrl->id) {
        case V4L2_CID_BRIGHTNESS:
                ctrl->val = read_reg(0x123);
                break;
        }
}
```

Note that you use the ‘new value’ `union as` well in g\_volatile\_ctrl. In general
controls that need to implement g\_volatile\_ctrl are read-only controls. If they
are not, a V4L2\_EVENT\_CTRL\_CH\_VALUE will not be generated when the control
changes.

To mark a control as volatile you have to set V4L2\_CTRL\_FLAG\_VOLATILE:

```
ctrl = v4l2_ctrl_new_std(&sd->ctrl_handler, ...);
if (ctrl)
        ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
```

For try/s\_ctrl the new values (i.e. as passed by the user) are filled in and
you can modify them in try\_ctrl or set them in s\_ctrl. The ‘cur’ `union
contains` the current value, which you can use (but not change!) as well.

If s\_ctrl returns 0 (OK), then the control framework will copy the new final
values to the ‘cur’ union.

While in g\_volatile/s/try\_ctrl you can access the value of all controls owned
by the same handler since the handler’s lock is held. If you need to access
the value of controls owned by other handlers, then you have to be very careful
not to introduce deadlocks.

Outside of the control ops you have to go through to helper functions to get
or set a single control value safely in your driver:

```
s32 v4l2_ctrl_g_ctrl(struct v4l2_ctrl *ctrl);
int v4l2_ctrl_s_ctrl(struct v4l2_ctrl *ctrl, s32 val);
```

These functions go through the control framework just as VIDIOC\_G/S\_CTRL ioctls
do. Don’t use these inside the control ops g\_volatile/s/try\_ctrl, though, that
will result in a deadlock since these helpers lock the handler as well.

You can also take the handler lock yourself:

```
mutex_lock(&state->ctrl_handler.lock);
pr_info("String value is '%s'\n", ctrl1->p_cur.p_char);
pr_info("Integer value is '%s'\n", ctrl2->cur.val);
mutex_unlock(&state->ctrl_handler.lock);
```

## 3.15.6. Menu Controls

The v4l2\_ctrl `struct contains` this union:

```
union {
        u32 step;
        u32 menu_skip_mask;
};
```

For menu controls menu\_skip\_mask is used. What it does is that it allows you
to easily exclude certain menu items. This is used in the VIDIOC\_QUERYMENU
implementation where you can return -EINVAL if a certain menu item is not
present. Note that VIDIOC\_QUERYCTRL always returns a step value of 1 for
menu controls.

A good example is the MPEG Audio Layer II Bitrate menu control where the
menu is a list of standardized possible bitrates. But in practice hardware
implementations will only support a subset of those. By setting the skip
mask you can tell the framework which menu items should be skipped. Setting
it to 0 means that all menu items are supported.

You set this mask either through the v4l2\_ctrl\_config struct for a custom
control, or by calling [`v4l2_ctrl_new_std_menu()`](#c.v4l2_ctrl_new_std_menu "v4l2_ctrl_new_std_menu").

## 3.15.7. Custom Controls

Driver specific controls can be created using [`v4l2_ctrl_new_custom()`](#c.v4l2_ctrl_new_custom "v4l2_ctrl_new_custom"):

```
static const struct v4l2_ctrl_config ctrl_filter = {
        .ops = &ctrl_custom_ops,
        .id = V4L2_CID_MPEG_CX2341X_VIDEO_SPATIAL_FILTER,
        .name = "Spatial Filter",
        .type = V4L2_CTRL_TYPE_INTEGER,
        .flags = V4L2_CTRL_FLAG_SLIDER,
        .max = 15,
        .step = 1,
};

ctrl = v4l2_ctrl_new_custom(&foo->ctrl_handler, &ctrl_filter, NULL);
```

The last argument is the priv pointer which can be set to driver-specific
private data.

The v4l2\_ctrl\_config `struct also` has a field to set the is\_private flag.

If the name field is not set, then the framework will assume this is a standard
control and will fill in the name, type and flags fields accordingly.

## 3.15.8. Active and Grabbed Controls

If you get more complex relationships between controls, then you may have to
activate and deactivate controls. For example, if the Chroma AGC control is
on, then the Chroma Gain control is inactive. That is, you may set it, but
the value will not be used by the hardware as long as the automatic gain
control is on. Typically user interfaces can disable such input fields.

You can set the ‘active’ status using [`v4l2_ctrl_activate()`](#c.v4l2_ctrl_activate "v4l2_ctrl_activate"). By default all
controls are active. Note that the framework does not check for this flag.
It is meant purely for GUIs. The function is typically called from within
s\_ctrl.

The other flag is the ‘grabbed’ flag. A grabbed control means that you cannot
change it because it is in use by some resource. Typical examples are MPEG
bitrate controls that cannot be changed while capturing is in progress.

If a control is set to ‘grabbed’ using [`v4l2_ctrl_grab()`](#c.v4l2_ctrl_grab "v4l2_ctrl_grab"), then the framework
will return -EBUSY if an attempt is made to set this control. The
[`v4l2_ctrl_grab()`](#c.v4l2_ctrl_grab "v4l2_ctrl_grab") function is typically called from the driver when it
starts or stops streaming.

## 3.15.9. Control Clusters

By default all controls are independent from the others. But in more
complex scenarios you can get dependencies from one control to another.
In that case you need to ‘cluster’ them:

```
struct foo {
        struct v4l2_ctrl_handler ctrl_handler;
#define AUDIO_CL_VOLUME (0)
#define AUDIO_CL_MUTE   (1)
        struct v4l2_ctrl *audio_cluster[2];
        ...
};

state->audio_cluster[AUDIO_CL_VOLUME] =
        v4l2_ctrl_new_std(&state->ctrl_handler, ...);
state->audio_cluster[AUDIO_CL_MUTE] =
        v4l2_ctrl_new_std(&state->ctrl_handler, ...);
v4l2_ctrl_cluster(ARRAY_SIZE(state->audio_cluster), state->audio_cluster);
```

From now on whenever one or more of the controls belonging to the same
cluster is set (or ‘gotten’, or ‘tried’), only the control ops of the first
control (‘volume’ in this example) is called. You effectively create a new
composite control. Similar to how a ‘struct’ works in C.

So when s\_ctrl is called with V4L2\_CID\_AUDIO\_VOLUME as argument, you should set
all two controls belonging to the audio\_cluster:

```
static int foo_s_ctrl(struct v4l2_ctrl *ctrl)
{
        struct foo *state = container_of(ctrl->handler, struct foo, ctrl_handler);

        switch (ctrl->id) {
        case V4L2_CID_AUDIO_VOLUME: {
                struct v4l2_ctrl *mute = ctrl->cluster[AUDIO_CL_MUTE];

                write_reg(0x123, mute->val ? 0 : ctrl->val);
                break;
        }
        case V4L2_CID_CONTRAST:
                write_reg(0x456, ctrl->val);
                break;
        }
        return 0;
}
```

In the example above the following are equivalent for the VOLUME case:

```
ctrl == ctrl->cluster[AUDIO_CL_VOLUME] == state->audio_cluster[AUDIO_CL_VOLUME]
ctrl->cluster[AUDIO_CL_MUTE] == state->audio_cluster[AUDIO_CL_MUTE]
```

In practice using cluster arrays like this becomes very tiresome. So instead
the following equivalent method is used:

```
struct {
        /* audio cluster */
        struct v4l2_ctrl *volume;
        struct v4l2_ctrl *mute;
};
```

The anonymous `struct is` used to clearly ‘cluster’ these two control pointers,
but it serves no other purpose. The effect is the same as creating an
array with two control pointers. So you can just do:

```
state->volume = v4l2_ctrl_new_std(&state->ctrl_handler, ...);
state->mute = v4l2_ctrl_new_std(&state->ctrl_handler, ...);
v4l2_ctrl_cluster(2, &state->volume);
```

And in foo\_s\_ctrl you can use these pointers directly: state->mute->val.

Note that controls in a cluster may be NULL. For example, if for some
reason mute was never added (because the hardware doesn’t support that
particular feature), then mute will be NULL. So in that case we have a
cluster of 2 controls, of which only 1 is actually instantiated. The
only restriction is that the first control of the cluster must always be
present, since that is the ‘master’ control of the cluster. The master
control is the one that identifies the cluster and that provides the
pointer to the v4l2\_ctrl\_ops struct that is used for that cluster.

Obviously, all controls in the cluster array must be initialized to either
a valid control or to NULL.

In rare cases you might want to know which controls of a cluster actually
were set explicitly by the user. For this you can check the ‘is\_new’ flag of
each control. For example, in the case of a volume/mute cluster the ‘is\_new’
flag of the mute control would be set if the user called VIDIOC\_S\_CTRL for
mute only. If the user would call VIDIOC\_S\_EXT\_CTRLS for both mute and volume
controls, then the ‘is\_new’ flag would be 1 for both controls.

The ‘is\_new’ flag is always 1 when called from [`v4l2_ctrl_handler_setup()`](#c.v4l2_ctrl_handler_setup "v4l2_ctrl_handler_setup").

## 3.15.10. Handling autogain/gain-type Controls with Auto Clusters

A common type of control cluster is one that handles ‘auto-foo/foo’-type
controls. Typical examples are autogain/gain, autoexposure/exposure,
autowhitebalance/red balance/blue balance. In all cases you have one control
that determines whether another control is handled automatically by the hardware,
or whether it is under manual control from the user.

If the cluster is in automatic mode, then the manual controls should be
marked inactive and volatile. When the volatile controls are read the
g\_volatile\_ctrl operation should return the value that the hardware’s automatic
mode set up automatically.

If the cluster is put in manual mode, then the manual controls should become
active again and the volatile flag is cleared (so g\_volatile\_ctrl is no longer
called while in manual mode). In addition just before switching to manual mode
the current values as determined by the auto mode are copied as the new manual
values.

Finally the V4L2\_CTRL\_FLAG\_UPDATE should be set for the auto control since
changing that control affects the control flags of the manual controls.

In order to simplify this a special variation of v4l2\_ctrl\_cluster was
introduced:

```
void v4l2_ctrl_auto_cluster(unsigned ncontrols, struct v4l2_ctrl **controls,
                            u8 manual_val, bool set_volatile);
```

The first two arguments are identical to v4l2\_ctrl\_cluster. The third argument
tells the framework which value switches the cluster into manual mode. The
last argument will optionally set V4L2\_CTRL\_FLAG\_VOLATILE for the non-auto controls.
If it is false, then the manual controls are never volatile. You would typically
use that if the hardware does not give you the option to read back to values as
determined by the auto mode (e.g. if autogain is on, the hardware doesn’t allow
you to obtain the current gain value).

The first control of the cluster is assumed to be the ‘auto’ control.

Using this function will ensure that you don’t need to handle all the complex
flag and volatile handling.

## 3.15.11. VIDIOC\_LOG\_STATUS Support

This ioctl allow you to dump the current status of a driver to the kernel log.
The v4l2\_ctrl\_handler\_log\_status(ctrl\_handler, prefix) can be used to dump the
value of the controls owned by the given handler to the log. You can supply a
prefix as well. If the prefix didn’t end with a space, then ‘: ‘ will be added
for you.

## 3.15.12. Different Handlers for Different Video Nodes

Usually the V4L2 driver has just one control handler that is global for
all video nodes. But you can also specify different control handlers for
different video nodes. You can do that by manually setting the ctrl\_handler
field of [`struct video_device`](v4l2-dev.html#c.video_device "video_device").

That is no problem if there are no subdevs involved but if there are, then
you need to block the automatic merging of subdev controls to the global
control handler. You do that by simply setting the ctrl\_handler field in
[`struct v4l2_device`](v4l2-device.html#c.v4l2_device "v4l2_device") to NULL. Now [`v4l2_device_register_subdev()`](v4l2-device.html#c.v4l2_device_register_subdev "v4l2_device_register_subdev") will no longer
merge subdev controls.

After each subdev was added, you will then have to call v4l2\_ctrl\_add\_handler
manually to add the subdev’s control handler (sd->ctrl\_handler) to the desired
control handler. This control handler may be specific to the video\_device or
for a subset of video\_device’s. For example: the radio device nodes only have
audio controls, while the video and vbi device nodes share the same control
handler for the audio and video controls.

If you want to have one handler (e.g. for a radio device node) have a subset
of another handler (e.g. for a video device node), then you should first add
the controls to the first handler, add the other controls to the second
handler and finally add the first handler to the second. For example:

```
v4l2_ctrl_new_std(&radio_ctrl_handler, &radio_ops, V4L2_CID_AUDIO_VOLUME, ...);
v4l2_ctrl_new_std(&radio_ctrl_handler, &radio_ops, V4L2_CID_AUDIO_MUTE, ...);
v4l2_ctrl_new_std(&video_ctrl_handler, &video_ops, V4L2_CID_BRIGHTNESS, ...);
v4l2_ctrl_new_std(&video_ctrl_handler, &video_ops, V4L2_CID_CONTRAST, ...);
v4l2_ctrl_add_handler(&video_ctrl_handler, &radio_ctrl_handler, NULL);
```

The last argument to [`v4l2_ctrl_add_handler()`](#c.v4l2_ctrl_add_handler "v4l2_ctrl_add_handler") is a filter function that allows
you to filter which controls will be added. Set it to NULL if you want to add
all controls.

Or you can add specific controls to a handler:

```
volume = v4l2_ctrl_new_std(&video_ctrl_handler, &ops, V4L2_CID_AUDIO_VOLUME, ...);
v4l2_ctrl_new_std(&video_ctrl_handler, &ops, V4L2_CID_BRIGHTNESS, ...);
v4l2_ctrl_new_std(&video_ctrl_handler, &ops, V4L2_CID_CONTRAST, ...);
```

What you should not do is make two identical controls for two handlers.
For example:

```
v4l2_ctrl_new_std(&radio_ctrl_handler, &radio_ops, V4L2_CID_AUDIO_MUTE, ...);
v4l2_ctrl_new_std(&video_ctrl_handler, &video_ops, V4L2_CID_AUDIO_MUTE, ...);
```

This would be bad since muting the radio would not change the video mute
control. The rule is to have one control for each hardware ‘knob’ that you
can twiddle.

## 3.15.13. Finding Controls

Normally you have created the controls yourself and you can store the [`struct
v4l2_ctrl`](#c.v4l2_ctrl "v4l2_ctrl") pointer into your own struct.

But sometimes you need to find a control from another handler that you do
not own. For example, if you have to find a volume control from a subdev.

You can do that by calling v4l2\_ctrl\_find:

```
struct v4l2_ctrl *volume;

volume = v4l2_ctrl_find(sd->ctrl_handler, V4L2_CID_AUDIO_VOLUME);
```

Since v4l2\_ctrl\_find will lock the handler you have to be careful where you
use it. For example, this is not a good idea:

```
struct v4l2_ctrl_handler ctrl_handler;

v4l2_ctrl_new_std(&ctrl_handler, &video_ops, V4L2_CID_BRIGHTNESS, ...);
v4l2_ctrl_new_std(&ctrl_handler, &video_ops, V4L2_CID_CONTRAST, ...);
```

...and in video\_ops.s\_ctrl:

```
case V4L2_CID_BRIGHTNESS:
        contrast = v4l2_find_ctrl(&ctrl_handler, V4L2_CID_CONTRAST);
        ...
```

When s\_ctrl is called by the framework the ctrl\_handler.lock is already taken, so
attempting to find another control from the same handler will deadlock.

It is recommended not to use this function from inside the control ops.

## 3.15.14. Preventing Controls inheritance

When one control handler is added to another using v4l2\_ctrl\_add\_handler, then
by default all controls from one are merged to the other. But a subdev might
have low-level controls that make sense for some advanced embedded system, but
not when it is used in consumer-level hardware. In that case you want to keep
those low-level controls local to the subdev. You can do this by simply
setting the ‘is\_private’ flag of the control to 1:

```
static const struct v4l2_ctrl_config ctrl_private = {
        .ops = &ctrl_custom_ops,
        .id = V4L2_CID_...,
        .name = "Some Private Control",
        .type = V4L2_CTRL_TYPE_INTEGER,
        .max = 15,
        .step = 1,
        .is_private = 1,
};

ctrl = v4l2_ctrl_new_custom(&foo->ctrl_handler, &ctrl_private, NULL);
```

These controls will now be skipped when v4l2\_ctrl\_add\_handler is called.

## 3.15.15. V4L2\_CTRL\_TYPE\_CTRL\_CLASS Controls

Controls of this type can be used by GUIs to get the name of the control class.
A fully featured GUI can make a dialog with multiple tabs with each tab
containing the controls belonging to a particular control class. The name of
each tab can be found by querying a special control with ID <control class | 1>.

Drivers do not have to care about this. The framework will automatically add
a control of this type whenever the first control belonging to a new control
class is added.

## 3.15.16. Adding Notify Callbacks

Sometimes the platform or bridge driver needs to be notified when a control
from a sub-device driver changes. You can set a notify callback by calling
this function:

```
void v4l2_ctrl_notify(struct v4l2_ctrl *ctrl,
        void (*notify)(struct v4l2_ctrl *ctrl, void *priv), void *priv);
```

Whenever the give control changes value the notify callback will be called
with a pointer to the control and the priv pointer that was passed with
v4l2\_ctrl\_notify. Note that the control’s handler lock is held when the
notify function is called.

There can be only one notify function per control handler. Any attempt
to set another notify function will cause a WARN\_ON.

## 3.15.17. v4l2\_ctrl functions and data structures

union v4l2\_ctrl\_ptr
:   A pointer to a control value.

> **Definition**:
>
> ```
> union v4l2_ctrl_ptr {
>       s32 *p_s32;
>       s64 *p_s64;
>       u8 *p_u8;
>       u16 *p_u16;
>       u32 *p_u32;
>       char *p_char;
>       struct v4l2_ctrl_mpeg2_sequence *p_mpeg2_sequence;
>       struct v4l2_ctrl_mpeg2_picture *p_mpeg2_picture;
>       struct v4l2_ctrl_mpeg2_quantisation *p_mpeg2_quantisation;
>       struct v4l2_ctrl_fwht_params *p_fwht_params;
>       struct v4l2_ctrl_h264_sps *p_h264_sps;
>       struct v4l2_ctrl_h264_pps *p_h264_pps;
>       struct v4l2_ctrl_h264_scaling_matrix *p_h264_scaling_matrix;
>       struct v4l2_ctrl_h264_slice_params *p_h264_slice_params;
>       struct v4l2_ctrl_h264_decode_params *p_h264_decode_params;
>       struct v4l2_ctrl_h264_pred_weights *p_h264_pred_weights;
>       struct v4l2_ctrl_vp8_frame *p_vp8_frame;
>       struct v4l2_ctrl_hevc_sps *p_hevc_sps;
>       struct v4l2_ctrl_hevc_pps *p_hevc_pps;
>       struct v4l2_ctrl_hevc_slice_params *p_hevc_slice_params;
>       struct v4l2_ctrl_vp9_compressed_hdr *p_vp9_compressed_hdr_probs;
>       struct v4l2_ctrl_vp9_frame *p_vp9_frame;
>       struct v4l2_ctrl_hdr10_cll_info *p_hdr10_cll;
>       struct v4l2_ctrl_hdr10_mastering_display *p_hdr10_mastering;
>       struct v4l2_area *p_area;
>       struct v4l2_ctrl_av1_sequence *p_av1_sequence;
>       struct v4l2_ctrl_av1_tile_group_entry *p_av1_tile_group_entry;
>       struct v4l2_ctrl_av1_frame *p_av1_frame;
>       struct v4l2_ctrl_av1_film_grain *p_av1_film_grain;
>       struct v4l2_rect *p_rect;
>       void *p;
>       const void *p_const;
> };
> ```

**Members**

`p_s32`
:   Pointer to a 32-bit signed value.

`p_s64`
:   Pointer to a 64-bit signed value.

`p_u8`
:   Pointer to a 8-bit unsigned value.

`p_u16`
:   Pointer to a 16-bit unsigned value.

`p_u32`
:   Pointer to a 32-bit unsigned value.

`p_char`
:   Pointer to a string.

`p_mpeg2_sequence`
:   Pointer to a MPEG2 sequence structure.

`p_mpeg2_picture`
:   Pointer to a MPEG2 picture structure.

`p_mpeg2_quantisation`
:   Pointer to a MPEG2 quantisation data structure.

`p_fwht_params`
:   Pointer to a FWHT stateless parameters structure.

`p_h264_sps`
:   Pointer to a `struct v4l2_ctrl_h264_sps`.

`p_h264_pps`
:   Pointer to a `struct v4l2_ctrl_h264_pps`.

`p_h264_scaling_matrix`
:   Pointer to a `struct v4l2_ctrl_h264_scaling_matrix`.

`p_h264_slice_params`
:   Pointer to a `struct v4l2_ctrl_h264_slice_params`.

`p_h264_decode_params`
:   Pointer to a `struct v4l2_ctrl_h264_decode_params`.

`p_h264_pred_weights`
:   Pointer to a `struct v4l2_ctrl_h264_pred_weights`.

`p_vp8_frame`
:   Pointer to a VP8 frame params structure.

`p_hevc_sps`
:   Pointer to an HEVC sequence parameter set structure.

`p_hevc_pps`
:   Pointer to an HEVC picture parameter set structure.

`p_hevc_slice_params`
:   Pointer to an HEVC slice parameters structure.

`p_vp9_compressed_hdr_probs`
:   Pointer to a VP9 frame compressed header probs structure.

`p_vp9_frame`
:   Pointer to a VP9 frame params structure.

`p_hdr10_cll`
:   Pointer to an HDR10 Content Light Level structure.

`p_hdr10_mastering`
:   Pointer to an HDR10 Mastering Display structure.

`p_area`
:   Pointer to an area.

`p_av1_sequence`
:   Pointer to an AV1 sequence structure.

`p_av1_tile_group_entry`
:   Pointer to an AV1 tile group entry structure.

`p_av1_frame`
:   Pointer to an AV1 frame structure.

`p_av1_film_grain`
:   Pointer to an AV1 film grain structure.

`p_rect`
:   Pointer to a rectangle.

`p`
:   Pointer to a compound value.

`p_const`
:   Pointer to a constant compound value.

union [v4l2\_ctrl\_ptr](#c.v4l2_ctrl_ptr "v4l2_ctrl_ptr") v4l2\_ctrl\_ptr\_create(void \*ptr)
:   Helper function to return a v4l2\_ctrl\_ptr from a void pointer

**Parameters**

`void *ptr`
:   The void pointer

struct v4l2\_ctrl\_ops
:   The control operations that the driver has to provide.

> **Definition**:
>
> ```
> struct v4l2_ctrl_ops {
>       int (*g_volatile_ctrl)(struct v4l2_ctrl *ctrl);
>       int (*try_ctrl)(struct v4l2_ctrl *ctrl);
>       int (*s_ctrl)(struct v4l2_ctrl *ctrl);
> };
> ```

**Members**

`g_volatile_ctrl`
:   Get a new value for this control. Generally only relevant
    for volatile (and usually read-only) controls such as a control
    that returns the current signal strength which changes
    continuously.
    If not set, then the currently cached value will be returned.

`try_ctrl`
:   Test whether the control’s value is valid. Only relevant when
    the usual min/max/step checks are not sufficient.

`s_ctrl`
:   Actually set the new control value. s\_ctrl is compulsory. The
    ctrl->handler->lock is held when these ops are called, so no
    one else can access controls owned by that handler.

struct v4l2\_ctrl\_type\_ops
:   The control type operations that the driver has to provide.

> **Definition**:
>
> ```
> struct v4l2_ctrl_type_ops {
>       bool (*equal)(const struct v4l2_ctrl *ctrl, union v4l2_ctrl_ptr ptr1, union v4l2_ctrl_ptr ptr2);
>       void (*init)(const struct v4l2_ctrl *ctrl, u32 from_idx, union v4l2_ctrl_ptr ptr);
>       void (*minimum)(const struct v4l2_ctrl *ctrl, u32 idx, union v4l2_ctrl_ptr ptr);
>       void (*maximum)(const struct v4l2_ctrl *ctrl, u32 idx, union v4l2_ctrl_ptr ptr);
>       void (*log)(const struct v4l2_ctrl *ctrl);
>       int (*validate)(const struct v4l2_ctrl *ctrl, union v4l2_ctrl_ptr ptr);
> };
> ```

**Members**

`equal`
:   return true if all ctrl->elems array elements are equal.

`init`
:   initialize the value for array elements from from\_idx to ctrl->elems.

`minimum`
:   set the value to the minimum value of the control.

`maximum`
:   set the value to the maximum value of the control.

`log`
:   log the value.

`validate`
:   validate the value for ctrl->new\_elems array elements.
    Return 0 on success and a negative value otherwise.

v4l2\_ctrl\_notify\_fnc
:   **Typedef**: typedef for a notify argument with a function that should be called when a control value has changed.

**Syntax**

> `void v4l2_ctrl_notify_fnc (struct v4l2_ctrl *ctrl, void *priv)`

**Parameters**

`struct v4l2_ctrl *ctrl`
:   pointer to struct [`v4l2_ctrl`](#c.v4l2_ctrl "v4l2_ctrl")

`void *priv`
:   control private data

**Description**

This `typedef definition` is used as an argument to [`v4l2_ctrl_notify()`](#c.v4l2_ctrl_notify "v4l2_ctrl_notify")
and as an argument at struct [`v4l2_ctrl_handler`](#c.v4l2_ctrl_handler "v4l2_ctrl_handler").

struct v4l2\_ctrl
:   The control structure.

> **Definition**:
>
> ```
> struct v4l2_ctrl {
>       struct list_head node;
>       struct list_head ev_subs;
>       struct v4l2_ctrl_handler *handler;
>       struct v4l2_ctrl **cluster;
>       unsigned int ncontrols;
>       unsigned int done:1;
>       unsigned int is_new:1;
>       unsigned int has_changed:1;
>       unsigned int is_private:1;
>       unsigned int is_auto:1;
>       unsigned int is_int:1;
>       unsigned int is_string:1;
>       unsigned int is_ptr:1;
>       unsigned int is_array:1;
>       unsigned int is_dyn_array:1;
>       unsigned int has_volatiles:1;
>       unsigned int call_notify:1;
>       unsigned int manual_mode_value:8;
>       const struct v4l2_ctrl_ops *ops;
>       const struct v4l2_ctrl_type_ops *type_ops;
>       u32 id;
>       const char *name;
>       enum v4l2_ctrl_type type;
>       s64 minimum, maximum, default_value;
>       u32 elems;
>       u32 elem_size;
>       u32 new_elems;
>       u32 dims[V4L2_CTRL_MAX_DIMS];
>       u32 nr_of_dims;
>       union {
>             u64 step;
>             u64 menu_skip_mask;
>       };
>       union {
>             const char * const *qmenu;
>             const s64 *qmenu_int;
>       };
>       unsigned long flags;
>       void *priv;
>       void *p_array;
>       u32 p_array_alloc_elems;
>       s32 val;
>       struct {
>             s32 val;
>       } cur;
>       union v4l2_ctrl_ptr p_def;
>       union v4l2_ctrl_ptr p_min;
>       union v4l2_ctrl_ptr p_max;
>       union v4l2_ctrl_ptr p_new;
>       union v4l2_ctrl_ptr p_cur;
> };
> ```

**Members**

`node`
:   The list node.

`ev_subs`
:   The list of control event subscriptions.

`handler`
:   The handler that owns the control.

`cluster`
:   Point to start of cluster array.

`ncontrols`
:   Number of controls in cluster array.

`done`
:   Internal flag: set for each processed control.

`is_new`
:   Set when the user specified a new value for this control. It
    is also set when called from [`v4l2_ctrl_handler_setup()`](#c.v4l2_ctrl_handler_setup "v4l2_ctrl_handler_setup"). Drivers
    should never set this flag.

`has_changed`
:   Set when the current value differs from the new value. Drivers
    should never use this flag.

`is_private`
:   If set, then this control is private to its handler and it
    will not be added to any other handlers. Drivers can set
    this flag.

`is_auto`
:   If set, then this control selects whether the other cluster
    members are in ‘automatic’ mode or ‘manual’ mode. This is
    used for autogain/gain type clusters. Drivers should never
    set this flag directly.

`is_int`
:   If set, then this control has a simple integer value (i.e. it
    uses ctrl->val).

`is_string`
:   If set, then this control has type `V4L2_CTRL_TYPE_STRING`.

`is_ptr`
:   If set, then this control is an array and/or has type >=
    `V4L2_CTRL_COMPOUND_TYPES`
    and/or has type `V4L2_CTRL_TYPE_STRING`. In other words, `struct
    v4l2_ext_control` uses field p to point to the data.

`is_array`
:   If set, then this control contains an N-dimensional array.

`is_dyn_array`
:   If set, then this control contains a dynamically sized 1-dimensional array.
    If this is set, then **is\_array** is also set.

`has_volatiles`
:   If set, then one or more members of the cluster are volatile.
    Drivers should never touch this flag.

`call_notify`
:   If set, then call the handler’s notify function whenever the
    control’s value changes.

`manual_mode_value`
:   If the is\_auto flag is set, then this is the value
    of the auto control that determines if that control is in
    manual mode. So if the value of the auto control equals this
    value, then the whole cluster is in manual mode. Drivers should
    never set this flag directly.

`ops`
:   The control ops.

`type_ops`
:   The control type ops.

`id`
:   The control ID.

`name`
:   The control name.

`type`
:   The control type.

`minimum`
:   The control’s minimum value.

`maximum`
:   The control’s maximum value.

`default_value`
:   The control’s default value.

`elems`
:   The number of elements in the N-dimensional array.

`elem_size`
:   The size in bytes of the control.

`new_elems`
:   The number of elements in p\_new. This is the same as **elems**,
    except for dynamic arrays. In that case it is in the range of
    1 to **p\_array\_alloc\_elems**.

`dims`
:   The size of each dimension.

`nr_of_dims`
:   The number of dimensions in **dims**.

`{unnamed_union}`
:   anonymous

`step`
:   The control’s step value for non-menu controls.

`menu_skip_mask`
:   The control’s skip mask for menu controls. This makes it
    easy to skip menu items that are not valid. If bit X is set,
    then menu item X is skipped. Of course, this only works for
    menus with <= 32 menu items. There are no menus that come
    close to that number, so this is OK. Should we ever need more,
    then this will have to be extended to a u64 or a bit array.

`{unnamed_union}`
:   anonymous

`qmenu`
:   A const char \* array for all menu items. Array entries that are
    empty strings (“”) correspond to non-existing menu items (this
    is in addition to the menu\_skip\_mask above). The last entry
    must be NULL.
    Used only if the **type** is `V4L2_CTRL_TYPE_MENU`.

`qmenu_int`
:   A 64-bit integer array for with integer menu items.
    The size of array must be equal to the menu size, e. g.:
    ![ceil(\frac{maximum - minimum}{step}) + 1](_images/math/9016a6a64338a27b5fb85e1e8e9cf07a34c4a582.png).
    Used only if the **type** is `V4L2_CTRL_TYPE_INTEGER_MENU`.

`flags`
:   The control’s flags.

`priv`
:   The control’s private pointer. For use by the driver. It is
    untouched by the control framework. Note that this pointer is
    not freed when the control is deleted. Should this be needed
    then a new internal bitfield can be added to tell the framework
    to free this pointer.

`p_array`
:   Pointer to the allocated array. Only valid if **is\_array** is true.

`p_array_alloc_elems`
:   The number of elements in the allocated
    array for both the cur and new values. So **p\_array** is actually
    sized for 2 \* **p\_array\_alloc\_elems** \* **elem\_size**. Only valid if
    **is\_array** is true.

`val`
:   The control’s new s32 value.

`cur`
:   Structure to store the current value.

`cur.val`
:   The control’s current value, if the **type** is represented via
    a u32 integer (see `enum v4l2_ctrl_type`).

`p_def`
:   The control’s default value represented via a union which
    provides a standard way of accessing control types
    through a pointer (for compound controls only).

`p_min`
:   The control’s minimum value represented via a union which
    provides a standard way of accessing control types
    through a pointer (for compound controls only).

`p_max`
:   The control’s maximum value represented via a union which
    provides a standard way of accessing control types
    through a pointer (for compound controls only).

`p_new`
:   The control’s new value represented via a union which provides
    a standard way of accessing control types
    through a pointer.

`p_cur`
:   The control’s current value represented via a union which
    provides a standard way of accessing control types
    through a pointer.

struct v4l2\_ctrl\_ref
:   The control reference.

> **Definition**:
>
> ```
> struct v4l2_ctrl_ref {
>       struct list_head node;
>       struct v4l2_ctrl_ref *next;
>       struct v4l2_ctrl *ctrl;
>       struct v4l2_ctrl_helper *helper;
>       bool from_other_dev;
>       bool req_done;
>       bool p_req_valid;
>       bool p_req_array_enomem;
>       u32 p_req_array_alloc_elems;
>       u32 p_req_elems;
>       union v4l2_ctrl_ptr p_req;
> };
> ```

**Members**

`node`
:   List node for the sorted list.

`next`
:   Single-link list node for the hash.

`ctrl`
:   The actual control information.

`helper`
:   Pointer to helper struct. Used internally in
    `prepare_ext_ctrls` function at `v4l2-ctrl.c`.

`from_other_dev`
:   If true, then **ctrl** was defined in another
    device than the [`struct v4l2_ctrl_handler`](#c.v4l2_ctrl_handler "v4l2_ctrl_handler").

`req_done`
:   Internal flag: if the control handler containing this control
    reference is bound to a media request, then this is set when
    the control has been applied. This prevents applying controls
    from a cluster with multiple controls twice (when the first
    control of a cluster is applied, they all are).

`p_req_valid`
:   If set, then p\_req contains the control value for the request.

`p_req_array_enomem`
:   If set, then p\_req is invalid since allocating space for
    an array failed. Attempting to read this value shall
    result in ENOMEM. Only valid if ctrl->is\_array is true.

`p_req_array_alloc_elems`
:   The number of elements allocated for the
    array. Only valid if **p\_req\_valid** and ctrl->is\_array are
    true.

`p_req_elems`
:   The number of elements in **p\_req**. This is the same as
    ctrl->elems, except for dynamic arrays. In that case it is in
    the range of 1 to **p\_req\_array\_alloc\_elems**. Only valid if
    **p\_req\_valid** is true.

`p_req`
:   If the control handler containing this control reference
    is bound to a media request, then this points to the
    value of the control that must be applied when the request
    is executed, or to the value of the control at the time
    that the request was completed. If **p\_req\_valid** is false,
    then this control was never set for this request and the
    control will not be updated when this request is applied.

**Description**

Each control handler has a list of these refs. The list\_head is used to
keep a sorted-by-control-ID list of all controls, while the next pointer
is used to link the control in the hash’s bucket.

struct v4l2\_ctrl\_handler
:   The control handler keeps track of all the controls: both the controls owned by the handler and those inherited from other handlers.

> **Definition**:
>
> ```
> struct v4l2_ctrl_handler {
>       struct mutex _lock;
>       struct mutex *lock;
>       struct list_head ctrls;
>       struct list_head ctrl_refs;
>       struct v4l2_ctrl_ref *cached;
>       struct v4l2_ctrl_ref **buckets;
>       v4l2_ctrl_notify_fnc notify;
>       void *notify_priv;
>       u16 nr_of_buckets;
>       int error;
>       bool request_is_queued;
>       struct list_head requests;
>       struct list_head requests_queued;
>       struct media_request_object req_obj;
> };
> ```

**Members**

`_lock`
:   Default for “lock”.

`lock`
:   Lock to control access to this handler and its controls.
    May be replaced by the user right after init.

`ctrls`
:   The list of controls owned by this handler.

`ctrl_refs`
:   The list of control references.

`cached`
:   The last found control reference. It is common that the same
    control is needed multiple times, so this is a simple
    optimization.

`buckets`
:   Buckets for the hashing. Allows for quick control lookup.

`notify`
:   A notify callback that is called whenever the control changes
    value.
    Note that the handler’s lock is held when the notify function
    is called!

`notify_priv`
:   Passed as argument to the v4l2\_ctrl notify callback.

`nr_of_buckets`
:   Total number of buckets in the array.

`error`
:   The error code of the first failed control addition.

`request_is_queued`
:   True if the request was queued.

`requests`
:   List to keep track of open control handler request objects.
    For the parent control handler (**req\_obj.ops** == NULL) this
    is the list header. When the parent control handler is
    removed, it has to unbind and put all these requests since
    they refer to the parent.

`requests_queued`
:   List of the queued requests. This determines the order
    in which these controls are applied. Once the request is
    completed it is removed from this list.

`req_obj`
:   The [`struct media_request_object`](mc-core.html#c.media_request_object "media_request_object"), used to link into a
    [`struct media_request`](mc-core.html#c.media_request "media_request"). This request object has a refcount.

struct v4l2\_ctrl\_config
:   Control configuration structure.

> **Definition**:
>
> ```
> struct v4l2_ctrl_config {
>       const struct v4l2_ctrl_ops *ops;
>       const struct v4l2_ctrl_type_ops *type_ops;
>       u32 id;
>       const char *name;
>       enum v4l2_ctrl_type type;
>       s64 min;
>       s64 max;
>       u64 step;
>       s64 def;
>       union v4l2_ctrl_ptr p_def;
>       union v4l2_ctrl_ptr p_min;
>       union v4l2_ctrl_ptr p_max;
>       u32 dims[V4L2_CTRL_MAX_DIMS];
>       u32 elem_size;
>       u32 flags;
>       u64 menu_skip_mask;
>       const char * const *qmenu;
>       const s64 *qmenu_int;
>       unsigned int is_private:1;
> };
> ```

**Members**

`ops`
:   The control ops.

`type_ops`
:   The control type ops. Only needed for compound controls.

`id`
:   The control ID.

`name`
:   The control name.

`type`
:   The control type.

`min`
:   The control’s minimum value.

`max`
:   The control’s maximum value.

`step`
:   The control’s step value for non-menu controls.

`def`
:   The control’s default value.

`p_def`
:   The control’s default value for compound controls.

`p_min`
:   The control’s minimum value for compound controls.

`p_max`
:   The control’s maximum value for compound controls.

`dims`
:   The size of each dimension.

`elem_size`
:   The size in bytes of the control.

`flags`
:   The control’s flags.

`menu_skip_mask`
:   The control’s skip mask for menu controls. This makes it
    easy to skip menu items that are not valid. If bit X is set,
    then menu item X is skipped. Of course, this only works for
    menus with <= 64 menu items. There are no menus that come
    close to that number, so this is OK. Should we ever need more,
    then this will have to be extended to a bit array.

`qmenu`
:   A const char \* array for all menu items. Array entries that are
    empty strings (“”) correspond to non-existing menu items (this
    is in addition to the menu\_skip\_mask above). The last entry
    must be NULL.

`qmenu_int`
:   A const s64 integer array for all menu items of the type
    V4L2\_CTRL\_TYPE\_INTEGER\_MENU.

`is_private`
:   If set, then this control is private to its handler and it
    will not be added to any other handlers.

void v4l2\_ctrl\_fill(u32 id, const char \*\*name, enum v4l2\_ctrl\_type \*type, s64 \*min, s64 \*max, u64 \*step, s64 \*def, u32 \*flags)
:   Fill in the control fields based on the control ID.

**Parameters**

`u32 id`
:   ID of the control

`const char **name`
:   pointer to be filled with a string with the name of the control

`enum v4l2_ctrl_type *type`
:   pointer for storing the type of the control

`s64 *min`
:   pointer for storing the minimum value for the control

`s64 *max`
:   pointer for storing the maximum value for the control

`u64 *step`
:   pointer for storing the control step

`s64 *def`
:   pointer for storing the default value for the control

`u32 *flags`
:   pointer for storing the flags to be used on the control

**Description**

This works for all standard V4L2 controls.
For non-standard controls it will only fill in the given arguments
and **name** content will be set to `NULL`.

This function will overwrite the contents of **name**, **type** and **flags**.
The contents of **min**, **max**, **step** and **def** may be modified depending on
the type.

Note

Do not use in drivers! It is used internally for backwards compatibility
control handling only. Once all drivers are converted to use the new
control framework this function will no longer be exported.

int v4l2\_ctrl\_handler\_init\_class(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, unsigned int nr\_of\_controls\_hint, struct lock\_class\_key \*key, const char \*name)
:   Initialize the control handler.

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   The control handler.

`unsigned int nr_of_controls_hint`
:   A hint of how many controls this handler is
    expected to refer to. This is the total number, so including
    any inherited controls. It doesn’t have to be precise, but if
    it is way off, then you either waste memory (too many buckets
    are allocated) or the control lookup becomes slower (not enough
    buckets are allocated, so there are more slow list lookups).
    It will always work, though.

`struct lock_class_key *key`
:   Used by the lock validator if CONFIG\_LOCKDEP is set.

`const char *name`
:   Used by the lock validator if CONFIG\_LOCKDEP is set.

**Description**

Attention

Never use this call directly, always use the [`v4l2_ctrl_handler_init()`](#c.v4l2_ctrl_handler_init "v4l2_ctrl_handler_init")
macro that hides the **key** and **name** arguments.

**Return**

returns an error if the buckets could not be allocated. This
error will also be stored in **hdl->error**.

v4l2\_ctrl\_handler\_init

`v4l2_ctrl_handler_init (hdl, nr_of_controls_hint)`

> helper function to create a static struct `lock_class_key` and calls [`v4l2_ctrl_handler_init_class()`](#c.v4l2_ctrl_handler_init_class "v4l2_ctrl_handler_init_class")

**Parameters**

`hdl`
:   The control handler.

`nr_of_controls_hint`
:   A hint of how many controls this handler is
    expected to refer to. This is the total number, so including
    any inherited controls. It doesn’t have to be precise, but if
    it is way off, then you either waste memory (too many buckets
    are allocated) or the control lookup becomes slower (not enough
    buckets are allocated, so there are more slow list lookups).
    It will always work, though.

**Description**

This helper function creates a static struct `lock_class_key` and
calls [`v4l2_ctrl_handler_init_class()`](#c.v4l2_ctrl_handler_init_class "v4l2_ctrl_handler_init_class"), providing a proper name for the lock
validador.

Use this helper function to initialize a control handler.

int v4l2\_ctrl\_handler\_free(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl)
:   Free all controls owned by the handler and free the control list.

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   The control handler.

**Description**

Does nothing if **hdl** == NULL.

**Return**

**hdl**’s error field or 0 if **hdl** is NULL.

void v4l2\_ctrl\_lock(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl)
:   Helper function to lock the handler associated with the control.

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control to lock.

void v4l2\_ctrl\_unlock(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl)
:   Helper function to unlock the handler associated with the control.

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control to unlock.

int \_\_v4l2\_ctrl\_handler\_setup(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl)
:   Call the s\_ctrl op for all controls belonging to the handler to initialize the hardware to the current control values. The caller is responsible for acquiring the control handler mutex on behalf of [`__v4l2_ctrl_handler_setup()`](#c.__v4l2_ctrl_handler_setup "__v4l2_ctrl_handler_setup").

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   The control handler.

**Description**

Button controls will be skipped, as are read-only controls.

If **hdl** == NULL, then this just returns 0.

int v4l2\_ctrl\_handler\_setup(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl)
:   Call the s\_ctrl op for all controls belonging to the handler to initialize the hardware to the current control values.

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   The control handler.

**Description**

Button controls will be skipped, as are read-only controls.

If **hdl** == NULL, then this just returns 0.

void v4l2\_ctrl\_handler\_log\_status(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, const char \*prefix)
:   Log all controls owned by the handler.

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   The control handler.

`const char *prefix`
:   The prefix to use when logging the control values. If the
    prefix does not end with a space, then “: “ will be added
    after the prefix. If **prefix** == NULL, then no prefix will be
    used.

**Description**

For use with VIDIOC\_LOG\_STATUS.

Does nothing if **hdl** == NULL.

struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*v4l2\_ctrl\_new\_custom(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, const struct [v4l2\_ctrl\_config](#c.v4l2_ctrl_config "v4l2_ctrl_config") \*cfg, void \*priv)
:   Allocate and initialize a new custom V4L2 control.

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   The control handler.

`const struct v4l2_ctrl_config *cfg`
:   The control’s configuration data.

`void *priv`
:   The control’s driver-specific private data.

**Description**

If the [`v4l2_ctrl`](#c.v4l2_ctrl "v4l2_ctrl") `struct could` not be allocated then NULL is returned
and **hdl->error** is set to the error code (if it wasn’t set already).

struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*v4l2\_ctrl\_new\_std(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, const struct [v4l2\_ctrl\_ops](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") \*ops, u32 id, s64 min, s64 max, u64 step, s64 def)
:   Allocate and initialize a new standard V4L2 non-menu control.

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   The control handler.

`const struct v4l2_ctrl_ops *ops`
:   The control ops.

`u32 id`
:   The control ID.

`s64 min`
:   The control’s minimum value.

`s64 max`
:   The control’s maximum value.

`u64 step`
:   The control’s step value

`s64 def`
:   The control’s default value.

**Description**

If the [`v4l2_ctrl`](#c.v4l2_ctrl "v4l2_ctrl") `struct could` not be allocated, or the control
ID is not known, then NULL is returned and **hdl->error** is set to the
appropriate error code (if it wasn’t set already).

If **id** refers to a menu control, then this function will return NULL.

Use [`v4l2_ctrl_new_std_menu()`](#c.v4l2_ctrl_new_std_menu "v4l2_ctrl_new_std_menu") when adding menu controls.

struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*v4l2\_ctrl\_new\_std\_menu(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, const struct [v4l2\_ctrl\_ops](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") \*ops, u32 id, u8 max, u64 mask, u8 def)
:   Allocate and initialize a new standard V4L2 menu control.

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   The control handler.

`const struct v4l2_ctrl_ops *ops`
:   The control ops.

`u32 id`
:   The control ID.

`u8 max`
:   The control’s maximum value.

`u64 mask`
:   The control’s skip mask for menu controls. This makes it
    easy to skip menu items that are not valid. If bit X is set,
    then menu item X is skipped. Of course, this only works for
    menus with <= 64 menu items. There are no menus that come
    close to that number, so this is OK. Should we ever need more,
    then this will have to be extended to a bit array.

`u8 def`
:   The control’s default value.

**Description**

Same as [`v4l2_ctrl_new_std()`](#c.v4l2_ctrl_new_std "v4l2_ctrl_new_std"), but **min** is set to 0 and the **mask** value
determines which menu items are to be skipped.

If **id** refers to a non-menu control, then this function will return NULL.

struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*v4l2\_ctrl\_new\_std\_menu\_items(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, const struct [v4l2\_ctrl\_ops](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") \*ops, u32 id, u8 max, u64 mask, u8 def, const char \*const \*qmenu)
:   Create a new standard V4L2 menu control with driver specific menu.

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   The control handler.

`const struct v4l2_ctrl_ops *ops`
:   The control ops.

`u32 id`
:   The control ID.

`u8 max`
:   The control’s maximum value.

`u64 mask`
:   The control’s skip mask for menu controls. This makes it
    easy to skip menu items that are not valid. If bit X is set,
    then menu item X is skipped. Of course, this only works for
    menus with <= 64 menu items. There are no menus that come
    close to that number, so this is OK. Should we ever need more,
    then this will have to be extended to a bit array.

`u8 def`
:   The control’s default value.

`const char * const *qmenu`
:   The new menu.

**Description**

Same as [`v4l2_ctrl_new_std_menu()`](#c.v4l2_ctrl_new_std_menu "v4l2_ctrl_new_std_menu"), but **qmenu** will be the driver specific
menu of this control.

struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*v4l2\_ctrl\_new\_std\_compound(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, const struct [v4l2\_ctrl\_ops](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") \*ops, u32 id, const union [v4l2\_ctrl\_ptr](#c.v4l2_ctrl_ptr "v4l2_ctrl_ptr") p\_def, const union [v4l2\_ctrl\_ptr](#c.v4l2_ctrl_ptr "v4l2_ctrl_ptr") p\_min, const union [v4l2\_ctrl\_ptr](#c.v4l2_ctrl_ptr "v4l2_ctrl_ptr") p\_max)
:   Allocate and initialize a new standard V4L2 compound control.

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   The control handler.

`const struct v4l2_ctrl_ops *ops`
:   The control ops.

`u32 id`
:   The control ID.

`const union v4l2_ctrl_ptr p_def`
:   The control’s default value.

`const union v4l2_ctrl_ptr p_min`
:   The control’s minimum value.

`const union v4l2_ctrl_ptr p_max`
:   The control’s maximum value.

**Description**

Same as [`v4l2_ctrl_new_std()`](#c.v4l2_ctrl_new_std "v4l2_ctrl_new_std"), but with support for compound controls.
To fill in the **p\_def**, **p\_min** and **p\_max** fields, use [`v4l2_ctrl_ptr_create()`](#c.v4l2_ctrl_ptr_create "v4l2_ctrl_ptr_create")
to convert a pointer to a const [`union v4l2_ctrl_ptr`](#c.v4l2_ctrl_ptr "v4l2_ctrl_ptr").
Use v4l2\_ctrl\_ptr\_create(NULL) if you want the default, minimum or maximum
value of the compound control to be all zeroes.
If the compound control does not set the `V4L2_CTRL_FLAG_HAS_WHICH_MIN_MAX`
flag, then it does not has minimum and maximum values. In that case just use
v4l2\_ctrl\_ptr\_create(NULL) for the **p\_min** and **p\_max** arguments.

struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*v4l2\_ctrl\_new\_int\_menu(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, const struct [v4l2\_ctrl\_ops](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") \*ops, u32 id, u8 max, u8 def, const s64 \*qmenu\_int)
:   Create a new standard V4L2 integer menu control.

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   The control handler.

`const struct v4l2_ctrl_ops *ops`
:   The control ops.

`u32 id`
:   The control ID.

`u8 max`
:   The control’s maximum value.

`u8 def`
:   The control’s default value.

`const s64 *qmenu_int`
:   The control’s menu entries.

**Description**

Same as [`v4l2_ctrl_new_std_menu()`](#c.v4l2_ctrl_new_std_menu "v4l2_ctrl_new_std_menu"), but **mask** is set to 0 and it additionally
takes as an argument an array of integers determining the menu items.

If **id** refers to a non-integer-menu control, then this function will
return `NULL`.

v4l2\_ctrl\_filter
:   **Typedef**: Typedef to define the filter function to be used when adding a control handler.

**Syntax**

> `bool v4l2_ctrl_filter (const struct v4l2_ctrl *ctrl)`

**Parameters**

`const struct v4l2_ctrl *ctrl`
:   pointer to struct [`v4l2_ctrl`](#c.v4l2_ctrl "v4l2_ctrl").

int v4l2\_ctrl\_add\_handler(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*add, [v4l2\_ctrl\_filter](#c.v4l2_ctrl_filter "v4l2_ctrl_filter") filter, bool from\_other\_dev)
:   Add all controls from handler **add** to handler **hdl**.

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   The control handler.

`struct v4l2_ctrl_handler *add`
:   The control handler whose controls you want to add to
    the **hdl** control handler.

`v4l2_ctrl_filter filter`
:   This function will filter which controls should be added.

`bool from_other_dev`
:   If true, then the controls in **add** were defined in another
    device than **hdl**.

**Description**

Does nothing if either of the two handlers is a NULL pointer.
If **filter** is NULL, then all controls are added. Otherwise only those
controls for which **filter** returns true will be added.
In case of an error **hdl->error** will be set to the error code (if it
wasn’t set already).

bool v4l2\_ctrl\_radio\_filter(const struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl)
:   Standard filter for radio controls.

**Parameters**

`const struct v4l2_ctrl *ctrl`
:   The control that is filtered.

**Description**

This will return true for any controls that are valid for radio device
nodes. Those are all of the V4L2\_CID\_AUDIO\_\* user controls and all FM
transmitter class controls.

This function is to be used with [`v4l2_ctrl_add_handler()`](#c.v4l2_ctrl_add_handler "v4l2_ctrl_add_handler").

void v4l2\_ctrl\_cluster(unsigned int ncontrols, struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*\*controls)
:   Mark all controls in the cluster as belonging to that cluster.

**Parameters**

`unsigned int ncontrols`
:   The number of controls in this cluster.

`struct v4l2_ctrl **controls`
:   The cluster control array of size **ncontrols**.

void v4l2\_ctrl\_auto\_cluster(unsigned int ncontrols, struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*\*controls, u8 manual\_val, bool set\_volatile)
:   Mark all controls in the cluster as belonging to that cluster and set it up for autofoo/foo-type handling.

**Parameters**

`unsigned int ncontrols`
:   The number of controls in this cluster.

`struct v4l2_ctrl **controls`
:   The cluster control array of size **ncontrols**. The first control
    must be the ‘auto’ control (e.g. autogain, autoexposure, etc.)

`u8 manual_val`
:   The value for the first control in the cluster that equals the
    manual setting.

`bool set_volatile`
:   If true, then all controls except the first auto control will
    be volatile.

**Description**

Use for control groups where one control selects some automatic feature and
the other controls are only active whenever the automatic feature is turned
off (manual mode). Typical examples: autogain vs gain, auto-whitebalance vs
red and blue balance, etc.

The behavior of such controls is as follows:

When the autofoo control is set to automatic, then any manual controls
are set to inactive and any reads will call g\_volatile\_ctrl (if the control
was marked volatile).

When the autofoo control is set to manual, then any manual controls will
be marked active, and any reads will just return the current value without
going through g\_volatile\_ctrl.

In addition, this function will set the `V4L2_CTRL_FLAG_UPDATE` flag
on the autofoo control and `V4L2_CTRL_FLAG_INACTIVE` on the foo control(s)
if autofoo is in auto mode.

struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*v4l2\_ctrl\_find(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, u32 id)
:   Find a control with the given ID.

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   The control handler.

`u32 id`
:   The control ID to find.

**Description**

If **hdl** == NULL this will return NULL as well. Will lock the handler so
do not use from inside [`v4l2_ctrl_ops`](#c.v4l2_ctrl_ops "v4l2_ctrl_ops").

void v4l2\_ctrl\_activate(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, bool active)
:   Make the control active or inactive.

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control to (de)activate.

`bool active`
:   True if the control should become active.

**Description**

This sets or clears the V4L2\_CTRL\_FLAG\_INACTIVE flag atomically.
Does nothing if **ctrl** == NULL.
This will usually be called from within the s\_ctrl op.
The V4L2\_EVENT\_CTRL event will be generated afterwards.

This function assumes that the control handler is locked.

void \_\_v4l2\_ctrl\_grab(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, bool grabbed)
:   Unlocked variant of v4l2\_ctrl\_grab.

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control to (de)activate.

`bool grabbed`
:   True if the control should become grabbed.

**Description**

This sets or clears the V4L2\_CTRL\_FLAG\_GRABBED flag atomically.
Does nothing if **ctrl** == NULL.
The V4L2\_EVENT\_CTRL event will be generated afterwards.
This will usually be called when starting or stopping streaming in the
driver.

This function assumes that the control handler is locked by the caller.

void v4l2\_ctrl\_grab(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, bool grabbed)
:   Mark the control as grabbed or not grabbed.

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control to (de)activate.

`bool grabbed`
:   True if the control should become grabbed.

**Description**

This sets or clears the V4L2\_CTRL\_FLAG\_GRABBED flag atomically.
Does nothing if **ctrl** == NULL.
The V4L2\_EVENT\_CTRL event will be generated afterwards.
This will usually be called when starting or stopping streaming in the
driver.

This function assumes that the control handler is not locked and will
take the lock itself.

int \_\_v4l2\_ctrl\_modify\_range(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, s64 min, s64 max, u64 step, s64 def)
:   Unlocked variant of [`v4l2_ctrl_modify_range()`](#c.v4l2_ctrl_modify_range "v4l2_ctrl_modify_range")

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control to update.

`s64 min`
:   The control’s minimum value.

`s64 max`
:   The control’s maximum value.

`u64 step`
:   The control’s step value

`s64 def`
:   The control’s default value.

**Description**

Update the range of a control on the fly. This works for control types
INTEGER, BOOLEAN, MENU, INTEGER MENU and BITMASK. For menu controls the
**step** value is interpreted as a menu\_skip\_mask.

An error is returned if one of the range arguments is invalid for this
control type.

The caller is responsible for acquiring the control handler mutex on behalf
of [`__v4l2_ctrl_modify_range()`](#c.__v4l2_ctrl_modify_range "__v4l2_ctrl_modify_range").

int v4l2\_ctrl\_modify\_range(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, s64 min, s64 max, u64 step, s64 def)
:   Update the range of a control.

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control to update.

`s64 min`
:   The control’s minimum value.

`s64 max`
:   The control’s maximum value.

`u64 step`
:   The control’s step value

`s64 def`
:   The control’s default value.

**Description**

Update the range of a control on the fly. This works for control types
INTEGER, BOOLEAN, MENU, INTEGER MENU and BITMASK. For menu controls the
**step** value is interpreted as a menu\_skip\_mask.

An error is returned if one of the range arguments is invalid for this
control type.

This function assumes that the control handler is not locked and will
take the lock itself.

int \_\_v4l2\_ctrl\_modify\_dimensions(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, u32 dims[V4L2\_CTRL\_MAX\_DIMS])
:   Unlocked variant of [`v4l2_ctrl_modify_dimensions()`](#c.v4l2_ctrl_modify_dimensions "v4l2_ctrl_modify_dimensions")

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control to update.

`u32 dims[V4L2_CTRL_MAX_DIMS]`
:   The control’s new dimensions.

**Description**

Update the dimensions of an array control on the fly. The elements of the
array are reset to their default value, even if the dimensions are
unchanged.

An error is returned if **dims** is invalid for this control.

The caller is responsible for acquiring the control handler mutex on behalf
of [`__v4l2_ctrl_modify_dimensions()`](#c.__v4l2_ctrl_modify_dimensions "__v4l2_ctrl_modify_dimensions").

**Note**

calling this function when the same control is used in pending requests
is untested. It should work (a request with the wrong size of the control
will drop that control silently), but it will be very confusing.

int v4l2\_ctrl\_modify\_dimensions(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, u32 dims[V4L2\_CTRL\_MAX\_DIMS])
:   Update the dimensions of an array control.

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control to update.

`u32 dims[V4L2_CTRL_MAX_DIMS]`
:   The control’s new dimensions.

**Description**

Update the dimensions of an array control on the fly. The elements of the
array are reset to their default value, even if the dimensions are
unchanged.

An error is returned if **dims** is invalid for this control type.

This function assumes that the control handler is not locked and will
take the lock itself.

**Note**

calling this function when the same control is used in pending requests
is untested. It should work (a request with the wrong size of the control
will drop that control silently), but it will be very confusing.

void v4l2\_ctrl\_notify(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, [v4l2\_ctrl\_notify\_fnc](#c.v4l2_ctrl_notify_fnc "v4l2_ctrl_notify_fnc") notify, void \*priv)
:   Function to set a notify callback for a control.

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control.

`v4l2_ctrl_notify_fnc notify`
:   The callback function.

`void *priv`
:   The callback private handle, passed as argument to the callback.

**Description**

This function sets a callback function for the control. If **ctrl** is NULL,
then it will do nothing. If **notify** is NULL, then the notify callback will
be removed.

There can be only one notify. If another already exists, then a WARN\_ON
will be issued and the function will do nothing.

const char \*v4l2\_ctrl\_get\_name(u32 id)
:   Get the name of the control

**Parameters**

`u32 id`
:   The control ID.

**Description**

This function returns the name of the given control ID or NULL if it isn’t
a known control.

const char \*const \*v4l2\_ctrl\_get\_menu(u32 id)
:   Get the menu string array of the control

**Parameters**

`u32 id`
:   The control ID.

**Description**

This function returns the NULL-terminated menu string array name of the
given control ID or NULL if it isn’t a known menu control.

const s64 \*v4l2\_ctrl\_get\_int\_menu(u32 id, u32 \*len)
:   Get the integer menu array of the control

**Parameters**

`u32 id`
:   The control ID.

`u32 *len`
:   The size of the integer array.

**Description**

This function returns the integer array of the given control ID or NULL if it
if it isn’t a known integer menu control.

s32 v4l2\_ctrl\_g\_ctrl(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl)
:   Helper function to get the control’s value from within a driver.

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control.

**Description**

This returns the control’s value safely by going through the control
framework. This function will lock the control’s handler, so it cannot be
used from within the [`v4l2_ctrl_ops`](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") functions.

This function is for integer type controls only.

int \_\_v4l2\_ctrl\_s\_ctrl(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, s32 val)
:   Unlocked variant of [`v4l2_ctrl_s_ctrl()`](#c.v4l2_ctrl_s_ctrl "v4l2_ctrl_s_ctrl").

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control.

`s32 val`
:   The new value.

**Description**

This sets the control’s new value safely by going through the control
framework. This function assumes the control’s handler is already locked,
allowing it to be used from within the [`v4l2_ctrl_ops`](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") functions.

This function is for integer type controls only.

int v4l2\_ctrl\_s\_ctrl(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, s32 val)
:   Helper function to set the control’s value from within a driver.

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control.

`s32 val`
:   The new value.

**Description**

This sets the control’s new value safely by going through the control
framework. This function will lock the control’s handler, so it cannot be
used from within the [`v4l2_ctrl_ops`](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") functions.

This function is for integer type controls only.

s64 v4l2\_ctrl\_g\_ctrl\_int64(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl)
:   Helper function to get a 64-bit control’s value from within a driver.

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control.

**Description**

This returns the control’s value safely by going through the control
framework. This function will lock the control’s handler, so it cannot be
used from within the [`v4l2_ctrl_ops`](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") functions.

This function is for 64-bit integer type controls only.

int \_\_v4l2\_ctrl\_s\_ctrl\_int64(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, s64 val)
:   Unlocked variant of [`v4l2_ctrl_s_ctrl_int64()`](#c.v4l2_ctrl_s_ctrl_int64 "v4l2_ctrl_s_ctrl_int64").

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control.

`s64 val`
:   The new value.

**Description**

This sets the control’s new value safely by going through the control
framework. This function assumes the control’s handler is already locked,
allowing it to be used from within the [`v4l2_ctrl_ops`](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") functions.

This function is for 64-bit integer type controls only.

int v4l2\_ctrl\_s\_ctrl\_int64(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, s64 val)
:   Helper function to set a 64-bit control’s value from within a driver.

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control.

`s64 val`
:   The new value.

**Description**

This sets the control’s new value safely by going through the control
framework. This function will lock the control’s handler, so it cannot be
used from within the [`v4l2_ctrl_ops`](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") functions.

This function is for 64-bit integer type controls only.

int \_\_v4l2\_ctrl\_s\_ctrl\_string(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, const char \*s)
:   Unlocked variant of [`v4l2_ctrl_s_ctrl_string()`](#c.v4l2_ctrl_s_ctrl_string "v4l2_ctrl_s_ctrl_string").

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control.

`const char *s`
:   The new string.

**Description**

This sets the control’s new string safely by going through the control
framework. This function assumes the control’s handler is already locked,
allowing it to be used from within the [`v4l2_ctrl_ops`](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") functions.

This function is for string type controls only.

int v4l2\_ctrl\_s\_ctrl\_string(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, const char \*s)
:   Helper function to set a control’s string value from within a driver.

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control.

`const char *s`
:   The new string.

**Description**

This sets the control’s new string safely by going through the control
framework. This function will lock the control’s handler, so it cannot be
used from within the [`v4l2_ctrl_ops`](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") functions.

This function is for string type controls only.

int \_\_v4l2\_ctrl\_s\_ctrl\_compound(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, enum v4l2\_ctrl\_type type, const void \*p)
:   Unlocked variant to set a compound control

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control.

`enum v4l2_ctrl_type type`
:   The type of the data.

`const void *p`
:   The new compound payload.

**Description**

This sets the control’s new compound payload safely by going through the
control framework. This function assumes the control’s handler is already
locked, allowing it to be used from within the [`v4l2_ctrl_ops`](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") functions.

This function is for compound type controls only.

int v4l2\_ctrl\_s\_ctrl\_compound(struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, enum v4l2\_ctrl\_type type, const void \*p)
:   Helper function to set a compound control from within a driver.

**Parameters**

`struct v4l2_ctrl *ctrl`
:   The control.

`enum v4l2_ctrl_type type`
:   The type of the data.

`const void *p`
:   The new compound payload.

**Description**

This sets the control’s new compound payload safely by going through the
control framework. This function will lock the control’s handler, so it
cannot be used from within the [`v4l2_ctrl_ops`](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") functions.

This function is for compound type controls only.

void v4l2\_ctrl\_replace(struct v4l2\_event \*old, const struct v4l2\_event \*new)
:   Function to be used as a callback to [`struct v4l2_subscribed_event_ops`](v4l2-event.html#c.v4l2_subscribed_event_ops "v4l2_subscribed_event_ops") `replace()`

**Parameters**

`struct v4l2_event *old`
:   pointer to struct `v4l2_event` with the reported
    event;

`const struct v4l2_event *new`
:   pointer to struct `v4l2_event` with the modified
    event;

void v4l2\_ctrl\_merge(const struct v4l2\_event \*old, struct v4l2\_event \*new)
:   Function to be used as a callback to [`struct v4l2_subscribed_event_ops`](v4l2-event.html#c.v4l2_subscribed_event_ops "v4l2_subscribed_event_ops") `merge()`

**Parameters**

`const struct v4l2_event *old`
:   pointer to struct `v4l2_event` with the reported
    event;

`struct v4l2_event *new`
:   pointer to struct `v4l2_event` with the merged
    event;

int v4l2\_ctrl\_log\_status(struct [file](#c.v4l2_ctrl_log_status "file") \*file, void \*priv)
:   helper function to implement `VIDIOC_LOG_STATUS` ioctl

**Parameters**

`struct file *file`
:   pointer to [`struct file`](../../filesystems/api-summary.html#c.file "file")

`void *priv`
:   unused. Kept just to be compatible to the arguments expected by
    [`struct v4l2_ioctl_ops`](v4l2-common.html#c.v4l2_ioctl_ops "v4l2_ioctl_ops").vidioc\_log\_status.

**Description**

Can be used as a vidioc\_log\_status function that just dumps all controls
associated with the filehandle.

int v4l2\_ctrl\_subscribe\_event(struct [v4l2\_fh](v4l2-fh.html#c.v4l2_fh "v4l2_fh") \*fh, const struct v4l2\_event\_subscription \*sub)
:   Subscribes to an event

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

`const struct v4l2_event_subscription *sub`
:   pointer to `struct v4l2_event_subscription`

**Description**

Can be used as a vidioc\_subscribe\_event function that just subscribes
control events.

\_\_poll\_t v4l2\_ctrl\_poll(struct [file](#c.v4l2_ctrl_poll "file") \*file, struct poll\_table\_struct \*wait)
:   function to be used as a callback to the poll() That just polls for control events.

**Parameters**

`struct file *file`
:   pointer to [`struct file`](../../filesystems/api-summary.html#c.file "file")

`struct poll_table_struct *wait`
:   pointer to `struct poll_table_struct`

int v4l2\_ctrl\_request\_setup(struct [media\_request](mc-core.html#c.media_request "media_request") \*req, struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*parent)
:   helper function to apply control values in a request

**Parameters**

`struct media_request *req`
:   The request

`struct v4l2_ctrl_handler *parent`
:   The parent control handler (‘priv’ in [`media_request_object_find()`](mc-core.html#c.media_request_object_find "media_request_object_find"))

**Description**

This is a helper function to call the control handler’s s\_ctrl callback with
the control values contained in the request. Do note that this approach of
applying control values in a request is only applicable to memory-to-memory
devices.

void v4l2\_ctrl\_request\_complete(struct [media\_request](mc-core.html#c.media_request "media_request") \*req, struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*parent)
:   Complete a control handler request object

**Parameters**

`struct media_request *req`
:   The request

`struct v4l2_ctrl_handler *parent`
:   The parent control handler (‘priv’ in [`media_request_object_find()`](mc-core.html#c.media_request_object_find "media_request_object_find"))

**Description**

This function is to be called on each control handler that may have had a
request object associated with it, i.e. control handlers of a driver that
supports requests.

The function first obtains the values of any volatile controls in the control
handler and attach them to the request. Then, the function completes the
request object.

struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*v4l2\_ctrl\_request\_hdl\_find(struct [media\_request](mc-core.html#c.media_request "media_request") \*req, struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*parent)
:   Find the control handler in the request

**Parameters**

`struct media_request *req`
:   The request

`struct v4l2_ctrl_handler *parent`
:   The parent control handler (‘priv’ in [`media_request_object_find()`](mc-core.html#c.media_request_object_find "media_request_object_find"))

**Description**

This function finds the control handler in the request. It may return
NULL if not found. When done, you must call [`v4l2_ctrl_request_hdl_put()`](#c.v4l2_ctrl_request_hdl_put "v4l2_ctrl_request_hdl_put")
with the returned handler pointer.

If the request is not in state VALIDATING or QUEUED, then this function
will always return NULL.

Note that in state VALIDATING the req\_queue\_mutex is held, so
no objects can be added or deleted from the request.

In state QUEUED it is the driver that will have to ensure this.

void v4l2\_ctrl\_request\_hdl\_put(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl)
:   Put the control handler

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   Put this control handler

**Description**

This function released the control handler previously obtained from’
[`v4l2_ctrl_request_hdl_find()`](#c.v4l2_ctrl_request_hdl_find "v4l2_ctrl_request_hdl_find").

struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*v4l2\_ctrl\_request\_hdl\_ctrl\_find(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, u32 id)
:   Find a control with the given ID.

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   The control handler from the request.

`u32 id`
:   The ID of the control to find.

**Description**

This function returns a pointer to the control if this control is
part of the request or NULL otherwise.

int v4l2\_queryctrl(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, struct [v4l2\_queryctrl](#c.v4l2_queryctrl "v4l2_queryctrl") \*qc)
:   Helper function to implement [VIDIOC\_QUERYCTRL](../../userspace-api/media/v4l/vidioc-queryctrl.html#vidioc-queryctrl) ioctl

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   pointer to [`struct v4l2_ctrl_handler`](#c.v4l2_ctrl_handler "v4l2_ctrl_handler")

`struct v4l2_queryctrl *qc`
:   pointer to [`struct v4l2_queryctrl`](#c.v4l2_queryctrl "v4l2_queryctrl")

**Description**

If hdl == NULL then they will all return -EINVAL.

void v4l2\_query\_ext\_ctrl\_to\_v4l2\_queryctrl(struct [v4l2\_queryctrl](#c.v4l2_queryctrl "v4l2_queryctrl") \*to, const struct [v4l2\_query\_ext\_ctrl](#c.v4l2_query_ext_ctrl "v4l2_query_ext_ctrl") \*from)
:   Convert a qec to qe.

**Parameters**

`struct v4l2_queryctrl *to`
:   The v4l2\_queryctrl to write to.

`const struct v4l2_query_ext_ctrl *from`
:   The v4l2\_query\_ext\_ctrl to read from.

**Description**

This function is a helper to convert a v4l2\_query\_ext\_ctrl into a
v4l2\_queryctrl.

int v4l2\_query\_ext\_ctrl(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, struct [v4l2\_query\_ext\_ctrl](#c.v4l2_query_ext_ctrl "v4l2_query_ext_ctrl") \*qc)
:   Helper function to implement [VIDIOC\_QUERY\_EXT\_CTRL](../../userspace-api/media/v4l/vidioc-queryctrl.html#vidioc-queryctrl) ioctl

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   pointer to [`struct v4l2_ctrl_handler`](#c.v4l2_ctrl_handler "v4l2_ctrl_handler")

`struct v4l2_query_ext_ctrl *qc`
:   pointer to [`struct v4l2_query_ext_ctrl`](#c.v4l2_query_ext_ctrl "v4l2_query_ext_ctrl")

**Description**

If hdl == NULL then they will all return -EINVAL.

int v4l2\_querymenu(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, struct [v4l2\_querymenu](#c.v4l2_querymenu "v4l2_querymenu") \*qm)
:   Helper function to implement [VIDIOC\_QUERYMENU](../../userspace-api/media/v4l/vidioc-queryctrl.html#vidioc-queryctrl) ioctl

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   pointer to [`struct v4l2_ctrl_handler`](#c.v4l2_ctrl_handler "v4l2_ctrl_handler")

`struct v4l2_querymenu *qm`
:   pointer to [`struct v4l2_querymenu`](#c.v4l2_querymenu "v4l2_querymenu")

**Description**

If hdl == NULL then they will all return -EINVAL.

int v4l2\_g\_ctrl(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, struct v4l2\_control \*ctrl)
:   Helper function to implement [VIDIOC\_G\_CTRL](../../userspace-api/media/v4l/vidioc-g-ctrl.html#vidioc-g-ctrl) ioctl

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   pointer to [`struct v4l2_ctrl_handler`](#c.v4l2_ctrl_handler "v4l2_ctrl_handler")

`struct v4l2_control *ctrl`
:   pointer to `struct v4l2_control`

**Description**

If hdl == NULL then they will all return -EINVAL.

int v4l2\_s\_ctrl(struct [v4l2\_fh](v4l2-fh.html#c.v4l2_fh "v4l2_fh") \*fh, struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, struct v4l2\_control \*ctrl)
:   Helper function to implement [VIDIOC\_S\_CTRL](../../userspace-api/media/v4l/vidioc-g-ctrl.html#vidioc-g-ctrl) ioctl

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

`struct v4l2_ctrl_handler *hdl`
:   pointer to [`struct v4l2_ctrl_handler`](#c.v4l2_ctrl_handler "v4l2_ctrl_handler")

`struct v4l2_control *ctrl`
:   pointer to `struct v4l2_control`

**Description**

If hdl == NULL then they will all return -EINVAL.

int v4l2\_g\_ext\_ctrls(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, struct [video\_device](v4l2-dev.html#c.video_device "video_device") \*vdev, struct [media\_device](mc-core.html#c.media_device "media_device") \*mdev, struct v4l2\_ext\_controls \*c)
:   Helper function to implement [VIDIOC\_G\_EXT\_CTRLS](../../userspace-api/media/v4l/vidioc-g-ext-ctrls.html#vidioc-g-ext-ctrls) ioctl

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   pointer to [`struct v4l2_ctrl_handler`](#c.v4l2_ctrl_handler "v4l2_ctrl_handler")

`struct video_device *vdev`
:   pointer to [`struct video_device`](v4l2-dev.html#c.video_device "video_device")

`struct media_device *mdev`
:   pointer to [`struct media_device`](mc-core.html#c.media_device "media_device")

`struct v4l2_ext_controls *c`
:   pointer to `struct v4l2_ext_controls`

**Description**

If hdl == NULL then they will all return -EINVAL.

int v4l2\_try\_ext\_ctrls(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, struct [video\_device](v4l2-dev.html#c.video_device "video_device") \*vdev, struct [media\_device](mc-core.html#c.media_device "media_device") \*mdev, struct v4l2\_ext\_controls \*c)
:   Helper function to implement [VIDIOC\_TRY\_EXT\_CTRLS](../../userspace-api/media/v4l/vidioc-g-ext-ctrls.html#vidioc-g-ext-ctrls) ioctl

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   pointer to [`struct v4l2_ctrl_handler`](#c.v4l2_ctrl_handler "v4l2_ctrl_handler")

`struct video_device *vdev`
:   pointer to [`struct video_device`](v4l2-dev.html#c.video_device "video_device")

`struct media_device *mdev`
:   pointer to [`struct media_device`](mc-core.html#c.media_device "media_device")

`struct v4l2_ext_controls *c`
:   pointer to `struct v4l2_ext_controls`

**Description**

If hdl == NULL then they will all return -EINVAL.

int v4l2\_s\_ext\_ctrls(struct [v4l2\_fh](v4l2-fh.html#c.v4l2_fh "v4l2_fh") \*fh, struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, struct [video\_device](v4l2-dev.html#c.video_device "video_device") \*vdev, struct [media\_device](mc-core.html#c.media_device "media_device") \*mdev, struct v4l2\_ext\_controls \*c)
:   Helper function to implement [VIDIOC\_S\_EXT\_CTRLS](../../userspace-api/media/v4l/vidioc-g-ext-ctrls.html#vidioc-g-ext-ctrls) ioctl

**Parameters**

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

`struct v4l2_ctrl_handler *hdl`
:   pointer to [`struct v4l2_ctrl_handler`](#c.v4l2_ctrl_handler "v4l2_ctrl_handler")

`struct video_device *vdev`
:   pointer to [`struct video_device`](v4l2-dev.html#c.video_device "video_device")

`struct media_device *mdev`
:   pointer to [`struct media_device`](mc-core.html#c.media_device "media_device")

`struct v4l2_ext_controls *c`
:   pointer to `struct v4l2_ext_controls`

**Description**

If hdl == NULL then they will all return -EINVAL.

int v4l2\_ctrl\_subdev\_subscribe\_event(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd, struct [v4l2\_fh](v4l2-fh.html#c.v4l2_fh "v4l2_fh") \*fh, struct v4l2\_event\_subscription \*sub)
:   Helper function to implement as a [`struct v4l2_subdev_core_ops`](v4l2-subdev.html#c.v4l2_subdev_core_ops "v4l2_subdev_core_ops") subscribe\_event function that just subscribes control events.

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

`struct v4l2_fh *fh`
:   pointer to [`struct v4l2_fh`](v4l2-fh.html#c.v4l2_fh "v4l2_fh")

`struct v4l2_event_subscription *sub`
:   pointer to `struct v4l2_event_subscription`

int v4l2\_ctrl\_subdev\_log\_status(struct [v4l2\_subdev](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev") \*sd)
:   Log all controls owned by subdev’s control handler.

**Parameters**

`struct v4l2_subdev *sd`
:   pointer to [`struct v4l2_subdev`](v4l2-subdev.html#c.v4l2_subdev "v4l2_subdev")

int v4l2\_ctrl\_new\_fwnode\_properties(struct [v4l2\_ctrl\_handler](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") \*hdl, const struct [v4l2\_ctrl\_ops](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") \*ctrl\_ops, const struct [v4l2\_fwnode\_device\_properties](v4l2-fwnode.html#c.v4l2_fwnode_device_properties "v4l2_fwnode_device_properties") \*p)
:   Register controls for the device properties

**Parameters**

`struct v4l2_ctrl_handler *hdl`
:   pointer to [`struct v4l2_ctrl_handler`](#c.v4l2_ctrl_handler "v4l2_ctrl_handler") to register controls on

`const struct v4l2_ctrl_ops *ctrl_ops`
:   pointer to [`struct v4l2_ctrl_ops`](#c.v4l2_ctrl_ops "v4l2_ctrl_ops") to register controls with

`const struct v4l2_fwnode_device_properties *p`
:   pointer to [`struct v4l2_fwnode_device_properties`](v4l2-fwnode.html#c.v4l2_fwnode_device_properties "v4l2_fwnode_device_properties")

**Description**

This function registers controls associated to device properties, using the
property values contained in **p** parameter, if the property has been set to
a value.

Currently the following v4l2 controls are parsed and registered:
- V4L2\_CID\_CAMERA\_ORIENTATION
- V4L2\_CID\_CAMERA\_SENSOR\_ROTATION;

Controls already registered by the caller with the **hdl** control handler are
not overwritten. Callers should register the controls they want to handle
themselves before calling this function.

This function will set the control handler’s error field on failure, just as
other functions adding controls to the handler.

**Return**

0 on success, a negative error code on failure.

bool v4l2\_ctrl\_type\_op\_equal(const struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, union [v4l2\_ctrl\_ptr](#c.v4l2_ctrl_ptr "v4l2_ctrl_ptr") ptr1, union [v4l2\_ctrl\_ptr](#c.v4l2_ctrl_ptr "v4l2_ctrl_ptr") ptr2)
:   Default v4l2\_ctrl\_type\_ops equal callback.

**Parameters**

`const struct v4l2_ctrl *ctrl`
:   The v4l2\_ctrl pointer.

`union v4l2_ctrl_ptr ptr1`
:   A v4l2 control value.

`union v4l2_ctrl_ptr ptr2`
:   A v4l2 control value.

**Return**

true if values are equal, otherwise false.

void v4l2\_ctrl\_type\_op\_init(const struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, u32 from\_idx, union [v4l2\_ctrl\_ptr](#c.v4l2_ctrl_ptr "v4l2_ctrl_ptr") ptr)
:   Default v4l2\_ctrl\_type\_ops init callback.

**Parameters**

`const struct v4l2_ctrl *ctrl`
:   The v4l2\_ctrl pointer.

`u32 from_idx`
:   Starting element index.

`union v4l2_ctrl_ptr ptr`
:   The v4l2 control value.

**Return**

void

void v4l2\_ctrl\_type\_op\_log(const struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl)
:   Default v4l2\_ctrl\_type\_ops log callback.

**Parameters**

`const struct v4l2_ctrl *ctrl`
:   The v4l2\_ctrl pointer.

**Return**

void

int v4l2\_ctrl\_type\_op\_validate(const struct [v4l2\_ctrl](#c.v4l2_ctrl "v4l2_ctrl") \*ctrl, union [v4l2\_ctrl\_ptr](#c.v4l2_ctrl_ptr "v4l2_ctrl_ptr") ptr)
:   Default v4l2\_ctrl\_type\_ops validate callback.

**Parameters**

`const struct v4l2_ctrl *ctrl`
:   The v4l2\_ctrl pointer.

`union v4l2_ctrl_ptr ptr`
:   The v4l2 control value.

**Return**

0 on success, a negative error code on failure.
