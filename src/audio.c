/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <wchar.h>

#include "audio.h"
#include "cdindex.h"
#include "extract.h"
#include "index.h"

#define CD_FOOTER           0x10
#define CD_EXTENDED_HEADER  0x40
#define CD_UNSYNC           0x80

#define CD_CHARSET_DEFAULT  0x00
#define CD_CHARSET_UTF16    0x01
#define CD_CHARSET_UTF8     0x03

#define CD_BOM_LITTLE_ENDIAN    0xFEFF

#define CD_HAS_FOOTER(FLAGS)    (FLAGS & CD_FOOTER)
#define CD_HAS_EXTENDED(FLAGS)  (FLAGS & CD_EXTENDED_HEADER)
#define CD_IS_UNSYNC(FLAGS)     (FLAGS & CD_UNSYNC)

#define CD_FRAMESIZE_MAX    2048

#define MP3_GETVER(MP3)     ((MP3[1] & 0x18) >> 3)
#define MP3_GETLAYER(MP3)   ((MP3[1] & 0x06) >> 1)
#define MP3_GETBITRATE(MP3) ((MP3[2] & 0xF0) >> 4)
#define MP3_GETFREQ(MP3)    ((MP3[2] & 0x0C) >> 2)
#define MP3_GETPADDING(MP3) ((MP3[2] & 0x02) >> 1)
#define MP3_GETMODE(MP3)    ((MP3[3] & 0xC0) >> 6)
#define MP3_GETCOPY(MP3)    ((MP3[3] & 0x08) >> 3)
#define MP3_GETORIG(MP3)    ((MP3[3] & 0x04) >> 2)

typedef struct {
    const char* path;
    int fd;
} cd_audio_base;

typedef struct {
    char id[3];
    struct {
        cd_byte major;
        cd_byte minor;
    } version;
    cd_byte flags;
    cd_offset size;
} packed(cd_id3v2_header);

typedef struct {
    char id[4];
    cd_offset size;
    cd_word flags;
    cd_byte charset;
} packed(cd_frame_header);

typedef struct {
    char id[3];
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    char comment[28];
    cd_byte zero;
    cd_byte track;
    cd_byte genre;
} packed(cd_id3v1_data);

typedef struct {
    char size[6];
    char id[9];
} packed(cd_lyrics_footer);

typedef struct {
    char id[3];
    char size[5];
} packed(cd_lyrics_tag);

typedef struct {
    cd_word wchar;
    cd_byte code;
} cd_unicode_item;

static const cd_word cd_bitrates[14][6] = {
    { 32,  32,  32,  32,  32,  8   },
    { 64,  48,  40,  64,  48,  16  },
    { 96,  56,  48,  96,  56,  24  },
    { 128, 64,  56,  128, 64,  32  },
    { 160, 80,  64,  160, 80,  64  },
    { 192, 96,  80,  192, 96,  80  },
    { 224, 112, 96,  224, 112, 56  },
    { 256, 128, 112, 256, 128, 64  },
    { 288, 160, 128, 288, 160, 128 },
    { 320, 192, 160, 320, 192, 160 },
    { 352, 224, 192, 352, 224, 112 },
    { 384, 256, 224, 384, 256, 128 },
    { 416, 320, 256, 416, 320, 256 },
    { 448, 384, 320, 448, 384, 320 }
};

static const cd_word cd_freqs[3][3] = {
    { 44100, 22050, 11025 },
    { 48000, 24000, 12000 },
    { 32000, 16000, 8000  }
};

