#include "gst_helpers.h"

void scale_input_videos(GstreamerData * data, int output_width, int output_height);
void setup_video_mixer_pads(GstreamerData * data, int output_width, int output_height);

/* Manually clean unused Gst Elements if not streaming to Twitch */
/* TODO Create them on-demand instead of eagerly*/
void clean_unused_streaming_gst_elements(GstreamerData * data);

GstreamerData create_data()
{
    GstreamerData data;
    /* Create the elements */
    data.decodebin1         = gst_element_factory_make("uridecodebin3", "decodebin1");
    data.decodebin2         = gst_element_factory_make("uridecodebin3", "decodebin2");
    data.decodebin3         = gst_element_factory_make("uridecodebin3", "decodebin3");
    data.videoscale1        = gst_element_factory_make("videoscale", "videobox1");
    data.videoscale2        = gst_element_factory_make("videoscale", "videobox2");
    data.videoscale3        = gst_element_factory_make("videoscale", "videobox3");
    data.video_scaled_caps1 = gst_element_factory_make("capsfilter", "video_scaled_capsfilter1");
    data.video_scaled_caps2 = gst_element_factory_make("capsfilter", "video_scaled_capsfilter2");
    data.video_scaled_caps3 = gst_element_factory_make("capsfilter", "video_scaled_capsfilter3");
    data.video_mixer        = gst_element_factory_make("videomixer", "videomixer");

    data.tee = gst_element_factory_make("tee", "tee");

    /* Live preview */
    data.queue_preview   = gst_element_factory_make("queue", "queue_preview");
    data.convert_preview = gst_element_factory_make("videoconvert", "convert_preview");
    data.sink_preview    = gst_element_factory_make("autovideosink", "sink_preview");

    /* Twitch streaming */
    data.queue_streaming         = gst_element_factory_make("queue", "queue_streaming");
    data.video_encoder_streaming = gst_element_factory_make("x264enc", "encoder_streaming");
    data.queue_encoded           = gst_element_factory_make("queue", "queue_encoded");
    data.muxer_streaming         = gst_element_factory_make("flvmux", "muxer_streaming");
    data.queue_muxed             = gst_element_factory_make("queue", "queue_muxed");
    data.sink_rtmp               = gst_element_factory_make("rtmpsink", "sink_streaming");

    data.pipeline = gst_pipeline_new("pipeline");

    if (!data.pipeline || !data.decodebin1 || !data.decodebin2 || !data.decodebin3 //
        || !data.videoscale1 || !data.videoscale2 || !data.videoscale3 || !data.video_scaled_caps1
        || !data.video_scaled_caps2 || !data.video_scaled_caps3 || !data.video_mixer || !data.tee //
        || !data.convert_preview || !data.queue_preview || !data.sink_preview ||                  //
        !data.queue_streaming || !data.video_encoder_streaming || !data.queue_encoded || !data.muxer_streaming
        || !data.queue_muxed || !data.sink_rtmp) {
        g_printerr("Not all elements could be created.\n");
        exit(1);
    }

    return data;
}


void link_pipeline_elements(GstreamerData * data, gboolean with_twitch)
{
    g_return_if_fail(data != NULL);

    if (with_twitch == FALSE) { clean_unused_streaming_gst_elements(data); }

    gboolean error = FALSE;

    /* Add the common part of the pipeline */
    gst_bin_add_many(GST_BIN(data->pipeline),
                     data->decodebin1,
                     data->decodebin2,
                     data->decodebin3,
                     data->videoscale1,
                     data->videoscale2,
                     data->videoscale3,
                     data->video_scaled_caps1,
                     data->video_scaled_caps2,
                     data->video_scaled_caps3,
                     data->video_mixer,
                     data->convert_preview,
                     data->sink_preview,
                     NULL);

    if (!gst_element_link(data->videoscale1, data->video_scaled_caps1)
        || !gst_element_link(data->videoscale2, data->video_scaled_caps2)
        || !gst_element_link(data->videoscale3, data->video_scaled_caps3)) {
        error = TRUE;
    }

    if (with_twitch) {
        gst_bin_add_many(GST_BIN(data->pipeline),
                         data->tee,
                         data->queue_preview,
                         data->queue_streaming,
                         data->video_encoder_streaming,
                         data->queue_encoded,
                         data->muxer_streaming,
                         data->queue_muxed,
                         data->sink_rtmp,
                         NULL);

        g_print("Linking GStreamer elements for live preview and Twitch streaming .\n");
        if (!gst_element_link_many(data->video_mixer, data->tee, NULL)
            || !gst_element_link_many(data->tee,
                                      data->queue_streaming,
                                      data->video_encoder_streaming,
                                      data->queue_encoded,
                                      data->muxer_streaming,
                                      data->queue_muxed,
                                      data->sink_rtmp,
                                      NULL)
            || !gst_element_link_many(data->tee,
                                      data->queue_preview,
                                      data->convert_preview,
                                      data->sink_preview,
                                      NULL)) {
            error = TRUE;
        }
    }
    else {
        g_print("Linking the elements without Twitch streaming part.\n");
        if (!gst_element_link_many(data->video_mixer, data->convert_preview, data->sink_preview, NULL)) {
            error = TRUE;
        }
    }
    if (error) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(data->pipeline);
        exit(1);
    }
}

void setup_video_placement(GstreamerData * data, int output_width, int output_height)
{
    g_return_if_fail(data != NULL);

    g_object_set(data->video_mixer, "background", 1, NULL); /* set black background beneath */

    scale_input_videos(data, output_width, output_height);
    setup_video_mixer_pads(data, output_width, output_height);
}

