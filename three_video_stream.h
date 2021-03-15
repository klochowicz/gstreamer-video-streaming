#ifndef _THREE_VIDEO_STREAM__H_
#define _THREE_VIDEO_STREAM__H_

#include <glib-object.h>
#include <glib.h>

G_BEGIN_DECLS

#define THREE_VIDEO_STREAM_TYPE_NAME (three_video_stream_get_type())
#define THREE_VIDEO_STREAM(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), THREE_VIDEO_STREAM_TYPE_NAME, ThreeVideoStream))
#define THREE_VIDEO_STREAM_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), THREE_VIDEO_STREAM_TYPE_NAME, ThreeVideoStreamClass))
#define IS_THREE_VIDEO_STREAM(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), THREE_VIDEO_STREAM_TYPE_NAME))
#define IS_THREE_VIDEO_STREAM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), THREE_VIDEO_STREAM_TYPE_NAME))
#define THREE_VIDEO_STREAM_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), THREE_VIDEO_STREAM_TYPE_NAME, ThreeVideoStreamClass))

typedef struct _ThreeVideoStreamClass   ThreeVideoStreamClass;
typedef struct _ThreeVideoStream        ThreeVideoStream;
typedef struct _ThreeVideoStreamPrivate ThreeVideoStreamPrivate;

struct _ThreeVideoStreamClass {
    /*< private >*/
    GObjectClass parent_class;
};

struct _ThreeVideoStream {
    /*< private >*/
    GObject                   parent_instance;
    ThreeVideoStreamPrivate * priv;
};

GType three_video_stream_get_type(void) G_GNUC_CONST;

/* METHODS */
ThreeVideoStream *
three_video_stream_new(gchar * file_path1, gchar * file_path2, gchar * file_path3, gchar * twitch_api_key);

ThreeVideoStream * three_video_stream_ref(ThreeVideoStream * three_video_stream);
void               three_video_stream_free(ThreeVideoStream * three_video_stream);
void               three_video_stream_clear(ThreeVideoStream ** three_video_stream);

G_END_DECLS

#endif /* _THREE_VIDEO_STREAM__H_ */
