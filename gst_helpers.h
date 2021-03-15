#ifndef _GST_HELPERS__H_
#define _GST_HELPERS__H_

#include <gst/gst.h>

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _GstreamerData {
    GstElement * pipeline;
    GstElement * decodebin1;
    GstElement * decodebin2;
    GstElement * decodebin3;
    GstElement * videoscale1;
    GstElement * videoscale2;
    GstElement * videoscale3;
    GstElement * video_scaled_caps1;
    GstElement * video_scaled_caps2;
    GstElement * video_scaled_caps3;
    GstElement * video_mixer;
    GstElement * convert_preview;
    GstElement * sink_preview;
    // XXX Do not call the following if Twitch is not setup
    GstElement * tee;
    GstElement * queue_preview;
    GstElement * queue_streaming;
    GstElement * video_encoder_streaming;
    GstElement * queue_encoded;
    GstElement * muxer_streaming;
    GstElement * queue_muxed;
    GstElement * sink_rtmp;
} GstreamerData;

/* A factory function that creates all necessary GstElements, struct */
/* Exits on error. */
GstreamerData create_data();

void link_pipeline_elements(GstreamerData * data, gboolean with_twitch);

void setup_video_placement(GstreamerData * data, int output_width, int output_height);

void setup_file_sources(GstreamerData * data, gchar * filepath1, gchar * filepath2, gchar * filepath3);

void setup_twitch_streaming(GstreamerData * data, gchar * twitch_api_key, gchar * twich_server);

void clean_unused_streaming_gst_elements(GstreamerData * data);

/* Try to change pipeline state to desired state */
/* Exits the program if request cannot be fulfilled */
void try_change_pipeline_state(GstElement * pipeline, GstState state);

#endif /* _GST_HELPERS__H_ */
