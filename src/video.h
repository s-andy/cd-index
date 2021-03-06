/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#ifndef _CD_VIDEO_H_
#define _CD_VIDEO_H_

#define CD_VIDEO_EXT        ".cdv"
#define CD_ASTREAMS_EXT     ".cdva"

#define CD_VIDEO_MARK       "CDV"
#define CD_VIDEO_MARK_LEN   3
#define CD_VIDEO_VERSION    0x01

#define CD_STREAMS_MARK     "CDVA"
#define CD_STREAMS_MARK_LEN 4
#define CD_STREAMS_VERSION  0x01

enum translations {
    TRANSLATION_UNKNOWN   = 0x00,
    TRANSLATION_ORIGINAL  = 0x01, // No translation
    TRANSLATION_DUBBED    = 0x02, // Professional dubbed translation
    TRANSLATION_TWO_VOICE = 0x03,
    TRANSLATION_ONE_VOICE = 0x04
};

static const cd_search_item cd_translation_map[] = {
    { "original", TRANSLATION_ORIGINAL },
    { "dubbed",   TRANSLATION_DUBBED },
    { "2-voice",  TRANSLATION_TWO_VOICE },
    { "1-voice",  TRANSLATION_ONE_VOICE },
    { NULL, TRANSLATION_UNKNOWN }
};

typedef struct {
    char mark[3];           // "CDV"
    cd_byte version;        // 0x01
} packed(cd_video_mark);

typedef struct {
    cd_offset offset;
    cd_word seconds;
    struct {
        cd_word width;
        cd_word height;
        char codec[18];
        char codec_tag[4];  // E.g., XVID
        cd_word bitrate;    // Only video, kb/s
        float framerate;    // Float, fps
        cd_bool interlaced; // Filled only if INCLUDE_THUMBNAILS is defined
    } video;
    cd_offset audio;        // Offset in streams database
    cd_byte vstreams;       // 1+ means 3D (usually)
    cd_byte astreams;       // Number of audio streams
    cd_byte subtitles;      // Number of *embedded* subtitles
    char title[128];
    cd_dword imdb;
    cd_time ctime;
    float latitude;
    float longitude;
} packed(cd_video_entry);

typedef struct {
    char mark[4];           // "CDVA"
    cd_byte version;        // 0x01
} packed(cd_streams_mark);

typedef struct {
    cd_offset offset;
    char lang[3];
    cd_byte translation;
    char codec[18];
    char codec_tag[4];
    cd_byte channels;
    cd_word bitrate;
    cd_word freq;
} packed(cd_stream_entry);

#endif /* _CD_VIDEO_H_ */
