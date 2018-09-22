/*
 * Copyright (C) 2017 Andriy Lesyuk; All rights reserved.
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libavformat/avformat.h>
#include <libffmpegthumbnailer/videothumbnailerc.h>

#include "extract.h"
#include "image.h"
#include "video.h"
#include "cdindex.h"

#define CD_THUMBNAILS           3
#define CD_FRAME_READ_ATTEMPTS  3

typedef struct {
    const char* dir;
    const char* vpath;
    const char* vapath;
    int vfd;
    int vafd;
    int skip_thumbs;
} cd_video_base;

typedef struct {
    const char* fmt;
    int utc;
} cd_time_format;

static const cd_time_format cd_video_formats[] = {
    { "%Y-%m-%dT%H:%M:%S", 1 },
    { "%Y%m%dT%H%M%S", 1 },
    { "%Y-%m-%d %H:%M:%S", 0 }, // Not ISO 8601, but happens
    { "%Y-%m-%d", 1 },
    { "%Y%m%d", 1 },
    { NULL, 0 }
};

time_t cd_video_parse_time(const char* time) {
    struct tm tm;
    const cd_time_format* format;
    memset(&tm, 0x00, sizeof(struct tm));
    for (format = cd_video_formats; format->fmt; format++) {
        if (strptime(time, format->fmt, &tm)) {
            time_t result = mktime(&tm);
            if (result != (time_t)-1) {
                if (format->utc) {
                    tzset();
                    return result - timezone;
                } else {
                    return result;
                }
            }
            break;
        }
    }
    return 0;
}

int cd_get_codec_tag(char* buf, unsigned int codec_tag) {
    register int i;
    char tchar;
    char ibuf[4];
    for (i = 0; i < 4; i++) {
        tchar = codec_tag & 0xFF;
        if (isprint(tchar)) {
            ibuf[i] = tchar;
        } else {
            return 0;
        }
        codec_tag >>= 8;
    }
    memcpy(buf, ibuf, 4);
    return 1;
}

static inline void cd_get_offset_and_range(int* start, int* range) {
    if ((*range) > 1000) { // More than 15 mins
        *start = floor((*range) * 0.05);
        *range = (*range) - floor((*range) * 0.15);
    }
}

int cd_video_thumbnail_init(cd_video_base* vbase) {
    if (vbase->skip_thumbs == -1) {
        vbase->skip_thumbs = cd_create_data_dir(vbase->dir);
    }
    return !vbase->skip_thumbs;
}

cd_bool cd_video_get_interlaced(AVFormatContext* format, int vindex) {
    int interlaced = 0;
    AVCodec* decoder = avcodec_find_decoder(format->streams[vindex]->codecpar->codec_id);
    if (decoder) {
        AVCodecContext* codecCtx = avcodec_alloc_context3(decoder);
        avcodec_parameters_to_context(codecCtx, format->streams[vindex]->codecpar);
        if (avcodec_open2(codecCtx, decoder, NULL) == 0) {
            AVPacket packet;
            av_init_packet(&packet);
            while (av_read_frame(format, &packet) == 0) {
                if (packet.stream_index == vindex) {
                    int error, attempts = 0;
                    AVFrame* frame = av_frame_alloc();
                    do {
                        error = avcodec_send_packet(codecCtx, &packet);
                        if (error == -1) break;
                        error = avcodec_receive_frame(codecCtx, frame);
                    } while ((error == AVERROR(EAGAIN)) && (++attempts < CD_FRAME_READ_ATTEMPTS));
                    if (error == 0) {
                        interlaced = frame->interlaced_frame;
                    }
                    av_frame_free(&frame);
                    av_packet_unref(&packet);
                    break;
                } else { // It's an audio frame
                    av_packet_unref(&packet);
                }
            }
        }
        avcodec_free_context(&codecCtx);
    }
    return interlaced;
}

int cd_video_generate_thumbnails(const char* file, int duration, int id, const char* dir) {
    int i, tid = 0;
    char stime[24];
    const char* ext = strrchr(file, '.');
    if (ext && !strcasecmp(ext, ".SSIF")) return tid; // Currently thumbnailer fails to generate thumbnails for SSIFs
    srand(time(NULL));
    int start = 0, range = duration;
    cd_get_offset_and_range(&start, &range);
    int seek, augment = floor(range / 3);
    video_thumbnailer* thumbnailer = video_thumbnailer_create();
    thumbnailer->thumbnail_size = CD_THUMBNAIL_SIZE;
    thumbnailer->thumbnail_image_type = Jpeg;
    for (i = 0; i < CD_THUMBNAILS; i++) {
        seek = (augment > 1) ? start + (rand() % augment) + augment * i : 0;
        sprintf(stime, "%d:%02d:%02d", (int)floor(seek / 3600), (int)floor((seek % 3600) / 60), seek % 60);
        thumbnailer->seek_time = stime;
        char* tpath = (char*)malloc(strlen(dir) + 16);
        sprintf(tpath, "%s/%u-%d.jpg", dir, id, ++tid);
        printf("[video] writing frame at %s to %s\n", stime, tpath);
        video_thumbnailer_generate_thumbnail_to_file(thumbnailer, file, tpath);
        free(tpath);
        if (augment <= 1) break;
    }
    video_thumbnailer_destroy(thumbnailer);
    return tid;
}

void* cd_video_init(cd_base* base) {
    cd_video_base* vbase = (cd_video_base*)malloc(sizeof(cd_video_base));
    size_t baselen = strlen(base->base_name);
    vbase->dir = (char*)malloc(baselen - 3);
    strncpy((char*)vbase->dir, base->base_name, baselen - 4);
    ((char*)vbase->dir)[baselen-4] = '\0';
    vbase->vpath = (char*)malloc(baselen + 1);
    strncpy((char*)vbase->vpath, base->base_name, baselen - 4);
    ((char*)vbase->vpath)[baselen-4] = '\0';
    strcat((char*)vbase->vpath, CD_VIDEO_EXT);
    vbase->vfd = -1;
    vbase->vapath = (char*)malloc(baselen + 2);
    strncpy((char*)vbase->vapath, base->base_name, baselen - 4);
    ((char*)vbase->vapath)[baselen-4] = '\0';
    strcat((char*)vbase->vapath, CD_ASTREAMS_EXT);
    vbase->skip_thumbs = -1;
    return vbase;
}

cd_offset cd_video_getdata(const char* file, cd_file_entry* cdentry, void* udata) {
    if (((cd_video_base*)udata)->vfd == -1) {
        ((cd_video_base*)udata)->vfd = open(((cd_video_base*)udata)->vpath, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        if (((cd_video_base*)udata)->vfd == -1) return 0;
        cd_video_mark vmark;
        memcpy(&vmark.mark, CD_VIDEO_MARK, CD_VIDEO_MARK_LEN);
        vmark.version = CD_VIDEO_VERSION;
        write(((cd_video_base*)udata)->vfd, &vmark, sizeof(cd_video_mark));
        ((cd_video_base*)udata)->vafd = open(((cd_video_base*)udata)->vapath, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        if (((cd_video_base*)udata)->vafd == -1) return 0;
        cd_streams_mark vamark;
        memcpy(&vamark.mark, CD_STREAMS_MARK, CD_STREAMS_MARK_LEN);
        vamark.version = CD_STREAMS_VERSION;
        write(((cd_video_base*)udata)->vafd, &vamark, sizeof(cd_streams_mark));
    }
    AVFormatContext* format = NULL;
    av_register_all();
    if (avformat_open_input(&format, file, NULL, NULL) == 0) {
        avformat_find_stream_info(format, NULL);
        off_t aoffset;
        cd_video_entry entry;
        memset(&entry, 0x00, sizeof(cd_video_entry));
        entry.seconds = (format->duration != AV_NOPTS_VALUE) ? format->duration / AV_TIME_BASE : 0;
        int i, vindex = -1;
        AVDictionaryEntry* title = av_dict_get(format->metadata, "title", NULL, 0);
        if (title) strncpy(entry.title, title->value, 128);
        AVDictionaryEntry* ctime = av_dict_get(format->metadata, "creation_time", NULL, 0);
        if (ctime) entry.ctime = cd_video_parse_time(ctime->value);
        AVDictionaryEntry* location = av_dict_get(format->metadata, "location", NULL, 0);
        if (location) sscanf(location->value, "%f%f/", &entry.latitude, &entry.longitude);
        entry.vstreams = entry.astreams = entry.subtitles = 0;
        for (i = 0; i < format->nb_streams; i++) {
            if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                if (vindex == -1) {
                    if (format->streams[i]->codecpar->codec_id != AV_CODEC_ID_NONE) {
                        const char* codec = avcodec_get_name(format->streams[i]->codecpar->codec_id);
                        if (codec) strncpy(entry.video.codec, codec, 18);
                        cd_get_codec_tag(entry.video.codec_tag, format->streams[i]->codecpar->codec_tag);
                    }
                    if (format->streams[i]->codecpar->width && format->streams[i]->codecpar->height) {
                        entry.video.width = format->streams[i]->codecpar->width;
                        entry.video.height = format->streams[i]->codecpar->height;
                    }
                    entry.video.bitrate = format->streams[i]->codecpar->bit_rate / 1000;
                    if (format->streams[i]->avg_frame_rate.den && format->streams[i]->avg_frame_rate.num) {
                        entry.video.framerate = roundf(av_q2d(format->streams[i]->avg_frame_rate) * 100) / 100.0f;
                    }
                    vindex = i;
                }
                entry.vstreams++;
            } else if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                cd_stream_entry sentry;
                memset(&sentry, 0x00, sizeof(cd_stream_entry));
                if (format->streams[i]->codecpar->codec_id != AV_CODEC_ID_NONE) {
                    const char* codec = avcodec_get_name(format->streams[i]->codecpar->codec_id);
                    if (codec) strncpy(sentry.codec, codec, 18);
                    cd_get_codec_tag(sentry.codec_tag, format->streams[i]->codecpar->codec_tag);
                }
                if (format->streams[i]->codecpar->sample_rate) sentry.freq = format->streams[i]->codecpar->sample_rate;
                sentry.channels = format->streams[i]->codecpar->channels;
                sentry.bitrate = format->streams[i]->codecpar->bit_rate / 1000;
                AVDictionaryEntry* lang = av_dict_get(format->streams[i]->metadata, "language", NULL, 0);
                if (lang) strncpy(sentry.lang, lang->value, 3);
                if (format->streams[i]->disposition & AV_DISPOSITION_ORIGINAL) sentry.translation = TRANSLATION_ORIGINAL;
                else if (format->streams[i]->disposition & AV_DISPOSITION_DUB) sentry.translation = TRANSLATION_DUBBED;
                entry.astreams++;
                aoffset = lseek(((cd_video_base*)udata)->vafd, 0, SEEK_END);
                write(((cd_video_base*)udata)->vafd, &sentry, sizeof(cd_stream_entry));
                if (entry.audio == 0) entry.audio = aoffset;
            } else if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                entry.subtitles++;
            }
        }
        if (vindex != -1) entry.video.interlaced = cd_video_get_interlaced(format, vindex);
        off_t offset = lseek(((cd_video_base*)udata)->vfd, 0, SEEK_END);
        write(((cd_video_base*)udata)->vfd, &entry, sizeof(cd_video_entry));
        avformat_close_input(&format);
#ifdef INCLUDE_THUMBNAILS
        if ((entry.seconds > 0) && (vindex != -1) && cd_video_thumbnail_init((cd_video_base*)udata)) {
            cd_video_generate_thumbnails(file, entry.seconds, cdentry->id, ((cd_video_base*)udata)->dir);
        }
#endif /* INCLUDE_THUMBNAILS */
        return offset;
    } else {
        printf("[warning] failed to open %s\n", file);
    }
    return 0;
}

void cd_video_finish(void* udata) {
    if (((cd_video_base*)udata)->vfd != -1) close(((cd_video_base*)udata)->vfd);
    free((void*)((cd_video_base*)udata)->vpath);
    if (((cd_video_base*)udata)->vafd != -1) close(((cd_video_base*)udata)->vafd);
    free((void*)((cd_video_base*)udata)->vapath);
    free((void*)((cd_video_base*)udata)->dir);
    free(udata);
}

static cd_extractor_info cd_video = {
    "video",
    "\\.(mpe?g|vob|mov|mp4|mkv|avi|3gp|wmv|flv|m2ts|ssif)$",
    cd_video_init,
    cd_video_getdata,
    cd_video_finish,
    NULL,
    NULL,
    NULL
};

void cd_extractor_load_video(cd_base* base) {
    cd_add_extractor(&cd_video, base);
}