void setup_file_sources(GstreamerData * data, gchar * file_path1, gchar * file_path2, gchar * filepath3)
{
    g_return_if_fail(data != NULL);
    g_return_if_fail(file_path1 != NULL || file_path2 != NULL || filepath3 != NULL);

    gchar * uri1 = g_strjoin("", "file://", file_path1, NULL);
    gchar * uri2 = g_strjoin("", "file://", file_path2, NULL);
    gchar * uri3 = g_strjoin("", "file://", filepath3, NULL);
    g_object_set(data->decodebin1, "uri", uri1, NULL);
    g_object_set(data->decodebin2, "uri", uri2, NULL);
    g_object_set(data->decodebin3, "uri", uri3, NULL);
    g_free(uri1);
    g_free(uri2);
    g_free(uri3);
}

void setup_twitch_streaming(GstreamerData * data, gchar * twitch_api_key, gchar * twitch_server)
{
    g_return_if_fail(data != NULL);
    g_return_if_fail(twitch_api_key != NULL || twitch_server != NULL);

    /* Set the parameters for the Twitch stream */
    g_object_set(data->video_encoder_streaming, "threads", 0, NULL);
    g_object_set(data->video_encoder_streaming, "bitrate", 400, NULL);
    g_object_set(data->video_encoder_streaming, "tune", 4, NULL);
    g_object_set(data->video_encoder_streaming, "key-int-max", 30, NULL);

    g_object_set(data->muxer_streaming, "streamable", TRUE, NULL);

    gchar * location = g_strjoin("", twitch_server, twitch_api_key, NULL);
    g_object_set(data->sink_rtmp, "location", location, NULL);
    g_free(location);
}

void clean_unused_streaming_gst_elements(GstreamerData * data)
{
    g_return_if_fail(data != NULL);
    gst_object_unref(data->tee);
    gst_object_unref(data->queue_preview);
    gst_object_unref(data->queue_streaming);
    gst_object_unref(data->video_encoder_streaming);
    gst_object_unref(data->queue_encoded);
    gst_object_unref(data->muxer_streaming);
    gst_object_unref(data->queue_muxed);
    gst_object_unref(data->sink_rtmp);
}

void try_change_pipeline_state(GstElement * pipeline, GstState state)
{
    GstStateChangeReturn ret = gst_element_set_state(pipeline, state);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Unable to set the pipeline to the paused state.\n");
        gst_object_unref(pipeline);
        exit(1);
    }
}

/* private functions' definitions */

void scale_input_videos(GstreamerData * data, int output_width, int output_height)
{
    GstCaps * videocaps_half = gst_caps_new_simple("video/x-raw",
                                                   "format",
                                                   G_TYPE_STRING,
                                                   "I420",
                                                   "framerate",
                                                   GST_TYPE_FRACTION,
                                                   25,
                                                   1,
                                                   "pixel-aspect-ratio",
                                                   GST_TYPE_FRACTION,
                                                   1,
                                                   1,
                                                   "width",
                                                   G_TYPE_INT,
                                                   output_width / 2,
                                                   "height",
                                                   G_TYPE_INT,
                                                   output_height / 2,
                                                   NULL);


    g_object_set(data->video_scaled_caps1, "caps", videocaps_half, NULL);
    g_object_set(data->video_scaled_caps2, "caps", videocaps_half, NULL);
    g_object_set(data->video_scaled_caps3, "caps", videocaps_half, NULL);
    gst_caps_unref(videocaps_half);
}

void setup_video_mixer_pads(GstreamerData * data, int output_width, int output_height)
{
    /* Manually link the videomixer, which has "Request" pads */
    GstPad *videomixer_pad1, *video1_pad = NULL;
    GstPad *videomixer_pad2, *video2_pad = NULL;
    GstPad *videomixer_pad3, *video3_pad = NULL;

    videomixer_pad1 = gst_element_get_request_pad(data->video_mixer, "sink_%u");
    videomixer_pad2 = gst_element_get_request_pad(data->video_mixer, "sink_%u");
    videomixer_pad3 = gst_element_get_request_pad(data->video_mixer, "sink_%u");

    video1_pad = gst_element_get_static_pad(data->video_scaled_caps1, "src");
    video2_pad = gst_element_get_static_pad(data->video_scaled_caps2, "src");
    video3_pad = gst_element_get_static_pad(data->video_scaled_caps3, "src");

    if (gst_pad_link(video1_pad, videomixer_pad1) != GST_PAD_LINK_OK
        || gst_pad_link(video2_pad, videomixer_pad2) != GST_PAD_LINK_OK
        || gst_pad_link(video3_pad, videomixer_pad3) != GST_PAD_LINK_OK) {
        g_printerr("Videomixer could not be linked.\n");
        gst_object_unref(data->pipeline);
        exit(1);
    }

    g_object_set(videomixer_pad1, "xpos", 0, NULL);
    g_object_set(videomixer_pad1, "ypos", output_height / 4, NULL);
    g_object_set(videomixer_pad2, "xpos", output_width / 2, NULL);
    g_object_set(videomixer_pad2, "ypos", 0, NULL);
    g_object_set(videomixer_pad3, "xpos", output_width / 2, NULL);
    g_object_set(videomixer_pad3, "ypos", output_height / 2, NULL);

    gst_object_unref(videomixer_pad1);
    gst_object_unref(videomixer_pad2);
    gst_object_unref(videomixer_pad3);
    gst_object_unref(video1_pad);
    gst_object_unref(video2_pad);
    gst_object_unref(video3_pad);
}
