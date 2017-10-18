/*
 * Copyright (C) 2007 Andriy Lesyuk; All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#include <regex.h>

#include "base.h"
#include "data.h"
#include "cdindex.h"
#include "audio.h"
#include "image.h"
#include "video.h"

typedef struct __cd_path_entry cd_path_entry;
struct __cd_path_entry {
    cd_offset id;
    char* name;
    cd_path_entry* prev;
};

typedef int (*cd_entry_dump)(const char*, cd_file_entry*, const char*);

typedef struct {
    const char* regex;
    cd_entry_dump dump;
} cd_dumper_info;

cd_path_entry* cd_push_entry(cd_path_entry* parent, cd_file_entry* entry) {
    cd_path_entry* path = (cd_path_entry*)malloc(sizeof(cd_path_entry));
    path->id = entry->id;
    path->name = strdup(entry->name);
    path->prev = NULL;
    if (parent) {
        cd_path_entry* top;
        for (top = parent; top->prev; top = top->prev);
        top->prev = path;
        return parent;
    } else return path;
}

cd_path_entry* cd_free_entries(cd_path_entry* path, cd_offset id, int base) {
    cd_path_entry* prev;
    cd_path_entry* entry;
    for (entry = path; entry;) {
        if (entry->id == id) break;
        prev = entry->prev;
        free(entry->name);
        free(entry);
        entry = prev;
    }
    if ((!entry) && (id != 0)) { // Full clean done while not requested
        cd_offset i;
        cd_file_entry data;
        for (i = id; i;) {
            lseek(base, sizeof(cd_iso_header) + (i - 1) * (sizeof(cd_file_entry) - sizeof(cd_offset)), SEEK_SET);
            read(base, (void*)&data + sizeof(cd_offset), sizeof(cd_file_entry) - sizeof(cd_offset));
            data.id = i;
            entry = cd_push_entry(entry, &data);
            i = data.parent;
        }
    }
    return entry;
}

cd_path_entry* cd_add_entry(cd_path_entry* parent, cd_file_entry* entry, cd_offset id, int base) {
    parent = cd_free_entries(parent, entry->parent, base);
    cd_path_entry* path = (cd_path_entry*)malloc(sizeof(cd_path_entry));
    path->id = id;
    path->name = strdup(entry->name);
    path->prev = parent;
    return path;
}

const char* cd_get_path(cd_path_entry* path, const char* file) {
    cd_path_entry* entry;
    size_t length = strlen(file) + 1;
    for (entry = path; entry; entry = entry->prev) {
        length += strlen(entry->name) + 1;
    }
    char* tmpstr = (char*)malloc(length);
    tmpstr[--length] = '\0';
    length -= strlen(file);
    memcpy(&tmpstr[length], file, strlen(file));
    for (entry = path; entry; entry = entry->prev) {
        tmpstr[--length] = '/';
        length -= strlen(entry->name);
        memcpy(&tmpstr[length], entry->name, strlen(entry->name));
    }
    return tmpstr;
}

const char* cd_get_genre(cd_byte gcode) {
    const cd_search_item* genre;
    for (genre = cd_genre_map; genre->name; genre++) {
        if (genre->code == gcode) return genre->name;
    }
    return "?";
}

const char* cd_get_lang(cd_byte lcode) {
    const cd_search_item* lang;
    for (lang = cd_lang_map; lang->name; lang++) {
        if (lang->code == lcode) return lang->name;
    }
    return "?";
}

const char* cd_get_translation(cd_byte tcode) {
    const cd_search_item* translation;
    for (translation = cd_translation_map; translation->name; translation++) {
        if (translation->code == tcode) return translation->name;
    }
    return "?";
}

cd_byte cd_get_index_version(int fd) {
    cd_index_mark mark;
    lseek(fd, 0, SEEK_SET);
    if ((read(fd, &mark, sizeof(cd_index_mark)) == sizeof(cd_index_mark)) &&
        (memcmp(mark.mark, CD_INDEX_MARK, CD_INDEX_MARK_LEN) == 0)) {
        return mark.version;
    }
    return 0x00;
}

int cd_list(const char* file) {
    int ret = EXIT_SUCCESS;
    int base = open(file, O_RDONLY);
    if (base != -1) {
        cd_byte cdiver = cd_get_index_version(base);
        if (cdiver == CD_INDEX_VERSION) {
            cd_offset i;
            time_t mtime;
            struct tm* tm;
            struct stat stat;
            char* fpath;
            struct group* grp;
            struct passwd* pwd;
            fstat(base, &stat);
            cd_file_entry entry;
            cd_path_entry* path = NULL;
            fpath = strdup(file);
            fpath[strlen(file)-1] = 'l';
            int slinks = open(fpath, O_RDONLY);
            free(fpath);
            for (i = 0; i < (stat.st_size - sizeof(cd_iso_header)) / (sizeof(cd_file_entry) - sizeof(cd_offset)); i++) {
                lseek(base, sizeof(cd_iso_header) + i * (sizeof(cd_file_entry) - sizeof(cd_offset)), SEEK_SET);
                read(base, (void*)&entry + sizeof(cd_offset), sizeof(cd_file_entry) - sizeof(cd_offset));
                if ((path && (entry.parent != path->id)) || (!path && entry.parent))
                    path = cd_free_entries(path, entry.parent, base);
                printf("%c%c%c%c%c%c%c%c%c%c 1",
                    ((entry.type == CD_DIR) || ((entry.type == CD_ARC) && (entry.child != 0))) ? 'd' : (entry.type == CD_LNK) ? 'l' : '-',
                    (entry.mode & S_IRUSR) ? 'r' : '-',
                    (entry.mode & S_IWUSR) ? 'w' : '-',
                    (entry.mode & S_IXUSR) ? 'x' : '-',
                    (entry.mode & S_IRGRP) ? 'r' : '-',
                    (entry.mode & S_IWGRP) ? 'w' : '-',
                    (entry.mode & S_IXGRP) ? 'x' : '-',
                    (entry.mode & S_IROTH) ? 'r' : '-',
                    (entry.mode & S_IWOTH) ? 'w' : '-',
                    (entry.mode & S_IXOTH) ? 'x' : '-');
                pwd = getpwuid(entry.uid);
                if (pwd) printf(" %s", pwd->pw_name);
                else printf(" %d", entry.uid);
                grp = getgrgid(entry.gid);
                if (grp) printf(" %s", grp->gr_name);
                else printf(" %d", entry.gid);
                mtime = entry.mtime;
                tm = localtime(&mtime);
                fpath = (char*)cd_get_path(path, entry.name);
                printf(" %lu %02d-%02d-%04d %02d:%02d %s",
                    (entry.type != CD_DIR) ? entry.size : 0,
                    tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900,
                    tm->tm_hour, tm->tm_min,
                    fpath);
                free(fpath);
                if (entry.type == CD_LNK) {
                    if ((slinks != -1) && entry.size) {
                        lseek(slinks, entry.info, SEEK_SET);
                        fpath = (char*)malloc(entry.size + 1);
                        read(slinks, fpath, entry.size);
                        fpath[entry.size] = '\0';
                        printf(" -> %s", fpath);
                        free(fpath);
                    } else {
                        printf(" -> ");
                    }
                }
                printf("\n");
                DEBUG_OUTPUT(DEBUG_DEBUG, "%3u: p:%3u <- n:%3u -> c:%3u %s\n",
                    i + 1, entry.parent, entry.next, entry.child, entry.name);
                if ((entry.type <= CD_ARC) && (entry.child != 0))
                    path = cd_add_entry(path, &entry, i + 1, base);
            }
            cd_free_entries(path, 0, base);
        } else {
            if (cdiver == 0x00) {
                printf("Invalid CD index!\n");
            } else if (cdiver < CD_INDEX_VERSION) {
                printf("Outdated CD index -- use cdupgrade to upgrade it!\n");
            } else if (cdiver > CD_INDEX_VERSION) {
                printf("CD index version is not supported -- outdated cdbrowse?\n");
            }
            ret = EXIT_FAILURE;
        }
        close(base);
    } else {
        ret = EXIT_FAILURE;
    }
    return ret;
}

int cd_dump_audio(const char* arch, cd_file_entry* entry, const char* to) {
    int ret = EXIT_SUCCESS;
    char* cda = (char*)malloc(strlen(arch) + 1);
    strncpy(cda, arch, strlen(arch) - 4);
    cda[strlen(arch)-4] = '\0';
    strcat(cda, CD_MUSIC_EXT);
    int fd = open(cda, O_RDONLY);
    free(cda);
    if (fd != -1) {
        umask(066);
        FILE* f = fopen(to, "w");
        if (f) {
            cd_audio_entry audio;
            lseek(fd, entry->info, SEEK_SET);
            read(fd, &audio, sizeof(cd_audio_entry));
            fprintf(f, "File:          %.*s\n", CD_NAME_MAX, entry->name);
            fprintf(f, "Version:       MPEG %d.%d Layer %s\n",
                (audio.mpeg == 0x11) ? 1 : 2, (audio.mpeg == 0x00) ? 5 : 0,
                (audio.layer == 0x11) ? "I" : ((audio.layer == 0x10) ? "II" : "III"));
            if (audio.bitrate) fprintf(f, "Duration:      %02lu:%02lu\n",
                (entry->size * 8 / (audio.bitrate * 1000)) / 60,
                (entry->size * 8 / (audio.bitrate * 1000)) % 60);
            fprintf(f, "Bitrate:       %u kbps\n", audio.bitrate);
            fprintf(f, "Sampling rate: %u Hz\n", audio.freq);
            fprintf(f, "Mode:          ");
            if (audio.mode == 0x11) fprintf(f, "mono (single channel)");
            else if (audio.mode == 0x10) fprintf(f, "mono (dual channel)");
            else if (audio.mode == 0x01) fprintf(f, "joint stereo");
            else fprintf(f, "stereo");
            fprintf(f, "\n");
            fprintf(f, "Copyright:     %s\n", (audio.copy) ? "yes" : "no");
            fprintf(f, "Original:      %s\n", (audio.orig) ? "yes" : "no");
            fprintf(f, "\n");
            fprintf(f, "Artist:        %.*s\n", 64, (*audio.artist) ? audio.artist : "-");
            fprintf(f, "Title:         %.*s\n", 128, (*audio.title) ? audio.title : "-");
            fprintf(f, "Album:         %.*s\n", 96, (*audio.album) ? audio.album : "-");
            fprintf(f, "Year:          ");
            if (audio.year) fprintf(f, "%d", audio.year);
            else fprintf(f, "-");
            fprintf(f, "\n");
            fprintf(f, "Genre:         %s\n", (audio.genre != NONE) ? cd_get_genre(audio.genre) : "-");
            fprintf(f, "Language:      %s\n", (audio.lang) ? cd_get_lang(audio.lang) : "-");
            fprintf(f, "Track number:  ");
            if (audio.track) fprintf(f, "%d", audio.track);
            else fprintf(f, "-");
            fprintf(f, "\n\n---\n\n");
            fclose(f);
        } else {
            ret = EXIT_FAILURE;
        }
        close(fd);
    } else {
        ret = EXIT_FAILURE;
    }
    return ret;
}

void cd_print_thumbnails(FILE* f, const char* arch, cd_file_entry* entry) {
    int i = 0, thumb_exists = 0, header = 0;
    size_t baselen = strlen(arch);
    char* dir = (char*)malloc(baselen - 3);
    strncpy(dir, arch, baselen - 4);
    dir[baselen-4] = '\0';
    do {
        char* path = (char*)malloc(strlen(dir) + 18);
        if (i > 0) {
            sprintf(path, "%s/%u-%d.jpg", dir, entry->id, i);
        } else {
            sprintf(path, "%s/%u.jpg", dir, entry->id);
        }
        thumb_exists = !access(path, F_OK);
        if (thumb_exists) {
            if (!header) {
                fprintf(f, "Thumbnails:\n");
                header = 1;
            }
            fprintf(f, "  %s\n", path);
        }
        free(path);
        i++;
    } while ((thumb_exists || (i == 1)) && (i < 10));
    free(dir);
}

int cd_dump_image(const char* arch, cd_file_entry* entry, const char* to) {
    int ret = EXIT_SUCCESS;
    char* cdp = (char*)malloc(strlen(arch) + 1);
    strncpy(cdp, arch, strlen(arch) - 4);
    cdp[strlen(arch)-4] = '\0';
    strcat(cdp, CD_PICTURE_EXT);
    int fd = open(cdp, O_RDONLY);
    free(cdp);
    if (fd != -1) {
        umask(066);
        FILE* f = fopen(to, "w");
        if (f) {
            cd_picture_entry image;
            lseek(fd, entry->info, SEEK_SET);
            read(fd, &image, sizeof(cd_picture_entry));
            fprintf(f, "File:          %.*s\n", CD_NAME_MAX, entry->name);
            fprintf(f, "Dimensions:    %dx%d\n", image.width, image.height);
            fprintf(f, "Created:       ");
            if (image.ctime) {
                time_t ictime = image.ctime;
                fprintf(f, asctime(localtime(&ictime)));
            } else fprintf(f, "-\n");
            fprintf(f, "\n");
            fprintf(f, "Creator:       %.*s\n", 64, (*image.creator) ? image.creator : "-");
            fprintf(f, "Author:        %.*s\n", 64, (*image.author) ? image.author : "-");
            fprintf(f, "Location:      ");
            if (image.latitude || image.longitude) {
                fprintf(f, "%f %f", image.latitude, image.longitude);
            } else fprintf(f, "-");
            fprintf(f, "\n\n---\n\n");
            cd_print_thumbnails(f, arch, entry);
            fclose(f);
        } else {
            ret = EXIT_FAILURE;
        }
        close(fd);
    } else {
         ret = EXIT_FAILURE;
    }
    return ret;
}

int cd_dump_video(const char* arch, cd_file_entry* entry, const char* to) {
    int ret = EXIT_SUCCESS;
    char* cdv = (char*)malloc(strlen(arch) + 1);
    strncpy(cdv, arch, strlen(arch) - 4);
    cdv[strlen(arch)-4] = '\0';
    strcat(cdv, CD_VIDEO_EXT);
    int fd = open(cdv, O_RDONLY);
    free(cdv);
    if (fd != -1) {
        umask(066);
        FILE* f = fopen(to, "w");
        if (f) {
            cd_video_entry ventry;
            lseek(fd, entry->info, SEEK_SET);
            read(fd, &ventry, sizeof(cd_video_entry));
            fprintf(f, "File:          %.*s\n", CD_NAME_MAX, entry->name);
            fprintf(f, "Title:         %.*s\n", 128, (*ventry.title) ? ventry.title : "-");
            fprintf(f, "Duration:      ");
            if (ventry.seconds > 3600) fprintf(f, "%d:%02d:%02d\n", ventry.seconds / 3600, (ventry.seconds % 3600) / 60, ventry.seconds % 60);
            else if (ventry.seconds > 60) fprintf(f, "%d:%02d\n", ventry.seconds / 60, ventry.seconds % 60);
            else fprintf(f, "0:%02d\n", ventry.seconds);
            fprintf(f, "Created:       ");
            if (ventry.ctime) {
                time_t vctime = ventry.ctime;
                fprintf(f, asctime(localtime(&vctime)));
            } else fprintf(f, "-\n");
            fprintf(f, "Video streams: %d\n", ventry.vstreams);
            fprintf(f, "Audio streams: %d\n", ventry.astreams);
            fprintf(f, "Subtitles:     %d\n", ventry.subtitles);
            fprintf(f, "Location:      ");
            if (ventry.latitude || ventry.longitude) {
                fprintf(f, "%f %f\n", ventry.latitude, ventry.longitude);
            } else fprintf(f, "-\n");
            if (ventry.imdb) fprintf(f, "IMDB:          tt%d\n", ventry.imdb);
            fprintf(f, "\n");
            fprintf(f, "Video:\n");
            fprintf(f, "  Dimensions:  %dx%d\n", ventry.video.width, ventry.video.height);
            fprintf(f, "  Codec:       %.*s", 18, ventry.video.codec);
            if (*ventry.video.codec_tag) fprintf(f, " [%.*s]", 4, ventry.video.codec_tag);
            fprintf(f, "\n");
            fprintf(f, "  Bitrate:     %u kbps\n", ventry.video.bitrate);
            fprintf(f, "  Framerate:   ");
            if (((int)(ventry.video.framerate * 100) % 10) > 0) fprintf(f, "%.2f fps\n", ventry.video.framerate);
            else if (((int)(ventry.video.framerate * 100) % 100) > 0) fprintf(f, "%.1f fps\n", ventry.video.framerate);
            else fprintf(f, "%.0f fps\n", ventry.video.framerate);
            fprintf(f, "  Interlaced:  %s\n", (ventry.video.interlaced) ? "yes" : "no");
            if (ventry.astreams > 0) {
                fprintf(f, "\n");
                fprintf(f, "Audio:\n");
                char* cdva = (char*)malloc(strlen(arch) + 2);
                strncpy(cdva, arch, strlen(arch) - 4);
                cdva[strlen(arch)-4] = '\0';
                strcat(cdva, CD_ASTREAMS_EXT);
                int afd = open(cdva, O_RDONLY);
                free(cdva);
                if (afd != -1) {
                    int i;
                    cd_stream_entry vaentry;
                    lseek(afd, ventry.audio, SEEK_SET);
                    for (i = 0; i < ventry.astreams; i++) {
                        read(afd, &vaentry, sizeof(cd_stream_entry));
                        fprintf(f, "  Stream #%d", i + 1);
                        if (vaentry.translation != TRANSLATION_UNKNOWN) fprintf(f, "(%s)", cd_get_translation(vaentry.translation));
                        fprintf(f, "\n");
                        fprintf(f, "    Language:  %.*s\n", 3, (*vaentry.lang) ? vaentry.lang : "-");
                        fprintf(f, "    Codec:     %.*s", 18, vaentry.codec);
                        if (*vaentry.codec_tag) fprintf(f, " [%.*s]", 4, vaentry.codec_tag);
                        fprintf(f, "\n");
                        fprintf(f, "    Channels:  %d\n", vaentry.channels);
                        fprintf(f, "    Bitrate:   %u kbps\n", vaentry.bitrate);
                        fprintf(f, "    Samp.rate: %u Hz\n", vaentry.freq);
                    }
                    close(afd);
                } else {
                    ret = EXIT_FAILURE;
                }
            }
            fprintf(f, "\n---\n\n");
            cd_print_thumbnails(f, arch, entry);
            fclose(f);
        } else {
            ret = EXIT_FAILURE;
        }
        close(fd);
    } else {
         ret = EXIT_FAILURE;
    }
    return ret;
}

cd_dumper_info cd_dumpers[] = {
    { "\\.mp3$", cd_dump_audio },
    { "\\.(bmp|gif|ico|jpe?g|png|psd|svg|tiff?|xcf|nef|crw|cr2)$", cd_dump_image },
    { "\\.(mpe?g|vob|mov|mp4|mkv|avi|3gp|wmv|flv)$", cd_dump_video },
    { NULL, NULL }
};

int cd_copyout(const char* arch, const char* file, const char* to) {
    cd_dumper_info* dumper;
    for (dumper = cd_dumpers; dumper->dump; dumper++) {
        regex_t* regex = (regex_t*)malloc(sizeof(regex_t));
        regcomp(regex, dumper->regex, REG_EXTENDED|REG_ICASE|REG_NOSUB);
        int result = regexec(regex, file, 0, NULL, 0);
        regfree(regex);
        free(regex);
        if (result == 0) {
            int base = open(arch, O_RDONLY);
            if ((base != -1) && (cd_get_index_version(base) == CD_INDEX_VERSION)) {
                size_t length;
                const char* next;
                const char* element;
                cd_file_entry entry;
                cd_offset offset = sizeof(cd_iso_header);
                for (element = file; element;) {
                    next = strchr(element, '/');
                    length = (next) ? next - element : strlen(element);
                    for (;;) {
                        lseek(base, offset, SEEK_SET);
                        read(base, (void*)&entry + sizeof(cd_offset), sizeof(cd_file_entry) - sizeof(cd_offset));
                        if (!strncmp(element, entry.name, length) && !entry.name[length]) {
                            if (next) {
                                if (entry.type == CD_DIR) {
                                    offset = sizeof(cd_iso_header) + (entry.child - 1) * (sizeof(cd_file_entry) - sizeof(cd_offset));
                                } else {
                                    close(base);
                                    return EXIT_FAILURE;
                                }
                            } else {
                                close(base);
                                if ((entry.type == CD_REG) && entry.info) {
                                    entry.id = (offset - sizeof(cd_iso_header)) / (sizeof(cd_file_entry) - sizeof(cd_offset)) + 1;
                                    return dumper->dump(arch, &entry, to);
                                } else {
                                    return EXIT_FAILURE;
                                }
                            }
                            break;
                        } else if (entry.next) {
                            offset = sizeof(cd_iso_header) + (entry.next - 1) * (sizeof(cd_file_entry) - sizeof(cd_offset));
                        } else {
                            close(base);
                            return EXIT_FAILURE;
                        }
                    }
                    element = next;
                    if (element) element++;
                }
                close(base);
            } else {
                if (base != -1) close(base);
                return EXIT_FAILURE;
            }
        }
    }
    return EXIT_FAILURE;
}

int cd_info(const char* file) {
    int ret = EXIT_SUCCESS;
    int base = open(file, O_RDONLY);
    if (base != -1) {
        cd_byte cdiver = cd_get_index_version(base);
        if (cdiver == CD_INDEX_VERSION) {
            time_t time;
            struct stat stat;
            cd_iso_header header;
            fstat(base, &stat);
            lseek(base, 0, SEEK_SET);
            read(base, (void*)&header, sizeof(cd_iso_header));
            printf("File:          %s\n", file);
            printf("Volume ID:     %.*s\n", 32, (*header.volume_id) ? header.volume_id : "-");
            printf("Bootable:      %s\n", (header.bootable) ? "yes" : "no");
            printf("Size:          %lu\n", header.size);
            printf("Files:         %lu\n", (stat.st_size - sizeof(cd_iso_header)) / (sizeof(cd_file_entry) - sizeof(cd_offset)));
            printf("Created:       ");
            if (header.ctime) {
                time = header.ctime;
                printf(asctime(localtime(&time)));
            } else printf("-\n");
            printf("Modified:      ");
            if (header.mtime) {
                time = header.mtime;
                printf(asctime(localtime(&time)));
            } else printf("-\n");
            printf("\n");
            printf("Publisher:     %.*s\n", 128, (*header.publisher) ? header.publisher : "-");
            printf("Preparer:      %.*s\n", 128, (*header.preparer) ? header.preparer : "-");
            printf("Generator:     %.*s\n", 128, (*header.generator) ? header.generator : "-");
            printf("\n---\n\n");
        } else {
            if (cdiver == 0x00) {
                fprintf(stderr, "Invalid CD index!\n");
            } else if (cdiver < CD_INDEX_VERSION) {
                fprintf(stderr, "Outdated CD index -- use cdupgrade to upgrade it!\n");
            } else if (cdiver > CD_INDEX_VERSION) {
                fprintf(stderr, "CD index version is not supported -- outdated cdbrowse?\n");
            }
            ret = EXIT_FAILURE;
        }
        close(base);
    } else {
        ret = EXIT_FAILURE;
    }
    return ret;
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        if (!strcmp(argv[1], "list")) {
            if (argc == 3) {
                return cd_list(argv[2]);
            }
        } else if (!strcmp(argv[1], "copyout")) {
            if (argc == 5) {
                return cd_copyout(argv[2], argv[3], argv[4]);
            }
        } else if (!strcmp(argv[1], "info")) {
            if (argc == 3) {
                return cd_info(argv[2]);
            }
        } else {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