static const cd_byte cd_cp1251_koi8u[] = {
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xBD, 0xA6, 0xA7,
    0xB3, 0xA9, 0xB4, 0xAB, 0xAC, 0xAD, 0xAE, 0xB7,
    0xB0, 0xB1, 0xB6, 0xA6, 0xAD, 0xB5, 0xB6, 0xB7,
    0xA3, 0xB9, 0xA4, 0xBB, 0xBC, 0xBD, 0xBE, 0xA7,
    0xE1, 0xE2, 0xF7, 0xE7, 0xE4, 0xE5, 0xF6, 0xFA,
    0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0,
    0xF2, 0xF3, 0xF4, 0xF5, 0xE6, 0xE8, 0xE3, 0xFE,
    0xFB, 0xFD, 0xFF, 0xF9, 0xF8, 0xFC, 0xE0, 0xF1,
    0xC1, 0xC2, 0xD7, 0xC7, 0xC4, 0xC5, 0xD6, 0xDA,
    0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0,
    0xD2, 0xD3, 0xD4, 0xD5, 0xC6, 0xC8, 0xC3, 0xDE,
    0xDB, 0xDD, 0xDF, 0xD9, 0xD8, 0xDC, 0xC0, 0xD1
};

static const cd_unicode_item cd_utf16_map[] = {
    { 0x2500, 0x80 }, { 0x2502, 0x81 }, { 0x250C, 0x82 }, { 0x2510, 0x83 },
    { 0x2514, 0x84 }, { 0x2518, 0x85 }, { 0x251C, 0x86 }, { 0x2524, 0x87 },
    { 0x252C, 0x88 }, { 0x2534, 0x89 }, { 0x253C, 0x8A }, { 0x2580, 0x8B },
    { 0x2584, 0x8C }, { 0x2588, 0x8D }, { 0x258C, 0x8E }, { 0x2590, 0x8F },
    { 0x2591, 0x90 }, { 0x2592, 0x91 }, { 0x2593, 0x92 }, { 0x2320, 0x93 },
    { 0x25A0, 0x94 }, { 0x2219, 0x95 }, { 0x221A, 0x96 }, { 0x2248, 0x97 },
    { 0x2264, 0x98 }, { 0x2265, 0x99 }, { 0x00A0, 0x9A }, { 0x2321, 0x9B },
    { 0x00B0, 0x9C }, { 0x00B2, 0x9D }, { 0x00B7, 0x9E }, { 0x00F7, 0x9F },
    { 0x2550, 0xA0 }, { 0x2551, 0xA1 }, { 0x2552, 0xA2 }, { 0x0451, 0xA3 },
    { 0x0454, 0xA4 }, { 0x2554, 0xA5 }, { 0x0456, 0xA6 }, { 0x0457, 0xA7 },
    { 0x2557, 0xA8 }, { 0x2558, 0xA9 }, { 0x2559, 0xAA }, { 0x255A, 0xAB },
    { 0x255B, 0xAC }, { 0x0491, 0xAD }, { 0x255D, 0xAE }, { 0x255E, 0xAF },
    { 0x255F, 0xB0 }, { 0x2560, 0xB1 }, { 0x2561, 0xB2 }, { 0x0401, 0xB3 },
    { 0x0404, 0xB4 }, { 0x2563, 0xB5 }, { 0x0406, 0xB6 }, { 0x0407, 0xB7 },
    { 0x2566, 0xB8 }, { 0x2567, 0xB9 }, { 0x2568, 0xBA }, { 0x2569, 0xBB },
    { 0x256A, 0xBC }, { 0x0490, 0xBD }, { 0x256C, 0xBE }, { 0x00A9, 0xBF },
    { 0x044E, 0xC0 }, { 0x0430, 0xC1 }, { 0x0431, 0xC2 }, { 0x0446, 0xC3 },
    { 0x0434, 0xC4 }, { 0x0435, 0xC5 }, { 0x0444, 0xC6 }, { 0x0433, 0xC7 },
    { 0x0445, 0xC8 }, { 0x0438, 0xC9 }, { 0x0439, 0xCA }, { 0x043A, 0xCB },
    { 0x043B, 0xCC }, { 0x043C, 0xCD }, { 0x043D, 0xCE }, { 0x043E, 0xCF },
    { 0x043F, 0xD0 }, { 0x044F, 0xD1 }, { 0x0440, 0xD2 }, { 0x0441, 0xD3 },
    { 0x0442, 0xD4 }, { 0x0443, 0xD5 }, { 0x0436, 0xD6 }, { 0x0432, 0xD7 },
    { 0x044C, 0xD8 }, { 0x044B, 0xD9 }, { 0x0437, 0xDA }, { 0x0448, 0xDB },
    { 0x044D, 0xDC }, { 0x0449, 0xDD }, { 0x0447, 0xDE }, { 0x044A, 0xDF },
    { 0x042E, 0xE0 }, { 0x0410, 0xE1 }, { 0x0411, 0xE2 }, { 0x0426, 0xE3 },
    { 0x0414, 0xE4 }, { 0x0415, 0xE5 }, { 0x0424, 0xE6 }, { 0x0413, 0xE7 },
    { 0x0425, 0xE8 }, { 0x0418, 0xE9 }, { 0x0419, 0xEA }, { 0x041A, 0xEB },
    { 0x041B, 0xEC }, { 0x041C, 0xED }, { 0x041D, 0xEE }, { 0x041E, 0xEF },
    { 0x041F, 0xF0 }, { 0x042F, 0xF1 }, { 0x0420, 0xF2 }, { 0x0421, 0xF3 },
    { 0x0422, 0xF4 }, { 0x0423, 0xF5 }, { 0x0416, 0xF6 }, { 0x0412, 0xF7 },
    { 0x042C, 0xF8 }, { 0x042B, 0xF9 }, { 0x0417, 0xFA }, { 0x0428, 0xFB },
    { 0x042D, 0xFC }, { 0x0429, 0xFD }, { 0x0427, 0xFE }, { 0x042A, 0xFF },
    { 0x0000, 0x00 }
};

