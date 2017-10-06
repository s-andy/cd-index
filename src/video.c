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
#include <libavutil/dict.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include "extract.h"
#include "image.h"
#include "video.h"
#include "cdindex.h"

typedef struct {
    const char* dir;
    const char* vpath;
    const char* vapath;
    int vfd;
    int vafd;
    int skip_thumbs;
} cd_video_base;

time_t cd_video_parse_time(const char* time) {
    struct tm tm;
    char **format;
    char* formats[] = {
        "%Y-%m-%dT%H:%M:%S",
        "%Y%m%dT%H%M%S",
        "%Y-%m-%d",
        "%Y%m%d",
        "%Y-%m-%d %H:%M:%S", // Not ISO 8601, but happens
        NULL
    };
    tzset();
    memset(&tm, 0x00, sizeof(struct tm));
    for (format = formats; *format; format++) {
        if (strptime(time, *format, &tm)) {
            time_t result = mktime(&tm);
            if (result != (time_t)-1) {
                return result - timezone;
            }
            break;
        }
    }
    return 0;
}

int cd_get_codec_tag(char* buf, unsigned int codec_tag) {
    register int i;
    char tchar;
    for (i = 0; i < 4; i++) {
        tchar = codec_tag & 0xFF;
        if (isprint(tchar)) {
            buf[i] = tchar;
        } else {
            return 0;
        }
        codec_tag >>= 8;
    }
    return 1;
}

static inline void cd_get_offset_and_range(int* start, int* range) {
    if ((*range) > 1000) { // More than 15 mins
        *start = floor((*range) * 0.05);
        *range = (*range) - floor((*range) * 0.15);
    }
}

int cd_video_thumbnail_init(AVCodecContext** jpegCtx, AVStream* stream, AVFrame* frame, cd_video_base* vbase) {
    if ((*jpegCtx) == NULL) { // Initialize JPEG codec
        AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
        if (codec) {
            (*jpegCtx) = avcodec_alloc_context3(codec);
            (*jpegCtx)->pix_fmt = AV_PIX_FMT_YUVJ420P;
            (*jpegCtx)->width = frame->width;
            (*jpegCtx)->height = frame->height;
            (*jpegCtx)->time_base = stream->time_base;
            cd_get_thumbnail_size(&((*jpegCtx)->width), &((*jpegCtx)->height));
            if (avcodec_open2((*jpegCtx), codec, NULL) == 0) {
                vbase->skip_thumbs = cd_create_data_dir(vbase->dir);
            } else {
                printf("[warning] could not open JPEG codec\n");
                vbase->skip_thumbs = 1;
            }
        } else {
            printf("[warning] could not load JPEG codec\n");
            vbase->skip_thumbs = 1;
        }
    }
    return !vbase->skip_thumbs;
}

AVFrame* cd_video_resize_frame(AVCodecContext* srcCtx, AVCodecContext* dstCtx, AVFrame* frame) {
    struct SwsContext* swsCtx = sws_getCachedContext(NULL, srcCtx->width, srcCtx->height, srcCtx->pix_fmt, dstCtx->width, dstCtx->height, srcCtx->pix_fmt, SWS_LANCZOS, NULL, NULL, NULL);
    AVFrame* newf = av_frame_alloc();
    int bufsize = av_image_get_buffer_size(srcCtx->pix_fmt, dstCtx->width, dstCtx->height, 1);
    uint8_t* buf = (uint8_t*)av_malloc(bufsize * sizeof(uint8_t));
    av_image_fill_arrays(newf->data, newf->linesize, buf, srcCtx->pix_fmt, dstCtx->width, dstCtx->height, 1);
    sws_scale(swsCtx, (const uint8_t* const*)frame->data, frame->linesize, 0, srcCtx->height, newf->data, newf->linesize);
    av_free(buf);
    av_frame_free(&frame);
    sws_freeContext(swsCtx);
    newf->format = srcCtx->pix_fmt;
    newf->width = dstCtx->width;
    newf->height = dstCtx->height;
    return newf;
}

