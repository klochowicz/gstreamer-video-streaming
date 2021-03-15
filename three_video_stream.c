#include <gst/gstpipeline.h>

/**
 * SECTION:three_video_stream
 * @title: ThreeVideoStream
 * @short_description: Object short description
 * @include: three_video_stream.h
 *
 * Long description
 *
 */

#include "gst_helpers.h"
#include "three_video_stream.h"

struct _ThreeVideoStreamPrivate {
    gchar *       file_path1;
    gchar *       file_path2;
    gchar *       file_path3;
    gchar *       twitch_api_key;
    gchar *       twitch_server;
    int           output_width;
    int           output_height;
    gboolean      ready_to_play;
    GstreamerData gstreamer_data;
};

enum {
    PROP_0,
    PROP_FILEPATH1,
    PROP_FILEPATH2,
    PROP_FILEPATH3,
    PROP_TWITCH_API_KEY,
    PROP_TWITCH_SERVER,
    PROP_READY_TO_PLAY,
    PROP_OUTPUT_WIDTH,
    PROP_OUTPUT_HEIGHT,
    PROP_GST_PIPELINE,
    PROP_SIZE,
};

/* This object is a child of GObject */
G_DEFINE_TYPE_WITH_CODE(ThreeVideoStream, three_video_stream, G_TYPE_OBJECT, G_ADD_PRIVATE(ThreeVideoStream))

static void cb_pad_added(GstElement * src, GstPad * new_pad, GstreamerData * data);


/* TODO allow changing at runtime */
void configure_gst_pipeline(ThreeVideoStreamPrivate * priv)
{
    gboolean link_with_twitch = strlen(priv->twitch_api_key) != 0;

    link_pipeline_elements(&priv->gstreamer_data, link_with_twitch);
    setup_video_placement(&priv->gstreamer_data, priv->output_width, priv->output_height);

    setup_file_sources(&priv->gstreamer_data, priv->file_path1, priv->file_path2, priv->file_path3);

    if (link_with_twitch) { setup_twitch_streaming(&priv->gstreamer_data, priv->twitch_api_key, priv->twitch_server); }

    g_signal_connect(priv->gstreamer_data.decodebin1, "pad-added", G_CALLBACK(cb_pad_added), &priv->gstreamer_data);
    g_signal_connect(priv->gstreamer_data.decodebin2, "pad-added", G_CALLBACK(cb_pad_added), &priv->gstreamer_data);
    g_signal_connect(priv->gstreamer_data.decodebin3, "pad-added", G_CALLBACK(cb_pad_added), &priv->gstreamer_data);
}

static void three_video_stream_init(ThreeVideoStream * self)
{
    self->priv                 = three_video_stream_get_instance_private(self);
    self->priv->gstreamer_data = create_data();
    self->priv->ready_to_play  = FALSE;
}

static void _three_video_stream_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
    ThreeVideoStream * self = THREE_VIDEO_STREAM(object);
    g_return_if_fail(IS_THREE_VIDEO_STREAM(object));

    switch (prop_id) {
    case PROP_FILEPATH1:
        g_free(self->priv->file_path1);
        self->priv->file_path1 = g_value_dup_string(value);
        break;
    case PROP_FILEPATH2:
        g_free(self->priv->file_path2);
        self->priv->file_path2 = g_value_dup_string(value);
        break;
    case PROP_FILEPATH3:
        g_free(self->priv->file_path3);
        self->priv->file_path3 = g_value_dup_string(value);
        break;
    case PROP_TWITCH_API_KEY:
        g_free(self->priv->twitch_api_key);
        self->priv->twitch_api_key = g_value_dup_string(value);
        break;
    case PROP_TWITCH_SERVER:
        g_free(self->priv->twitch_server);
        self->priv->twitch_server = g_value_dup_string(value);
        break;
    case PROP_READY_TO_PLAY: {
        gboolean changed;
        gboolean ready_to_play = g_value_get_boolean(value);
        changed                = (self->priv->ready_to_play != ready_to_play);

        self->priv->ready_to_play = ready_to_play;
        if (changed) {
            if (ready_to_play) {
                g_print("Starting the stream...");
                configure_gst_pipeline(self->priv);
            }
            else {
                g_print("Stopping the stream...");
            }
        }
        break;
    }
    case PROP_OUTPUT_WIDTH: self->priv->output_width = g_value_get_int(value); break;
    case PROP_OUTPUT_HEIGHT: self->priv->output_height = g_value_get_int(value); break;
    case PROP_GST_PIPELINE: g_printerr("Cannot change gst-pipeline property\n"); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec); break;
    }
}

