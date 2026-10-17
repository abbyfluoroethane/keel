#include "library.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lvgl.h"
/* lv_image_decoder_dsc_t is private in LVGL 9.6; prefetch needs its size. */
#include "src/image/lv_image_decoder_private.h"

/* ---- A minimal JSON reader ---------------------------------------------
 * Enough for library.json: objects, arrays, strings (with \uXXXX), numbers,
 * true/false/null. Values are a tree of nodes; strings are decoded in place
 * into their own allocations and kept for the life of the program.
 */

typedef enum { J_NULL, J_BOOL, J_NUM, J_STR, J_ARR, J_OBJ } jtype_t;

typedef struct jnode {
    jtype_t type;
    char * key;            /* set when the node is an object member */
    char * str;
    double num;
    struct jnode * child;  /* first element or member */
    struct jnode * next;
} jnode_t;

typedef struct {
    const char * p;
    bool err;
} jparser_t;

static void skip_ws(jparser_t * j)
{
    while(*j->p == ' ' || *j->p == '\n' || *j->p == '\r' || *j->p == '\t') j->p++;
}

static void put_utf8(char ** o, uint32_t cp)
{
    char * d = *o;
    if(cp < 0x80) *d++ = (char)cp;
    else if(cp < 0x800) {
        *d++ = (char)(0xC0 | (cp >> 6));
        *d++ = (char)(0x80 | (cp & 0x3F));
    }
    else if(cp < 0x10000) {
        *d++ = (char)(0xE0 | (cp >> 12));
        *d++ = (char)(0x80 | ((cp >> 6) & 0x3F));
        *d++ = (char)(0x80 | (cp & 0x3F));
    }
    else {
        *d++ = (char)(0xF0 | (cp >> 18));
        *d++ = (char)(0x80 | ((cp >> 12) & 0x3F));
        *d++ = (char)(0x80 | ((cp >> 6) & 0x3F));
        *d++ = (char)(0x80 | (cp & 0x3F));
    }
    *o = d;
}

static char * parse_string(jparser_t * j)
{
    if(*j->p != '"') {
        j->err = true;
        return NULL;
    }
    j->p++;
    const char * start = j->p;
    while(*j->p && *j->p != '"') j->p += (*j->p == '\\' && j->p[1]) ? 2 : 1;
    if(!*j->p) {
        j->err = true;
        return NULL;
    }
    char * out = malloc((size_t)(j->p - start) + 1);
    char * o = out;
    for(const char * s = start; s < j->p; s++) {
        if(*s != '\\') {
            *o++ = *s;
            continue;
        }
        s++;
        switch(*s) {
            case 'n': *o++ = '\n'; break;
            case 't': *o++ = '\t'; break;
            case 'r': *o++ = '\r'; break;
            case 'b': *o++ = '\b'; break;
            case 'f': *o++ = '\f'; break;
            case 'u': {
                uint32_t cp = (uint32_t)strtoul((char[]){s[1], s[2], s[3], s[4], 0}, NULL, 16);
                s += 4;
                if(cp >= 0xD800 && cp < 0xDC00 && s[1] == '\\' && s[2] == 'u') {
                    uint32_t lo = (uint32_t)strtoul((char[]){s[3], s[4], s[5], s[6], 0}, NULL, 16);
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                    s += 6;
                }
                put_utf8(&o, cp);
                break;
            }
            default: *o++ = *s; break;
        }
    }
    *o = '\0';
    j->p++;
    return out;
}

static jnode_t * parse_value(jparser_t * j)
{
    skip_ws(j);
    jnode_t * n = calloc(1, sizeof(*n));
    char c = *j->p;
    if(c == '{' || c == '[') {
        n->type = c == '{' ? J_OBJ : J_ARR;
        char close = c == '{' ? '}' : ']';
        j->p++;
        jnode_t ** tail = &n->child;
        skip_ws(j);
        if(*j->p == close) {
            j->p++;
            return n;
        }
        while(!j->err) {
            skip_ws(j);
            char * key = NULL;
            if(n->type == J_OBJ) {
                key = parse_string(j);
                skip_ws(j);
                if(*j->p != ':') {
                    j->err = true;
                    break;
                }
                j->p++;
            }
            jnode_t * v = parse_value(j);
            v->key = key;
            *tail = v;
            tail = &v->next;
            skip_ws(j);
            if(*j->p == ',') {
                j->p++;
                continue;
            }
            if(*j->p == close) {
                j->p++;
                break;
            }
            j->err = true;
        }
    }
    else if(c == '"') {
        n->type = J_STR;
        n->str = parse_string(j);
    }
    else if(!strncmp(j->p, "true", 4) || !strncmp(j->p, "false", 5)) {
        n->type = J_BOOL;
        n->num = c == 't';
        j->p += c == 't' ? 4 : 5;
    }
    else if(!strncmp(j->p, "null", 4)) {
        j->p += 4;
    }
    else {
        char * end;
        n->type = J_NUM;
        n->num = strtod(j->p, &end);
        if(end == j->p) j->err = true;
        j->p = end;
    }
    return n;
}

