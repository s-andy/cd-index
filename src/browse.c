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

#include "data.h"
#include "cdindex.h"
#include "audio.h"

typedef struct __cd_path_entry cd_path_entry;
struct __cd_path_entry {
    cd_offset id;
    char* name;
    cd_path_entry* prev;
};

// Updated MC does not recognize Mon DD YYYY HH:MM :(
/*
static const char* months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
*/

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
    if ((!entry) && (id != 0)) {    // Full clean done while not requested
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

int cd_list(const char* file) {
    int base = open(file, O_RDONLY);
    if (base != -1) {
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
                (entry.type <= CD_ARC) ? 'd' : (entry.type == CD_LNK) ? 'l' : '-',
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
            printf(" %u %02d-%02d-%04d %02d:%02d %s",
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
            DEBUG_OUTPUT(DEBUG_DEBUG, "%3lu: p:%3lu <- n:%3lu -> c:%3lu %s\n",
                i + 1, entry.parent, entry.next, entry.child, entry.name);
            if ((entry.type <= CD_ARC) && (entry.child != 0))
                path = cd_add_entry(path, &entry, i + 1, base);
        }
        cd_free_entries(path, 0, base);
        close(base);
    } else {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int cd_copyout(const char* arch, const char* file, const char* to) {
    regex_t* regex = (regex_t*)malloc(sizeof(regex_t));
    regcomp(regex, "\\.mp3$", REG_EXTENDED|REG_ICASE|REG_NOSUB);
    int result = regexec(regex, file, 0, NULL, 0);
    free(regex);
    if (result == REG_NOMATCH) return EXIT_FAILURE;
    int base = open(arch, O_RDONLY);
    if (base != -1) {
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
                            return EXIT_FAILURE;
                        }
                    } else {
                        if ((entry.type == CD_REG) && entry.info) {
                            char* cdafile = (char*)malloc(strlen(arch) + 1);
                            strncpy(cdafile, arch, strlen(arch) - 4);
                            cdafile[strlen(arch)-4] = '\0';
                            strcat(cdafile, CD_MUSIC_EXT);
                            int fd = open(cdafile, O_RDONLY);
                            free(cdafile);
                            if (fd != -1) {
                                umask(066);
                                FILE* f = fopen(to, "w");
                                if (f) {
                                    cd_audio_entry audio;
                                    lseek(fd, entry.info, SEEK_SET);
                                    read(fd, &audio, sizeof(cd_audio_entry));
                                    fprintf(f, "File:          %s\n", file);
                                    fprintf(f, "Version:       MPEG %d.%d Layer %s\n",
                                        (audio.mpeg == 0x11) ? 1 : 2, (audio.mpeg == 0x00) ? 5 : 0,
                                        (audio.layer == 0x11) ? "I" : ((audio.layer == 0x10) ? "II" : "III"));
                                    if (audio.bitrate) fprintf(f, "Duration:      %02u:%02u\n",
                                        (entry.size * 8 / (audio.bitrate * 1000)) / 60,
                                        (entry.size * 8 / (audio.bitrate * 1000)) % 60);
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
                                    fprintf(f, "Artist:        %s\n", (*audio.artist) ? audio.artist : "-");
                                    fprintf(f, "Title:         %s\n", (*audio.title) ? audio.title : "-");
                                    fprintf(f, "Album:         %s\n", (*audio.album) ? audio.album : "-");
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
                                    close(fd);
                                    return EXIT_FAILURE;
                                }
                                close(fd);
                            } else {
                                return EXIT_FAILURE;
                            }
                        } else {
                            return EXIT_FAILURE;
                        }
                    }
                    break;
                } else if (entry.next) {
                    offset = sizeof(cd_iso_header) + (entry.next - 1) * (sizeof(cd_file_entry) - sizeof(cd_offset));
                } else {
                    return EXIT_FAILURE;
                }
            }
            element = next;
            if (element) element++;
        }
        close(base);
    } else {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
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
        } else {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
