#pragma once

#include "cmdutils.h"
#include "../sdl.h"


static constexpr char program_name[] = "ffplay";
static constexpr int program_birth_year = 2003;

static constexpr unsigned sws_flags = SWS_BICUBIC;

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* Step size for volume control in dB */
#define SDL_VOLUME_STEP (0.75)

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

#define CURSOR_HIDE_DELAY 1000000

#define USE_ONEPASS_SUBTITLE_RENDER 1

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

enum ShowMode {
    SHOW_MODE_NONE = -1, 
    SHOW_MODE_VIDEO = 0, 
    SHOW_MODE_WAVES, 
    SHOW_MODE_RDFT, 
    SHOW_MODE_NB
};

struct FFPlay {
    /* options specified by the user */
    AVInputFormat* file_iformat;
    const char* input_filename;
    const char* window_title;
    int default_width = 640;
    int default_height = 480;
    int screen_width = 0;
    int screen_height = 0;
    int screen_left = SDL_WINDOWPOS_CENTERED;
    int screen_top = SDL_WINDOWPOS_CENTERED;
    int audio_disable;
    int video_disable;
    int subtitle_disable;
    const char* wanted_stream_spec[AVMEDIA_TYPE_NB] = { 0 };
    int seek_by_bytes = -1;
    float seek_interval = 10;
    int display_disable;
    int borderless;
    int alwaysontop;
    int startup_volume = 100;
    int show_status = 1;
    int av_sync_type = AV_SYNC_AUDIO_MASTER;
    int64_t start_time = AV_NOPTS_VALUE;
    int64_t duration = AV_NOPTS_VALUE;
    int fast = 0;
    int genpts = 0;
    int lowres = 0;
    int decoder_reorder_pts = -1;
    int autoexit;
    int exit_on_keydown;
    int exit_on_mousedown;
    int loop = 1;
    int framedrop = -1;
    int infinite_buffer = -1;
    enum ShowMode show_mode = SHOW_MODE_NONE;
    const char* audio_codec_name;
    const char* subtitle_codec_name;
    const char* video_codec_name;
    double rdftspeed = 0.02;
    int64_t cursor_last_shown;
    int cursor_hidden = 0;
#if CONFIG_AVFILTER
    const char** vfilters_list = NULL;
    int nb_vfilters = 0;
    char* afilters = NULL;
#endif
    int autorotate = 1;
    int find_stream_info = 1;
    int filter_nbthreads = 0;

    /* current context */
    int is_full_screen;
    int64_t audio_callback_time;

    AVPacket flush_pkt;

#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_RendererInfo renderer_info = { 0 };
    SDL_AudioDeviceID audio_dev;
};

extern FFPlay ffplay;