inline cd_offset cd_ntoh(cd_offset n) {
    unsigned char* bytes = (unsigned char*)&n;
    return (bytes[0] << 21) | (bytes[1] << 14) | (bytes[2] << 7) | bytes[3];
}

void cd_strcpy(char* dst, int dstlen, const char* src, int srclen, cd_byte charset) {
    if (charset == CD_CHARSET_DEFAULT) {
        register int i;
        for (i = 0; i < dstlen; i++) {
            if ((i < (dstlen - 1)) && (i < srclen)) {
                if ((unsigned char)src[i] < 0x80) dst[i] = src[i];
                else dst[i] = cd_cp1251_koi8u[(unsigned char)src[i]-0x80];
            } else dst[i] = '\0';
        }
    } else if (charset == CD_CHARSET_UTF16) {
        register int i;
        size_t wsrclen = (srclen / 2);
        cd_word* wsrc = (cd_word*)src;
        if (wsrc[0] == CD_BOM_LITTLE_ENDIAN) {
            for (i = 0; i < dstlen; i++) {
                if ((i < (dstlen - 1)) && (i < (wsrclen - 1))) {
                    if (wsrc[i+1] < 0x0080) dst[i] = (unsigned char)wsrc[i+1];
                    else {
                        const cd_unicode_item* unichar;
                        for (unichar = cd_utf16_map; unichar->wchar; unichar++) {
                            if (unichar->wchar == wsrc[i+1]) {
                                dst[i] = unichar->code;
                                break;
                            }
                        }
                    }
                } else dst[i] = '\0';
            }
        } else {
            printf("[fixme] big-endian UTF-16 encoding is not supported\n");
        }
    } else {
        printf("[fixme] unsupported character set 0x%02X\n", charset);
    }
}

inline size_t cd_wcslen(const cd_word* wstr) {
    register int i;
    const char* str = (const char*)wstr;
    for (i = 0; str[i]; i += 2);
    return i / 2;
}

inline void cd_wcstrncpy(char* str, cd_word* wcs, size_t len) {
    register int i;
    for (i = 0; i < len; i++) {
        if (wcs[i] < 0x0080) str[i] = (char)wcs[i];
        else str[i] = '?';
    }
    str[i] = '\0';
}

const char* cd_getstr(const char* name, cd_byte charset) {
    if (charset == CD_CHARSET_DEFAULT) {
        return name;
    } else if (charset == CD_CHARSET_UTF16) {
        if (((cd_word*)name)[0] == CD_BOM_LITTLE_ENDIAN) {
            char* s = (char*)malloc(cd_wcslen(&((cd_word*)name)[1])+1);
            cd_wcstrncpy(s, &((cd_word*)name)[1], cd_wcslen(&((cd_word*)name)[1]));
            return s;
        } else {
            printf("[fixme] big-endian UTF-16 encoding is not supported\n");
            return NULL;
        }
    } else {
        printf("[fixme] unsupported character set 0x%02X\n", charset);
        return NULL;
    }
}