static void _three_video_stream_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
    ThreeVideoStream * self = THREE_VIDEO_STREAM(object);
    g_return_if_fail(IS_THREE_VIDEO_STREAM(object));

    switch (prop_id) {
    case PROP_FILEPATH1: g_value_set_string(value, self->priv->file_path1); break;
    case PROP_FILEPATH2: g_value_set_string(value, self->priv->file_path2); break;
    case PROP_FILEPATH3: g_value_set_string(value, self->priv->file_path3); break;
    case PROP_TWITCH_API_KEY: g_value_set_string(value, self->priv->twitch_api_key); break;
    case PROP_TWITCH_SERVER: g_value_set_string(value, self->priv->twitch_server); break;
    case PROP_READY_TO_PLAY: g_value_set_boolean(value, self->priv->ready_to_play); break;
    case PROP_OUTPUT_WIDTH: g_value_set_int(value, self->priv->output_width); break;
    case PROP_OUTPUT_HEIGHT: g_value_set_int(value, self->priv->output_height); break;
    case PROP_GST_PIPELINE: {
        g_value_set_object(value, self->priv->gstreamer_data.pipeline);
        break;
    }
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec); break;
    }
}

static void _three_video_stream_dispose(GObject * object)
{
    /* Chain up : end */
    G_OBJECT_CLASS(three_video_stream_parent_class)->dispose(object);
}

static void _three_video_stream_finalize(GObject * object)
{
    ThreeVideoStream * self = THREE_VIDEO_STREAM(object);

    if (self->priv->gstreamer_data.pipeline != NULL) { g_object_unref(self->priv->gstreamer_data.pipeline); }

    g_free(self->priv->file_path1);
    g_free(self->priv->file_path2);
    g_free(self->priv->file_path3);
    g_free(self->priv->twitch_api_key);
    g_free(self->priv->twitch_server);

    /* Chain up : end */
    G_OBJECT_CLASS(three_video_stream_parent_class)->finalize(object);
}

