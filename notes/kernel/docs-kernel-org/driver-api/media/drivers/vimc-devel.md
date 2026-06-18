# 10.1.12.The Virtual Media Controller Driver (vimc)

> 출처(원문): https://docs.kernel.org/driver-api/media/drivers/vimc-devel.html
> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)

---

# 10.1.12. The Virtual Media Controller Driver (vimc)

## 10.1.12.1. Source code documentation

### 10.1.12.1.1. vimc-streamer

struct vimc\_stream
:   struct that represents a stream in the pipeline

**Definition**:

```
struct vimc_stream {
    struct media_pipeline pipe;
    struct vimc_ent_device *ved_pipeline[VIMC_STREAMER_PIPELINE_MAX_SIZE];
    unsigned int pipe_size;
    struct task_struct *kthread;
};
```

**Members**

`pipe`
:   the media pipeline object associated with this stream

`ved_pipeline`
:   array containing all the entities participating in the
    stream. The order is from a video device (usually a
    capture device) where stream\_on was called, to the
    entity generating the first base image to be
    processed in the pipeline.

`pipe_size`
:   size of **ved\_pipeline**

`kthread`
:   thread that generates the frames of the stream.

**Description**

When the user call stream\_on in a video device, [`struct vimc_stream`](#c.vimc_stream "vimc_stream") is
used to keep track of all entities and subdevices that generates and
process frames for the stream.

struct [media\_entity](../mc-core.html#c.media_entity "media_entity") \*vimc\_get\_source\_entity(struct [media\_entity](../mc-core.html#c.media_entity "media_entity") \*ent)
:   get the entity connected with the first sink pad

**Parameters**

`struct media_entity *ent`
:   reference media\_entity

**Description**

Helper function that returns the media entity containing the source pad
linked with the first sink pad from the given media entity pad list.

**Return**

The source pad or NULL, if it wasn’t found.

void vimc\_streamer\_pipeline\_terminate(struct [vimc\_stream](#c.vimc_stream "vimc_stream") \*stream)
:   Disable stream in all ved in stream

**Parameters**

`struct vimc_stream *stream`
:   the pointer to the stream structure with the pipeline to be
    disabled.

**Description**

Calls s\_stream to disable the stream in each entity of the pipeline

int vimc\_streamer\_pipeline\_init(struct [vimc\_stream](#c.vimc_stream "vimc_stream") \*stream, struct vimc\_ent\_device \*ved)
:   Initializes the stream structure

**Parameters**

`struct vimc_stream *stream`
:   the pointer to the stream structure to be initialized

`struct vimc_ent_device *ved`
:   the pointer to the vimc entity initializing the stream

**Description**

Initializes the stream structure. Walks through the entity graph to
construct the pipeline used later on the streamer thread.
Calls [`vimc_streamer_s_stream()`](#c.vimc_streamer_s_stream "vimc_streamer_s_stream") to enable stream in all entities of
the pipeline.

**Return**

0 if success, error code otherwise.

struct vimc\_sensor\_device \*vimc\_streamer\_get\_sensor(struct [vimc\_stream](#c.vimc_stream "vimc_stream") \*stream)
:   Get sensor from pipeline

**Parameters**

`struct vimc_stream *stream`
:   the pipeline

**Description**

Helper function to find the sensor device in the pipeline.
Returns pointer to sensor device or NULL if not found.

int vimc\_streamer\_thread(void \*data)
:   Process frames through the pipeline

**Parameters**

`void *data`
:   vimc\_stream `struct of` the current stream

**Description**

From the source to the sink, gets a frame from each subdevice and send to
the next one of the pipeline at a fixed framerate.

**Return**

Always zero (created as `int` instead of `void` to comply with
kthread API).

int vimc\_streamer\_s\_stream(struct [vimc\_stream](#c.vimc_stream "vimc_stream") \*stream, struct vimc\_ent\_device \*ved, int enable)
:   Start/stop the streaming on the media pipeline

**Parameters**

`struct vimc_stream *stream`
:   the pointer to the stream structure of the current stream

`struct vimc_ent_device *ved`
:   pointer to the vimc entity of the entity of the stream

`int enable`
:   flag to determine if stream should start/stop

**Description**

When starting, check if there is no `stream->kthread` allocated. This
should indicate that a stream is already running. Then, it initializes the
pipeline, creates and runs a kthread to consume buffers through the pipeline.
When stopping, analogously check if there is a stream running, stop the
thread and terminates the pipeline.

**Return**

0 if success, error code otherwise.