inline int cd_strtoi(const char* str, cd_byte charset) {
    const char* src = cd_getstr(str, charset);
    if (src) {
        int value = strtol(src, NULL, 10);
        if (src != str) free((void*)src);
        return value;
    }
    return 0;
}

cd_byte cd_genre(const char* name, cd_byte charset) {
    const char* str = cd_getstr(name, charset);
    if (str) {
        cd_byte gcode = 0xFF;
        if (str[0] == '(') {
            gcode = strtol(&str[1], NULL, 10);
        } else {
            const cd_search_item* genre;
            for (genre = cd_genre_map; genre->name; genre++) {
                if (!strcasecmp(str, genre->name)) {
                    gcode = genre->code;
                    break;
                }
            }
            if (gcode == 0xFF) {
                printf("[warning] unrecognized category \"%s\"\n", str);
            }
        }
        if (str != name) free((void*)str);
        return gcode;
    }
    return 0xFF;
}

cd_byte cd_lang(const char* name, cd_byte charset) {
    const char* str = cd_getstr(name, charset);
    if (str) {
        cd_byte lcode = 0x00;
        const cd_search_item* lang;
        for (lang = cd_lang_map; lang->name; lang++) {
            if (!strcasecmp(str, lang->name)) {
                lcode = lang->code;
                break;
            }
        }
        if (lcode == 0x00) {
            printf("[warning] unrecognized language \"%s\"\n", str);
        }
        if (str != name) free((void*)str);
        return lcode;
    }
    return 0x00;
}

cd_word cd_get_bitrate(cd_byte bitrate, cd_byte version, cd_byte layer) {
    if ((bitrate > 0x00) && (bitrate < 0x0F) &&
        (layer > 0x00) && (layer <= 0x03)) {
        int i = 3 - layer + ((version == 0x03) ? 0 : 3);
        return cd_bitrates[bitrate-1][i];
    }
    return 0;
}

cd_word cd_get_freq(cd_byte freq, cd_byte version) {
    if (freq < 0x03) {
        int i = (version == 0) ? 2 : 3 - version;
        return cd_freqs[freq][i];
    }
    return 0;
}

void* cd_audio_init(cd_base* base) {
    cd_audio_base* mbase = (cd_audio_base*)malloc(sizeof(cd_audio_base));
    mbase->path = (char*)malloc(strlen(base->base_name) + 1);
    strncpy((char*)mbase->path, base->base_name, strlen(base->base_name) - 4);
    ((char*)mbase->path)[strlen(base->base_name)-4] = '\0';
    strcat((char*)mbase->path, CD_MUSIC_EXT);
    mbase->fd = -1;
    return mbase;
}

