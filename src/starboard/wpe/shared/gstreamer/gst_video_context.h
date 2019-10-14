#ifndef STARBOARD_WPE_SHARED_GSTREAMER_VIDEO_CONTEXT_H_
#define STARBOARD_WPE_SHARED_GSTREAMER_VIDEO_CONTEXT_H_

#include "starboard/common/ref_counted.h"
#include "starboard/common/scoped_ptr.h"
#include <queue>

#include "starboard/media.h"
#include "starboard/thread.h"
#include "gst/gst.h"
#include "gst/app/gstappsrc.h"
#include "gst/app/gstappsink.h"

namespace starboard {
namespace wpe {
namespace shared {
namespace gstreamer {

#define USE_PLAYBIN 1

class VideoSample : public RefCountedThreadSafe<VideoSample> {

public:
    VideoSample(GstSample *sample) {
        sample_ = sample;
    }
    ~VideoSample() {
        gst_sample_unref(sample_);
    }
    GstSample *sample() {
        return sample_;
    }

private:
    GstSample *sample_;
};

class VideoContext {

public:
    VideoContext(SbMediaVideoCodec video_codec);
    ~VideoContext();

    void SetDecoder(void *video_decoder);
    void *GetDecoder();
    void SetPlay();
    void SetReady();
    gboolean fetchOutputBuffer();
    void updateState();

private:
    static gboolean BusCallback(GstBus *bus,
            GstMessage *message, gpointer *ptr);
    static void* MainThread(void* context);
    static void StartFeed(GstElement *pipeline, guint size, void *context);
    static void StopFeed(GstElement *pipeline, void *context);
    static gboolean ReadData(void *context);
    static GstFlowReturn NewSample(GstElement *sink, void *context);

#ifdef USE_PLAYBIN
    static void SourceSetup(
            GstElement *pipeline, GstElement *source, void *context);
#endif

    SbThread main_thread_;
    GMainLoop *loop;

    GstPipeline *pipeline;
    GstAppSrc *src;
#ifndef USE_PLAYBIN
    GstElement *capsfilter;
    GstElement *decoder;
    GstElement *parser;
    GstElement *queue;
#endif
    GstElement *appsink;
    guint sourceid;
    void *video_decoder;
    SbMediaVideoCodec video_codec;

    std::queue<scoped_refptr<VideoSample> > video_samples_;
};

}  // namespace gstreamer
}  // namespace shared
}  // namespace wpe
}  // namespace starboard

#endif