static jnode_t * jget(const jnode_t * obj, const char * key)
{
    if(!obj || obj->type != J_OBJ) return NULL;
    for(jnode_t * c = obj->child; c; c = c->next)
        if(c->key && !strcmp(c->key, key)) return c;
    return NULL;
}

static const char * jstr(const jnode_t * obj, const char * key)
{
    jnode_t * n = jget(obj, key);
    return n && n->type == J_STR ? n->str : "";
}

static int32_t jint(const jnode_t * obj, const char * key)
{
    jnode_t * n = jget(obj, key);
    return n && n->type == J_NUM ? (int32_t)n->num : 0;
}

static int32_t jlen(const jnode_t * arr)
{
    int32_t n = 0;
    if(arr)
        for(jnode_t * c = arr->child; c; c = c->next) n++;
    return n;
}

/* ---- Library ---------------------------------------------------------- */

static keel_library_t lib;
static char * asset_root;
static char ** cover_paths; /* album_count x 4, NULL when the file is missing */
static const int32_t cover_sizes[4] = {480, 408, 198, 72};

static int32_t find_artist(const char * id)
{
    for(int32_t i = 0; i < lib.artist_count; i++)
        if(!strcmp(lib.artists[i].id, id)) return i;
    return -1;
}

static int32_t find_album(const char * id)
{
    for(int32_t i = 0; i < lib.album_count; i++)
        if(!strcmp(lib.albums[i].id, id)) return i;
    return -1;
}

static int cmp_title(const void * a, const void * b)
{
    return strcmp(lib.tracks[*(const int32_t *)a].title, lib.tracks[*(const int32_t *)b].title);
}

