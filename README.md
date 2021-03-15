# gstreamer-video-streaming

[![Built with Spacemacs](https://cdn.rawgit.com/syl20bnr/spacemacs/442d025779da2f62fc86c2082703697714db6514/assets/spacemacs-badge.svg)](http://spacemacs.org)

Example C application utilising GStreamer to mix 3 videos onto one screen and optionally stream the result to Twitch via RTMP.

Tested on Ubuntu 20.04.

# Features
 - Mixing 3 videos into one screen (which size can be configured)
 - optional Twitch streaming
 - core functionality is wrapped inside a GObject class, allowing for usage outside of C

# Usage
 `ThreeVideoStream` object encapsulates all the functionality and exposes a set of properties that can affect the playback.
 Optional properties should be set before the property `ready-to-play` is set.

 Note: `ThreeVideoStream` exposes `GstPipeline *` as one of its properties to allow state monitoring & state changes of the underlying GStreamer pipeline.

# Roadmap
- [ ] add audio support
- [ ] implement more layouts (and expose a enum property to change them at runtime)
- [ ] allow building on MacOS & Windows
- [ ] add GUI controls (GTK)
- [ ] allow dynamically reconfiguring the pipeline
- [ ] add support for streaming to other RMTP services
- [ ] rewrite in Rust :)