cd_offset cd_audio_getdata(const char* file, cd_file_entry* cdentry, void* udata) {
    if (((cd_audio_base*)udata)->fd == -1) {
        ((cd_audio_base*)udata)->fd = open(((cd_audio_base*)udata)->path, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        if (((cd_audio_base*)udata)->fd == -1) return 0;
        cd_audio_mark mark;
        memcpy(&mark.mark, CD_MUSIC_MARK, CD_MUSIC_MARK_LEN);
        mark.version = CD_MUSIC_VERSION;
        write(((cd_audio_base*)udata)->fd, &mark, sizeof(cd_audio_mark));
    }
    int fd = open(file, O_RDONLY);
    if (fd != -1) {
        cd_audio_entry entry;
        cd_id3v2_header id3v2;
        memset(&entry, 0x00, sizeof(cd_audio_entry));
        entry.genre = NONE;
        entry.offset = cdentry->id;
        read(fd, &id3v2, sizeof(cd_id3v2_header));
        if (!strncmp(id3v2.id, "ID3", 3)) {
            cd_offset offset = sizeof(cd_id3v2_header);
            cd_frame_header frame;
            printf("[audio] using id3 v.2.%d.%d...\n", id3v2.version.major, id3v2.version.minor);
            if (CD_HAS_EXTENDED(id3v2.flags)) {
                cd_offset extsize;
                read(fd, &extsize, sizeof(cd_offset));
                offset = lseek(fd,  cd_ntoh(extsize) - sizeof(cd_offset), SEEK_CUR);
            }
            while (offset < cd_ntoh(id3v2.size)) {
                read(fd, &frame, sizeof(cd_frame_header));
                if (!frame.id[0]) break;
                if ((cd_ntoh(frame.size) < CD_FRAMESIZE_MAX) && (
                    !strncmp(frame.id, "TIT2", 4) ||    // song title
                    !strncmp(frame.id, "TALB", 4) ||    // album name
                    !strncmp(frame.id, "TPE1", 4) ||    // artist name
                    !strncmp(frame.id, "TRCK", 4) ||    // track number
                    !strncmp(frame.id, "TYER", 4) ||    // year
                    !strncmp(frame.id, "TCON", 4) ||    // category (content type)
                    !strncmp(frame.id, "TLAN", 4))) {   // language
                    char frmbuf[cd_ntoh(frame.size)];
                    read(fd, &frmbuf, cd_ntoh(frame.size) - 1);
                    frmbuf[cd_ntoh(frame.size)-1] = '\0';
                    if (!strncmp(frame.id, "TIT2", 4))
                        cd_strcpy(entry.title, 128, frmbuf, cd_ntoh(frame.size) - 1, frame.charset);
                    else if (!strncmp(frame.id, "TALB", 4))
                        cd_strcpy(entry.album, 96, frmbuf, cd_ntoh(frame.size) - 1, frame.charset);
                    else if (!strncmp(frame.id, "TPE1", 4))
                        cd_strcpy(entry.artist, 64, frmbuf, cd_ntoh(frame.size) - 1, frame.charset);
                    else if (!strncmp(frame.id, "TRCK", 4))
                        entry.track = cd_strtoi(frmbuf, frame.charset);
                    else if (!strncmp(frame.id, "TYER", 4))
                        entry.year = cd_strtoi(frmbuf, frame.charset);
                    else if (!strncmp(frame.id, "TCON", 4))
                        entry.genre = cd_genre(frmbuf, frame.charset);
                    else if (!strncmp(frame.id, "TLAN", 4))
                        entry.lang = cd_lang(frmbuf, frame.charset);
                    offset += sizeof(cd_frame_header) + cd_ntoh(frame.size) - 1;
                } else {
                    offset = lseek(fd, cd_ntoh(frame.size) - 1, SEEK_CUR);
                }
            }
            lseek(fd, cd_ntoh(id3v2.size) + ((CD_HAS_FOOTER(id3v2.flags)) ? 20 : 10), SEEK_SET);
        } else {
            cd_id3v1_data id3v1;
            lseek(fd, -128, SEEK_END);
            read(fd, &id3v1, sizeof(cd_id3v1_data));
            if (!strncmp(id3v1.id, "TAG", 3)) {
                cd_strcpy(entry.title, 128, id3v1.title, 30, CD_CHARSET_DEFAULT);
                cd_fix_string(entry.title, 30);
                cd_strcpy(entry.artist, 64, id3v1.artist, 30, CD_CHARSET_DEFAULT);
                cd_fix_string(entry.artist, 30);
                cd_strcpy(entry.album, 96, id3v1.album, 30, CD_CHARSET_DEFAULT);
                cd_fix_string(entry.album, 30);
                char year[5];
                strncpy(year, id3v1.year, 4);
                year[4] = '\0';
                entry.year = strtol(year, NULL, 10);
                entry.genre = id3v1.genre;
                if ((!id3v1.zero) && (id3v1.track)) entry.track = id3v1.track;
                // Lyrics
                cd_lyrics_footer lyrmark;
                lseek(fd, -143, SEEK_END);
                read(fd, &lyrmark, sizeof(cd_lyrics_footer));
                if (!strncmp(lyrmark.id, "LYRICS200", 9)) {
                    printf("[audio] using lyrics3...\n");
                    char size[7];
                    strncpy(size, lyrmark.size, 6);
                    size[6] = '\0';
                    int lyrsize = strtol(size, NULL, 10);
                    lseek(fd, -143 - lyrsize, SEEK_END);
                    char mark[11];
                    read(fd, mark, 11);
                    if (!strncmp(mark, "LYRICSBEGIN", 11)) {
                        lyrsize -= 11;
                        size[5] = '\0';
                        cd_lyrics_tag tag;
                        int tagsize;
                        while (lyrsize > 0) {
                            read(fd, &tag, sizeof(cd_lyrics_tag));
                            strncpy(size, tag.size, 5);
                            tagsize = strtol(size, NULL, 10);
                            lyrsize -= sizeof(cd_lyrics_tag) + tagsize;
                            if ((tagsize < CD_FRAMESIZE_MAX) && (
                                !strncmp(tag.id, "ETT", 3) ||
                                !strncmp(tag.id, "EAR", 3) ||
                                !strncmp(tag.id, "EAL", 3))) {
                                char tagbuf[tagsize+1];
                                read(fd, &tagbuf, tagsize);
                                tagbuf[tagsize] = '\0';
                                if (!strncmp(tag.id, "ETT", 3))
                                    cd_strcpy(entry.title, 128, tagbuf, tagsize, CD_CHARSET_DEFAULT);
                                else if (!strncmp(tag.id, "EAR", 3))
                                    cd_strcpy(entry.artist, 64, tagbuf, tagsize, CD_CHARSET_DEFAULT);
                                else if (!strncmp(tag.id, "EAL", 3))
                                    cd_strcpy(entry.album, 96, tagbuf, tagsize, CD_CHARSET_DEFAULT);
                            } else {
                                lseek(fd, tagsize, SEEK_CUR);
                            }
                        }
                    } else {
                        printf("[warning] lyrics tag is broken\n");
                    }
                } else {
                    printf("[audio] using id3 v.1.%d...\n", ((!id3v1.zero) && (id3v1.track)) ? 1 : 0);
                }
            }
            lseek(fd, 0, SEEK_SET);
        }
        cd_byte mp3[4];
        read(fd, mp3, 4);
        if ((mp3[0] == 0xFF) && ((mp3[1] & 0xE0) == 0xE0)) {
            entry.bitrate = cd_get_bitrate(MP3_GETBITRATE(mp3), MP3_GETVER(mp3), MP3_GETLAYER(mp3));
            entry.freq = cd_get_freq(MP3_GETFREQ(mp3), MP3_GETVER(mp3));
            entry.mpeg = MP3_GETVER(mp3);
            entry.layer = MP3_GETLAYER(mp3);
            entry.mode = MP3_GETMODE(mp3);
            entry.copy = MP3_GETCOPY(mp3);
            entry.orig = MP3_GETORIG(mp3);
#ifdef INCLUDE_DURATION
            entry.seconds = cdentry->size * 8 / (entry.bitrate * 1000);
#endif
        } else {
            printf("[warning] invalid mp3 header (%02X%02X)\n", mp3[0], (mp3[1] & 0xE0));
            close(fd);
            return 0;
        }
        close(fd);
        off_t offset = lseek(((cd_audio_base*)udata)->fd, 0, SEEK_END);
        write(((cd_audio_base*)udata)->fd, &entry, sizeof(cd_audio_entry));
        return offset;
    }
    return 0;
}

void cd_audio_finish(void* udata) {
    if (((cd_audio_base*)udata)->fd != -1) close(((cd_audio_base*)udata)->fd);
    free((void*)((cd_audio_base*)udata)->path);
    free(udata);
}

static cd_extractor_info cd_audio = {
    "audio",
    "\\.mp3$",
    cd_audio_init,
    cd_audio_getdata,
    cd_audio_finish,
    NULL,
    NULL,
    NULL
};

void cd_extractor_load_audio(cd_base* base) {
    cd_add_extractor(&cd_audio, base);
}