bool keel_library_load(const char * asset_dir)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/demo/library.json", asset_dir);
    FILE * f = fopen(path, "rb");
    if(!f) {
        LV_LOG_ERROR("keel: cannot open %s", path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char * text = malloc((size_t)size + 1);
    size_t got = fread(text, 1, (size_t)size, f);
    text[got] = '\0';
    fclose(f);

    jparser_t j = {text, false};
    jnode_t * root = parse_value(&j);
    free(text);
    if(j.err) {
        LV_LOG_ERROR("keel: %s is not valid JSON", path);
        return false;
    }

    jnode_t * artists = jget(root, "artists");
    jnode_t * albums = jget(root, "albums");
    jnode_t * playlists = jget(root, "playlists");

    lib.artist_count = jlen(artists);
    lib.artists = calloc((size_t)lib.artist_count, sizeof(keel_artist_t));
    int32_t i = 0;
    for(jnode_t * a = artists ? artists->child : NULL; a; a = a->next, i++) {
        lib.artists[i].id = jstr(a, "id");
        lib.artists[i].name = jstr(a, "name");
        lib.artists[i].source = jstr(a, "source");
    }

    lib.album_count = jlen(albums);
    lib.albums = calloc((size_t)lib.album_count, sizeof(keel_album_t));
    for(jnode_t * a = albums ? albums->child : NULL; a; a = a->next) lib.track_count += jlen(jget(a, "tracks"));
    lib.tracks = calloc((size_t)lib.track_count, sizeof(keel_track_t));

    int32_t t = 0;
    i = 0;
    for(jnode_t * a = albums ? albums->child : NULL; a; a = a->next, i++) {
        keel_album_t * al = &lib.albums[i];
        al->id = jstr(a, "id");
        al->title = jstr(a, "title");
        al->source = jstr(a, "source");
        al->year = jint(a, "year");
        al->artist = find_artist(jstr(a, "artist"));
        al->first_track = t;
        int32_t n = 1;
        for(jnode_t * tr = jget(a, "tracks")->child; tr; tr = tr->next, t++, n++) {
            lib.tracks[t] = (keel_track_t){jstr(tr, "title"), jstr(tr, "source"), jint(tr, "duration"), i, n};
            al->duration += lib.tracks[t].duration;
        }
        al->track_count = t - al->first_track;
        if(al->artist >= 0) {
            lib.artists[al->artist].album_count++;
            lib.artists[al->artist].track_count += al->track_count;
        }
    }

    lib.playlist_count = jlen(playlists);
    lib.playlists = calloc((size_t)lib.playlist_count, sizeof(keel_playlist_t));
    i = 0;
    for(jnode_t * p = playlists ? playlists->child : NULL; p; p = p->next, i++) {
        keel_playlist_t * pl = &lib.playlists[i];
        pl->title = jstr(p, "title");
        jnode_t * refs = jget(p, "tracks");
        pl->tracks = calloc((size_t)jlen(refs) + 1, sizeof(int32_t));
        for(jnode_t * r = refs ? refs->child : NULL; r; r = r->next) {
            /* "album-id:n", n 1-based */
            char id[128];
            const char * colon = strrchr(r->str, ':');
            if(!colon || colon - r->str >= (long)sizeof(id)) continue;
            memcpy(id, r->str, (size_t)(colon - r->str));
            id[colon - r->str] = '\0';
            int32_t al = find_album(id);
            int32_t n = atoi(colon + 1);
            if(al < 0 || n < 1 || n > lib.albums[al].track_count) continue;
            pl->tracks[pl->track_count++] = lib.albums[al].first_track + n - 1;
        }
    }

    lib.tracks_by_title = malloc(sizeof(int32_t) * (size_t)lib.track_count);
    for(i = 0; i < lib.track_count; i++) lib.tracks_by_title[i] = i;
    qsort(lib.tracks_by_title, (size_t)lib.track_count, sizeof(int32_t), cmp_title);

    /* Resolve cover paths once. LVGL's stdio driver is mounted as 'A'. */
    asset_root = strdup(asset_dir);
    cover_paths = calloc((size_t)lib.album_count * 4, sizeof(char *));
    for(i = 0; i < lib.album_count; i++) {
        for(int s = 0; s < 4; s++) {
            char p[600];
            snprintf(p, sizeof(p), "%s/demo/covers/%s-%d.png", asset_root, lib.albums[i].id, (int)cover_sizes[s]);
            if(access(p, R_OK) != 0) continue;
            char * src = malloc(strlen(p) + 3);
            sprintf(src, "A:%s", p);
            cover_paths[i * 4 + s] = src;
        }
    }
    return true;
}

const keel_library_t * keel_library(void)
{
    return &lib;
}

const char * keel_library_cover(int32_t album, int32_t size)
{
    if(album < 0 || album >= lib.album_count) return NULL;
    for(int s = 0; s < 4; s++)
        if(cover_sizes[s] == size) return cover_paths[album * 4 + s];
    return NULL;
}

typedef struct {
    int32_t size;
    int32_t next;
} prefetch_t;

static void prefetch_cb(lv_timer_t * t)
{
    prefetch_t * pf = lv_timer_get_user_data(t);
    if(pf->next >= lib.album_count) {
        lv_timer_delete(t);
        lv_free(pf);
        return;
    }
    const char * src = keel_library_cover(pf->next++, pf->size);
    lv_image_decoder_dsc_t dsc;
    if(src && lv_image_decoder_open(&dsc, src, NULL) == LV_RESULT_OK) lv_image_decoder_close(&dsc);
}

void keel_library_prefetch_covers(int32_t size)
{
    prefetch_t * pf = lv_malloc(sizeof(*pf));
    pf->size = size;
    pf->next = 0;
    /* 40 ms apart: a decode never shares a frame with the next one. */
    lv_timer_create(prefetch_cb, 40, pf);
}

void keel_format_time(char * buf, int32_t size, int32_t seconds)
{
    if(seconds < 0) seconds = 0;
    if(seconds >= 3600)
        snprintf(buf, (size_t)size, "%d:%02d:%02d", (int)(seconds / 3600), (int)(seconds / 60 % 60), (int)(seconds % 60));
    else
        snprintf(buf, (size_t)size, "%d:%02d", (int)(seconds / 60), (int)(seconds % 60));
}
