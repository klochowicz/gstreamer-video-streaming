#include "gst_helpers.h"
#include "three_video_stream.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <gmodule.h>
#include <gst/gst.h>
#include <stdlib.h>

/* Input parameters */
static gchar * twitch_api_key  = "";
static gchar * twitch_server   = "";
static gchar * video1_filename = "";
static gchar * video2_filename = "";
static gchar * video3_filename = "";
static int     output_width    = 1920;
static int     output_height   = 1080;

static GOptionEntry entries[7] = {
    {"twitch-api-key",
     'k',
     0,
     G_OPTION_ARG_STRING,
     &twitch_api_key,
     "Twitch Stream API key (only local playback without it)",
     NULL},
    {"twitch-server",
     's',
     0,
     G_OPTION_ARG_STRING,
     &twitch_server,
     "Choose preferred Twitch ingest server (optional)",
     NULL},
    {"video-a", 'a', 0, G_OPTION_ARG_FILENAME, &video1_filename, "First video (left half of the screen)", NULL},
    {"video-b", 'b', 0, G_OPTION_ARG_FILENAME, &video2_filename, "Second video (top right of the screen)", NULL},
    {"video-c", 'c', 0, G_OPTION_ARG_FILENAME, &video3_filename, "Third video (bottom right of the screen)", NULL},
    {"width", 'w', 0, G_OPTION_ARG_INT, &output_width, "Output video width", NULL},
    {"height", 'h', 0, G_OPTION_ARG_INT, &output_height, "Output video height", NULL},
};

static GMainLoop *        loop;
static GstElement *       pipeline;
static ThreeVideoStream * three_video_stream;
static GstBus *           bus;

void sig_int_handler(int unused);

void parse_args(gint * argc, gchar ** argv[]);
void show_confirmation_prompt();
void verify_parsed_arguments();

static void cleanup();

/* Handler for the GStreamer pipeline bus message */
static gboolean cb_on_bus_message(GstBus * bus, GstMessage * message, gpointer user_data);

int main(int argc, char * argv[])
{
    parse_args(&argc, &argv);
    verify_parsed_arguments();

    signal(SIGINT, sig_int_handler);

    gst_init(&argc, &argv);

    three_video_stream = three_video_stream_new(video1_filename, video2_filename, video3_filename, twitch_api_key);

    if (strlen(twitch_server) != 0) {
        g_print("Choosing custom Twitch ingest server: %s", twitch_server);
        g_object_set(three_video_stream, "twitch-server", twitch_server, NULL);
    }
    g_object_set(three_video_stream, "output-width", output_width, NULL);
    g_object_set(three_video_stream, "output-height", output_height, NULL);
    /* Everything has been configured, signal it by setting the 'ready-to-play' property  */
    g_object_set(three_video_stream, "ready-to-play", TRUE, NULL);

    /* Obtain the pipeline (to subscribe to bus messages, and to trigger pipeline state changes) */
    g_object_get(three_video_stream, "gst-pipeline", &pipeline, NULL);

    loop = g_main_loop_new(NULL, FALSE);

    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_signal_watch(bus);
    g_signal_connect(G_OBJECT(bus), "message", G_CALLBACK(cb_on_bus_message), NULL);
    gst_object_unref(GST_OBJECT(bus));

    /* Kick-off the pipeline - in paused state, as it requires pre-rolling first */
    try_change_pipeline_state(pipeline, GST_STATE_PAUSED);

    g_print("Starting the mainloop\n");
    g_main_loop_run(loop);

    cleanup();
    gst_deinit();
    return 0;
}

/* command line parsing helpers */
void parse_args(gint * argc, gchar ** argv[])
{
    GError *         error   = NULL;
    GOptionContext * context = NULL;

    context = g_option_context_new(" Stream 3 videos simultanously to Twitch using GStreamer");
    g_option_context_add_main_entries(context, entries, NULL);
    if (!g_option_context_parse(context, argc, argv, &error)) {
        g_printerr("option parsing failed: %s\n", error->message);
        exit(1);
    }
    g_option_context_free(context);
    g_free(error);
}

void show_confirmation_prompt()
{
    char r = getchar();
    if (r == '\n') { r = getchar(); }
    while (r != 'n' && r != 'N' && r != 'y' && r != 'Y') {
        g_print("invalid input, enter the choice(y/Y/n/N) again : ");
        r = getchar();
        if (r == '\n') { r = getchar(); }
    }
    if (r == 'n' || r == 'N') {
        g_print("Exiting the program\n");
        exit(0);
    }
}

void verify_parsed_arguments()
{
    if (strlen(video1_filename) == 0 || strlen(video2_filename) == 0 || strlen(video3_filename) == 0) {
        g_printerr("Failed to specify one of the mandatory arguments. Rerun with '--help'.\n");
        exit(1);
    }

    if (strlen(twitch_api_key) == 0) {
        g_print("Twitch API key not provided - you won't be able to stream :(.\n"
                "Would you like to continue with local playback?[Y/N]");
        show_confirmation_prompt();
    }
}

void sig_int_handler(int unused)
{
    g_print("Called ctrl-c. Sending EOS to cleanup gracefully.\n");
    /* This could change the property of the underlying pipeline, but for now it can get the whole object */
    GstEvent * eos_event = gst_event_new_eos();
    if (!gst_element_send_event(pipeline, eos_event)) {
        g_printerr("Could not send an EOS event!");
        gst_object_unref(eos_event);
        cleanup();
        exit(1);
    }
}

static void cleanup()
{
    if (three_video_stream != NULL) { g_object_unref(three_video_stream); }
    if (pipeline != NULL) { gst_object_unref(pipeline); }
}

static gboolean cb_on_bus_message(GstBus * bus, GstMessage * message, gpointer user_data)
{
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
        GError * err = NULL;
        gchar *  name, *debug = NULL;

        name = gst_object_get_path_string(message->src);
        gst_message_parse_error(message, &err, &debug);

        g_printerr("ERROR: from element %s: %s\n", name, err->message);
        if (debug != NULL) g_printerr("Additional debug info:\n%s\n", debug);

        g_error_free(err);
        g_free(debug);
        g_free(name);

        g_main_loop_quit(loop);
        break;
    }
    case GST_MESSAGE_STATE_CHANGED: {
        GstState new_state     = GST_STATE_NULL;
        GstState old_state     = GST_STATE_NULL;
        GstState pending_state = GST_STATE_NULL;
        gst_message_parse_state_changed(message, &old_state, &new_state, &pending_state);
        /* When the whole pipeline is prerolled, commence playback */
        if (GST_ELEMENT(message->src) == pipeline && new_state == GST_STATE_PAUSED) {
            g_print("Pipeline prerolled. Starting playback.\n");
            gst_element_set_state(pipeline, GST_STATE_PLAYING);
        }
        break;
    }
    case GST_MESSAGE_WARNING: {
        GError * err = NULL;
        gchar *  name, *debug = NULL;

        name = gst_object_get_path_string(message->src);
        gst_message_parse_warning(message, &err, &debug);

        g_printerr("ERROR: from element %s: %s\n", name, err->message);
        if (debug != NULL) g_printerr("Additional debug info:\n%s\n", debug);

        g_error_free(err);
        g_free(debug);
        g_free(name);
        break;
    }
    case GST_MESSAGE_EOS: {
        g_print("Got EOS\n");
        g_main_loop_quit(loop);
        gst_element_set_state(pipeline, GST_STATE_NULL);
        g_main_loop_unref(loop);
        break;
    }
    default: break;
    }

    return TRUE;
}