static void three_video_stream_class_init(ThreeVideoStreamClass * klass)
{
    GObjectClass * object_class = G_OBJECT_CLASS(klass);

    object_class->set_property = &_three_video_stream_set_property;
    object_class->get_property = &_three_video_stream_get_property;
    object_class->dispose      = &_three_video_stream_dispose;
    object_class->finalize     = &_three_video_stream_finalize;

    g_object_class_install_property(object_class,
                                    PROP_FILEPATH1,
                                    g_param_spec_string("file-path1",
                                                        NULL,
                                                        "Path to load the first video (for left part of the screen)",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_NAME
                                                            | G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_FILEPATH2,
                                    g_param_spec_string("file-path2",
                                                        NULL,
                                                        "Path to load the second video (for top-right part of the "
                                                        "screen)",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_NAME
                                                            | G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_FILEPATH3,
                                    g_param_spec_string("file-path3",
                                                        NULL,
                                                        "Path to load the third video (for bottom-right part of the "
                                                        "screen)",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_NAME
                                                            | G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_TWITCH_API_KEY,
                                    g_param_spec_string("twitch-api-key",
                                                        NULL,
                                                        "Twitch API key",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_NAME
                                                            | G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_TWITCH_SERVER,
                                    g_param_spec_string("twitch-server",
                                                        NULL,
                                                        "Twitch ingest server",
                                                        "rtmp://syd01.contribute.live-video.net/app/",
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_NAME
                                                            | G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_READY_TO_PLAY,
                                    g_param_spec_boolean("ready-to-play",
                                                         NULL,
                                                         "Signal that any optional configuration has been finished",
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_NAME
                                                             | G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_OUTPUT_WIDTH,
                                    g_param_spec_int("output-width",
                                                     NULL,
                                                     "Output video width",
                                                     320,
                                                     1920,
                                                     1920,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_NAME
                                                         | G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_OUTPUT_HEIGHT,
                                    g_param_spec_int("output-height",
                                                     NULL,
                                                     "Output video width",
                                                     240,
                                                     1080,
                                                     1080,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_NAME
                                                         | G_PARAM_STATIC_BLURB));


    g_object_class_install_property(object_class,
                                    PROP_GST_PIPELINE,
                                    g_param_spec_object("gst-pipeline",
                                                        "pipeline",
                                                        "Underlying GStreamer pipeline",
                                                        GST_TYPE_ELEMENT,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

/* METHODS */

/**
 * three_video_stream_new:
 * @file-path1: path to first video
 * @file-path2: path to second video
 * @file-path3: path to third video
 * @twitch-api-key: twitch API key
 *
 * A simple constructor.
 *
 * Returns: (transfer full): a newly created #ThreeVideoStream
 */
ThreeVideoStream *
three_video_stream_new(gchar * file_path1, gchar * file_path2, gchar * file_path3, gchar * twitch_api_key)
{
    ThreeVideoStream * three_video_stream = g_object_new(THREE_VIDEO_STREAM_TYPE_NAME,
                                                         "file-path1",
                                                         file_path1,
                                                         "file-path2",
                                                         file_path2,
                                                         "file-path3",
                                                         file_path3,
                                                         "twitch-api-key",
                                                         twitch_api_key,
                                                         NULL);

    return three_video_stream;
}

/**
 * three_video_stream_ref:
 * @three_video_stream: a #ThreeVideoStream
 *
 * Increase reference count by one.
 *
 * Returns: (transfer full): @three_video_stream.
 */
ThreeVideoStream * three_video_stream_ref(ThreeVideoStream * three_video_stream)
{
    return g_object_ref(three_video_stream);
}

/**
 * three_video_stream_free:
 * @three_video_stream: a #ThreeVideoStream
 *
 * Decrease reference count by one.
 *
 */
void three_video_stream_free(ThreeVideoStream * three_video_stream)
{
    g_object_unref(three_video_stream);
}

/**
 * three_video_stream_clear:
 * @three_video_stream: a #ThreeVideoStream
 *
 * Decrease reference count by one if *@three_video_stream != NULL
 * and sets @three_video_stream to NULL.
 *
 */
void three_video_stream_clear(ThreeVideoStream ** three_video_stream)
{
    g_clear_object(three_video_stream);
}

static void cb_pad_added(GstElement * src, GstPad * new_pad, GstreamerData * data)
{
    GstPad *         sink_pad     = NULL;
    GstCaps *        new_pad_caps = NULL;
    gboolean         skip_linking = FALSE;
    gchar *          src_name     = gst_element_get_name(src);
    gchar *          new_pad_name = gst_pad_get_name(new_pad);
    GstPadLinkReturn ret;

    g_print("Received new pad '%s' from '%s':\n", new_pad_name, src_name);

    if (GST_IS_PAD(sink_pad) && gst_pad_is_linked(sink_pad)) {
        g_print("We are already linked. Ignoring.\n");
        skip_linking = TRUE;
    }
    else if (g_str_has_prefix(new_pad_name, "audio")) {
        g_print(" Found audio pad, ignoring as audio is currently unsupported.\n");
        skip_linking = TRUE;
    }
    else if (g_str_has_prefix(new_pad_name, "video")) {
        g_print(" Found video pad. Plugging it into the videomixer.\n");
        if (strcmp(src_name, "decodebin1") == 0) { sink_pad = gst_element_get_static_pad(data->videoscale1, "sink"); }
        else if (strcmp(src_name, "decodebin2") == 0) {
            sink_pad = gst_element_get_static_pad(data->videoscale2, "sink");
        }
        else if (strcmp(src_name, "decodebin3") == 0) {
            sink_pad = gst_element_get_static_pad(data->videoscale3, "sink");
        }
        else {
            g_error("Unexpected element name '%s'.\n", src_name);
        }
    }

    if (!skip_linking) {
        /* Attempt the link */
        ret = gst_pad_link(new_pad, sink_pad);
        if (GST_PAD_LINK_FAILED(ret)) { g_printerr("Pad link link failed.\n"); }
        else {
            g_print("Pad link succeeded.\n");
        }
    }

    if (new_pad_caps != NULL) { gst_caps_unref(new_pad_caps); }
    if (sink_pad != NULL) { g_object_unref(sink_pad); }
    g_free(src_name);
    g_free(new_pad_name);
}