void cd_video_generate_thumbnails(AVFormatContext* format, int vindex, cd_video_base* vbase, int id, cd_bool* interlaced) {
    int duration = (format->duration != AV_NOPTS_VALUE) ? format->duration / AV_TIME_BASE : 0;
    int start = 0, range = duration;
    cd_get_offset_and_range(&start, &range);
    AVCodec* decoder = avcodec_find_decoder(format->streams[vindex]->codecpar->codec_id);
    AVCodec* jpegenc = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (decoder && jpegenc) {
        AVCodecContext* jpegCtx = NULL;
        AVCodecContext* codecCtx = avcodec_alloc_context3(decoder);
        avcodec_parameters_to_context(codecCtx, format->streams[vindex]->codecpar);
        if (avcodec_open2(codecCtx, decoder, NULL) == 0) {
            int i;
            srand(time(NULL));
            AVPacket packet;
            AVPacket jpegPac;
            av_init_packet(&packet);
            av_init_packet(&jpegPac);
            for (i = 0; i < 3; i++) {
                int augment = floor(range / 3);
                if (augment > 1) {
                    int seek = start + (rand() % augment) + augment * i;
                    if (seek > 0) {
                        if (av_seek_frame(format, vindex, seek / av_q2d(format->streams[vindex]->time_base), 0) < 0) {
                            printf("[warning] seeking to %d:%02d failed\n", (int)floor(seek / 60), seek % 60);
                            break;
                        } else {
                            avcodec_flush_buffers(codecCtx);
                        }
                        DEBUG_OUTPUT(DEBUG_DEBUG, "seeking to %d:%02d\n", (int)floor(seek / 60), seek % 60);
                    }
                }
                while (av_read_frame(format, &packet) == 0) {
                    if (packet.stream_index == vindex) {
                        int error;
                        AVFrame* frame = av_frame_alloc();
                        do {
                            avcodec_send_packet(codecCtx, &packet);
                            error = avcodec_receive_frame(codecCtx, frame);
                        } while (error == AVERROR(EAGAIN));
                        if (error != 0) {
                            printf("[warning] could not read video frame\n");
                        } else if (cd_video_thumbnail_init(&jpegCtx, format->streams[vindex], frame, vbase)) {
                            if (interlaced) *interlaced = frame->interlaced_frame;
                            if (frame->width != jpegCtx->width) {
                                frame = cd_video_resize_frame(codecCtx, jpegCtx, frame);
                            }
                            avcodec_send_frame(jpegCtx, frame);
                            avcodec_receive_packet(jpegCtx, &jpegPac);
                            char* tpath = (char*)malloc(strlen(vbase->dir) + 16);
                            sprintf(tpath, "%s/%u-%d.jpg", vbase->dir, id, i + 1);
                            printf("[video] writing thumbnail to %s\n", tpath);
                            int tfd = open(tpath, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
                            if (tfd != -1) {
                                write(tfd, jpegPac.data, jpegPac.size);
                                close(tfd);
                            }
                            free(tpath);
                            av_packet_unref(&jpegPac);
                        }
                        av_frame_free(&frame);
                        av_packet_unref(&packet);
                        break;
                    } else { // It's an audio frame
                        av_packet_unref(&packet);
                    }
                }
                if (augment <= 1) break;
            }
        } else {
            printf("[warning] could not open decoder\n");
        }
        if (jpegCtx) avcodec_free_context(&jpegCtx);
        avcodec_free_context(&codecCtx);
    } else {
        printf("[warning] could not load codec\n");
    }
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
    vbase->skip_thumbs = 0;
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
                entry.astreams++;
                if (entry.audio == 0) {
                    entry.audio = lseek(((cd_video_base*)udata)->vafd, 0, SEEK_END);
                    write(((cd_video_base*)udata)->vafd, &sentry, sizeof(cd_stream_entry));
                }
            } else if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                entry.subtitles++;
            }
        }
#ifdef INCLUDE_THUMBNAILS
        if (!((cd_video_base*)udata)->skip_thumbs && (entry.seconds > 0) && (vindex != -1)) {
            cd_video_generate_thumbnails(format, vindex, (cd_video_base*)udata, cdentry->id, &entry.video.interlaced);
        }
#endif /* INCLUDE_THUMBNAILS */
        off_t offset = lseek(((cd_video_base*)udata)->vfd, 0, SEEK_END);
        write(((cd_video_base*)udata)->vfd, &entry, sizeof(cd_video_entry));
        avformat_close_input(&format);
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
    "\\.(mpe?g|vob|ogg|mov|mp4|mkv|avi|3gp|wmv)$",
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
