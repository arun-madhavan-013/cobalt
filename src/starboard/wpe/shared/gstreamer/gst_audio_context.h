#ifndef STARBOARD_SHARED_GSTREAMER_AUDIO_CONTEXT_H_
#define STARBOARD_SHARED_GSTREAMER_AUDIO_CONTEXT_H_

#include "starboard/thread.h"

#include "gst/gst.h"
#include "gst/app/gstappsrc.h"
#include "gst/app/gstappsink.h"

namespace starboard {
namespace shared {
namespace gstreamer {

#define USE_PLAYBIN 1

class AudioContext {

public:
    AudioContext(int32_t number_of_channels);
    ~AudioContext();

    void SetDecoder(void *audio_decoder);
    void *GetDecoder();
    void SetPlay();
    void SetReady();

private:
    static gboolean BusCallback (GstBus *bus,
            GstMessage *message, gpointer *ptr);
    static void* MainThread (void* context);
    static void StartFeed (GstElement *pipeline, guint size, void *context);
    static void StopFeed (GstElement *pipeline, void *context);
#ifndef USE_PLAYBIN
    static void OnPadAdded (GstElement *element, GstPad *pad, void *context);
#endif
    static gboolean ReadData (void *context);
    static GstFlowReturn NewSample (GstElement *sink, void *context);

#ifdef USE_PLAYBIN
    static void SourceSetup(
            GstElement *pipeline, GstElement *source, void *context);
#endif

    SbThread main_thread_;
    GMainLoop *loop;

    GstPipeline *pipeline;
    GstAppSrc *src;
#ifndef USE_PLAYBIN
    GstElement *decoder;
    GstElement *convert;
    GstElement *resample;
    GstElement *capsfilter;
#endif
    GstElement *appsink;
    guint sourceid;
    void *audio_decoder;
};

}  // namespace gstreamer
}  // namespace shared
}  // namespace starboard

#endif
