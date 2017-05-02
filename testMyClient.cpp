/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2017, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
/////////////// Here is modified by mosquito
#include "SDL/SDL.h"
#include "SDL/SDL_thread.h"
#include <map>
#include <string.h>
#include <list>
extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/base64.h>
#include <libavformat/avformat.h>
}
using namespace std;
/////////////////////////
#if 0
struct RTSPConf {
        int initialized;
        char object[RTSPCONF_OBJECT_SIZE];
        char title[RTSPCONF_TITLE_SIZE];
        char display[RTSPCONF_DISPLAY_SIZE];
        char *servername;
        struct sockaddr_in sin;
        int serverport;
        char proto;             // transport layer tcp = 6; udp = 17
        // for controller
        int ctrlenable;
        int ctrlport;
        char ctrlproto;         // transport layer tcp = 6; udp = 17
        int sendmousemotion;
        //
        char *video_encoder_name[RTSPCONF_CODECNAME_SIZE+1];
        AVCodec *video_encoder_codec;
        char *video_decoder_name[RTSPCONF_CODECNAME_SIZE+1];
        AVCodec *video_decoder_codec;
        int video_fps;
        int video_renderer_software;    // 0 - use HW renderer, otherwise SW
        //
        char *audio_encoder_name[RTSPCONF_CODECNAME_SIZE+1];
        AVCodec *audio_encoder_codec;
        char *audio_decoder_name[RTSPCONF_CODECNAME_SIZE+1];
        AVCodec *audio_decoder_codec;
        int audio_bitrate;
        int audio_samplerate;
        int audio_channels;     // XXX: AVFrame->channels is int64_t, use with care
        AVSampleFormat audio_device_format;
        int64_t audio_device_channel_layout;
        AVSampleFormat audio_codec_format;
        int64_t audio_codec_channel_layout;
        std::vector<std::string> *vso;  // video specific options
};
#endif

//struct RTSPConf *rtspconf;
int image_rendered = 0;
static int video_framing = 0;
SDL_Surface *screen;
SDL_Overlay     *bmp = NULL;
struct SwsContext *sws_ctx = NULL;
SDL_Rect rect;
SDL_Event       event;
/////////////////////// here is modified by mosquito
static int video_sess_fmt = -1;
static int audio_sess_fmt = -1;
static const char *video_codec_name = NULL;
static const char *audio_codec_name = NULL;
static enum AVCodecID video_codec_id = AV_CODEC_ID_NONE;
static enum AVCodecID audio_codec_id = AV_CODEC_ID_NONE;
// save files
static FILE *savefp_yuv = NULL;
static FILE *savefp_yuvts = NULL;
///////////////////////////////////
//static map<string,gaConfVar> ga_vars;
#define VIDEO_SOURCE_CHANNEL_MAX 2
#define MAX_FRAMING_SIZE    8
#if 0
struct RTSPThreadParam {
    const char *url;
    bool running;
#ifdef ANDROID
    bool rtpOverTCP;
#endif
    char quitLive555;
    // video
    int width[VIDEO_SOURCE_CHANNEL_MAX];
    int height[VIDEO_SOURCE_CHANNEL_MAX];
    PixelFormat format[VIDEO_SOURCE_CHANNEL_MAX];
#ifdef ANDROID
    JNIEnv *jnienv;
    pthread_mutex_t surfaceMutex[VIDEO_SOURCE_CHANNEL_MAX];
    struct SwsContext *swsctx[VIDEO_SOURCE_CHANNEL_MAX];
    dpipe_t *pipe[VIDEO_SOURCE_CHANNEL_MAX];
#else
    pthread_mutex_t surfaceMutex[VIDEO_SOURCE_CHANNEL_MAX];
#if 1   // only support SDL2
    unsigned int windowId[VIDEO_SOURCE_CHANNEL_MAX];
    SDL_Window *surface[VIDEO_SOURCE_CHANNEL_MAX];
    SDL_Renderer *renderer[VIDEO_SOURCE_CHANNEL_MAX];
    SDL_Texture *overlay[VIDEO_SOURCE_CHANNEL_MAX];
#endif
    struct SwsContext *swsctx[VIDEO_SOURCE_CHANNEL_MAX];
    dpipe_t *pipe[VIDEO_SOURCE_CHANNEL_MAX];
    // audio
    pthread_mutex_t audioMutex;
    bool audioOpened;
#endif
    int videostate;
};
#endif
////////////////// Here is modified by mosquito
FILE *fp_log;
static map<unsigned short,int> port2channel;
static AVCodecContext *vdecoder[VIDEO_SOURCE_CHANNEL_MAX];
static AVFrame *vframe[VIDEO_SOURCE_CHANNEL_MAX];
//static RTSPThreadParam *rtspParam = NULL;
//////////////////////////


// Forward function definitions:

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

// Other event handler functions:
void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void* clientData); // called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);
  // called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// The main streaming routine (for each "rtsp://" URL):
void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL);

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
  return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
  return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment& env, char const* progName) {
  env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
  env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

char eventLoopWatchVariable = 0;

struct ga_codec_entry {
        const char *key;        /**< Key to identify the codec, must be unique */
        enum AVCodecID id;      /**< codec Id: defined in (ffmpeg) AVCodecID */
        const char *mime;       /**< MIME-type name */
        const char *ffmpeg_decoders[4]; /**< The ffmpeg decoder name */
};
#if 0
char *
ga_conf_readv(const char *key, char *store, int slen) {
    map<string,gaConfVar>::iterator mi; 
    if((mi = ga_vars.find(key)) == ga_vars.end())
        return NULL;
    if(mi->second.value().c_str() == NULL)
        return NULL;
    if(store == NULL)
        return strdup(mi->second.value().c_str());
    strncpy(store, mi->second.value().c_str(), slen);
    return store;
}

/**
 * Load the value of a parameter as an integer.
 *
 * @param key [in] The parameter to be loaded.
 * @return The integer value of the parameter.
 *
 * Note that when the given parameter is not defined,
 * this function returns zero.
 */
int
ga_conf_readint(const char *key) {
    char buf[64];
    char *ptr = ga_conf_readv(key, buf, sizeof(buf));
    if(ptr == NULL)
        return 0;
    return strtol(ptr, NULL, 0); 
}
#endif

static FILE *
ga_save_init_internal(const char *filename, const char *mode) {
    FILE *fp = NULL;
if(filename != NULL) {
        fp = fopen(filename, mode);
        if(fp == NULL) {
            fprintf(fp_log,"save file: open %s failed.\n", filename);
        }    
    }    
    return fp;
}

/**
 * Return the FILE pointer used to store saved raw image frame data.
 *
 * @param filename [in] Pathname to store the image data.
 * @return FILE pointer to the opened file.
 *
 * This function must be called if you plan to use the
 * save raw video image feature.
 * All captured images will be stored in this single file.
 */
FILE *
ga_save_init(const char *filename) {
    return ga_save_init_internal(filename, "wb");
}

FILE *
ga_save_init_txt(const char *filename) {
    return ga_save_init_internal(filename, "wt");
}
int
ga_save_yuv420p(FILE *fp, int w, int h, unsigned char *planes[], int linesize[]) {
    int i, j, wlen, written = 0; 
    int w2 = w/2; 
    int h2 = h/2; 
    int expected = w * h * 3 / 2; 
    unsigned char *src;
    if(fp == NULL || w <= 0 || h <= 0 || planes == NULL || linesize == NULL)
        return -1;
    // write Y
    src = planes[0];
    for(i = 0; i < h; i++) {
        if((wlen = fwrite(src, sizeof(char), w, fp)) < 0) 
            goto err_save_yuv;
        written += wlen;
        src += linesize[0];
    }    
    // write U/V
    for(j = 1; j < 3; j++) {
        src = planes[j];
        for(i = 0; i < h2; i++) {
            if((wlen = fwrite(src, sizeof(char), w2, fp)) < 0) 
                goto err_save_yuv;
            written += wlen;
            src += linesize[j];
        }    
    }    
    //   
    if(written != expected) {
        //ga_error("save YUV (%dx%d): expected %d, save %d (frame may be corrupted)\n",
    //        w, h, expected, written);
    }    
    //   
    fflush(fp);
    return written;
err_save_yuv:
    return -1;
}

typedef struct dpipe_buffer_s {
    void *pointer;      /**< pointer to a frame buffer. Aligned to 8-byte address: is equivalent to internal + offset */
    void *internal;     /**< internal pointer to the allocated buffer space. Used with malloc() and free(). */
    int offset;     /**< data pointer offset from internal */
    struct dpipe_buffer_s *next;    /**< pointer to the next dpipe frame buffer */
}   dpipe_buffer_t;
/**
 * The codec table.
 */
struct ga_codec_entry ga_codec_table[] = {
        { "H264", AV_CODEC_ID_H264, "video/avc", { "h264", NULL } },
        { "H265", AV_CODEC_ID_H265, "video/hevc", { "hevc", NULL } },
        { "VP8", AV_CODEC_ID_VP8, "video/x-vnd.on2.vp8", { "libvpx", NULL } },
        { "MPA", AV_CODEC_ID_MP3, "audio/mpeg", { "mp3", NULL } },
        { "OPUS", AV_CODEC_ID_OPUS, "audio/opus", { "libopus", NULL } },
        { NULL, AV_CODEC_ID_NONE, NULL, { NULL } } /* END */
};

/**
 * Get the codec entry from the codec table by key.
 */
static ga_codec_entry *
ga_lookup_core(const char *key) {
        int i = 0;
        while(i >= 0 && ga_codec_table[i].key != NULL) {
                if(strcasecmp(ga_codec_table[i].key, key) == 0)
                        return &ga_codec_table[i];
                i++;
        }
        return NULL;
}

/**
 * Get the codec ffmpeg decoders from the codec table by key.
 */
const char **
ga_lookup_ffmpeg_decoders(const char *key) {
        struct ga_codec_entry * e = ga_lookup_core(key);
        if(e==NULL || e->ffmpeg_decoders==NULL) {
                //ga_error("ga_lookup[%s]: ffmpeg decoders not found\n", key);
		return NULL;
        }
        return e->ffmpeg_decoders;
}

/**
 * Get the codec Id from the codec table by key.
 */
enum AVCodecID
ga_lookup_codec_id(const char *key) {
        struct ga_codec_entry * e = ga_lookup_core(key);
        if(e==NULL) {
                //ga_error("ga_lookup[%s]: codec id not found\n", key);
                return AV_CODEC_ID_NONE;
        }
        return e->id;
}

AVCodec*
ga_avcodec_find_decoder(const char **names, enum AVCodecID cid) {
        AVCodec *codec = NULL;
        if(names != NULL) {
                while(*names != NULL) {

             if((codec = avcodec_find_decoder_by_name(*names)) != NULL)
                                return codec;
                        names++;
                }
        }
        if(cid != AV_CODEC_ID_NONE)
                return avcodec_find_decoder(cid);
        return NULL;
}

static int
play_video_priv(int ch/*channel*/, unsigned char *buffer, int bufsize, struct timeval pts) {
    AVPacket avpkt;
    int got_picture, len;
    dpipe_buffer_t *data = NULL;
    AVPicture *dstframe = NULL;
    struct timeval ftv;
    static unsigned fcount = 0;
#ifdef PRINT_LATENCY
    static struct timeval btv0 = {0, 0};
    struct timeval ptv0, ptv1, btv1;
    // measure buffering time
    if(btv0.tv_sec == 0) {
        gettimeofday(&btv0, NULL);
    } else {
        long long dt;
        gettimeofday(&btv1, NULL);
        dt = tvdiff_us(&btv1, &btv0);
        if(dt < 2000000) {
           // ga_aggregated_print(0x8000, 599, dt);
        }
        btv0 = btv1;
    }
#endif
#if 0   
 // drop the frame?
    if(drop_video_frame(ch, buffer, bufsize, pts)) {
        return bufsize;
    }
#endif
    //
#ifdef SAVE_ENC
    if(fout != NULL) {
        fwrite(buffer, sizeof(char), bufsize, fout);
    }
#endif
    //
    av_init_packet(&avpkt);
    avpkt.size = bufsize;
    avpkt.data = buffer;
    if(vdecoder[ch] == NULL) puts("GO DIE DIE");
	//
    while(avpkt.size > 0) {
        //
#ifdef PRINT_LATENCY
        gettimeofday(&ptv0, NULL);
#endif
        if((len = avcodec_decode_video2(vdecoder[ch], vframe[ch], &got_picture, &avpkt)) < 0) {
            //fprintf(fp_log,"decode video frame %d error\n", frame);
            break;
        }
        if(got_picture) {
            //puts("YESSSSSSSSSS");
#ifdef COUNT_FRAME_RATE
            cf_frame[ch]++;
            if(cf_tv0[ch].tv_sec == 0) {
                gettimeofday(&cf_tv0[ch], NULL);
            }
            if(cf_frame[ch] == COUNT_FRAME_RATE) {
                gettimeofday(&cf_tv1[ch], NULL);
                cf_interval[ch] = tvdiff_us(&cf_tv1[ch], &cf_tv0[ch]);
                fprintf(fp_log,"# %u.%06u player frame rate: decoder %d @ %.4f fps\n",
                    cf_tv1[ch].tv_sec,
                    cf_tv1[ch].tv_usec,
                    ch,
                    1000000.0 * cf_frame[ch] / cf_interval[ch]);
                cf_tv0[ch] = cf_tv1[ch];
                cf_frame[ch] = 0;
            }
#endif
#if 0        
    // create surface & bitmap for the first time
            pthread_mutex_lock(&rtspParam->surfaceMutex[ch]);
            if(rtspParam->swsctx[ch] == NULL) {
                rtspParam->width[ch] = vframe[ch]->width;
                rtspParam->height[ch] = vframe[ch]->height;
                rtspParam->format[ch] = (PixelFormat) vframe[ch]->format;
#ifdef ANDROID
                create_overlay(ch, vframe[0]->width, vframe[0]->height, (PixelFormat) vframe[0]->format);
#else
                pthread_mutex_unlock(&rtspParam->surfaceMutex[ch]);
                bzero(&evt, sizeof(evt));
                evt.user.type = SDL_USEREVENT;
                evt.user.timestamp = time(0);
                evt.user.code = SDL_USEREVENT_CREATE_OVERLAY;
                evt.user.data1 = rtspParam;
                evt.user.data2 = (void*) ch;
                SDL_PushEvent(&evt);
                // skip the initial frame:
                // for event handler to create/setup surfaces
                goto skip_frame;
#endif
            }
            pthread_mutex_unlock(&rtspParam->surfaceMutex[ch]);
            // copy into pool
            data = dpipe_get(rtspParam->pipe[ch]);
            dstframe = (AVPicture*) data->pointer;
            sws_scale(rtspParam->swsctx[ch],
                // source: decoded frame
                vframe[ch]->data, vframe[ch]->linesize,
                0, vframe[ch]->height,
                // destination: texture
                dstframe->data, dstframe->linesize);
#endif     

bmp = SDL_CreateYUVOverlay( vframe[0]->width,  vframe[0]->height,
                           SDL_YV12_OVERLAY, screen);

// initialize SWS context for software scaling
sws_ctx = sws_getContext( vframe[0]->width,
                          vframe[0]->height,
			  vdecoder[0]->pix_fmt,
			  vframe[0]->width,
			  vframe[0]->height,
			 AV_PIX_FMT_YUV420P,
			 SWS_BILINEAR,
			 NULL,
			 NULL,
			 NULL);
    SDL_LockYUVOverlay(bmp);

    AVPicture pict;
    pict.data[0] = bmp->pixels[0];
    pict.data[1] = bmp->pixels[2];
    pict.data[2] = bmp->pixels[1];

    pict.linesize[0] = bmp->pitches[0];
    pict.linesize[1] = bmp->pitches[2];
    pict.linesize[2] = bmp->pitches[1];

    // Convert the image into YUV format that SDL uses
    sws_scale(sws_ctx, (uint8_t const * const *)vframe[0]->data,
	      vframe[0]->linesize, 0, vframe[0]->height,
	      pict.data, pict.linesize);
    
    SDL_UnlockYUVOverlay(bmp);

	rect.x = 0;
	rect.y = 0;
	rect.w = vframe[0]->width;
	rect.h = vframe[0]->height;
	SDL_DisplayYUVOverlay(bmp, &rect);

            //dstframe = (AVPicture*) data->pointer;
       if(ch==0 && savefp_yuv != NULL) {
		ga_save_yuv420p(savefp_yuv, vframe[0]->width, vframe[0]->height, vframe[0]->data, vframe[0]->linesize);
                if(savefp_yuvts != NULL) {
                    gettimeofday(&ftv, NULL);
                    //ga_save_printf(savefp_yuvts, "Frame #%08d: %u.%06u\n", fcount++, ftv.tv_sec, ftv.tv_usec);
                }
            }
            //dpipe_store(rtspParam->pipe[ch], data);
            // request to render it
#ifdef PRINT_LATENCY
            gettimeofday(&ptv1, NULL);
            //ga_aggregated_print(0x8001, 601, tvdiff_us(&ptv1, &ptv0));
#endif
#ifdef ANDROID
            //requestRender(rtspParam->jnienv);
#else
            //bzero(&evt, sizeof(evt));
            //evt.user.type = SDL_USEREVENT;
            //evt.user.timestamp = time(0);
            //evt.user.code = SDL_USEREVENT_RENDER_IMAGE;
            //evt.user.data1 = rtspParam;
            //evt.user.data2 = (void*) ch;
            //SDL_PushEvent(&evt);
#endif
        }
skip_frame:
        avpkt.size -= len;
        avpkt.data += len;
    }
    return avpkt.size;
}


static unsigned char *
decode_sprop(AVCodecContext *ctx, const char *sprop) {
    unsigned char startcode[] = {0, 0, 0, 1};
    printf("ctx size :%d\n",ctx->extradata_size); 
    printf("sprop size :%d\n",strlen(sprop)); 
    int sizemax = ctx->extradata_size + strlen(sprop) * 3;
    unsigned char *extra = (unsigned char*) malloc(sizemax);
    unsigned char *dest = extra;
    int extrasize = 0;
    int spropsize = strlen(sprop);
    char *mysprop = strdup(sprop);
    unsigned char *tmpbuf = (unsigned char *) strdup(sprop);
    char *s0 = mysprop, *s1;
    // already have extradata?
    if(ctx->extradata) {
        bcopy(ctx->extradata, extra, ctx->extradata_size);
        extrasize = ctx->extradata_size;
        dest += extrasize;
    }
    // start converting
    while(*s0) {
        int blen, more = 0;
        for(s1 = s0; *s1; s1++) {
            if(*s1 == ',' || *s1 == '\0')
                break;
        }
        if(*s1 == ',')
            more = 1;
        *s1 = '\0';
        if((blen = av_base64_decode(tmpbuf, s0, spropsize)) > 0) {
            int offset = 0;
            // no start code?
            if(memcmp(startcode, tmpbuf, sizeof(startcode)) != 0) {
                bcopy(startcode, dest, sizeof(startcode));
                offset += sizeof(startcode);
            }
            bcopy(tmpbuf, dest + offset, blen);
            dest += offset + blen;
            extrasize += offset + blen;
        }
        s0 = s1;
        if(more) {
            s0++;
        }
    }
    // release
    free(mysprop);
    free(tmpbuf);
    // show decoded sprop
    if(extrasize > 0) {
        if(ctx->extradata)
            free(ctx->extradata);
        ctx->extradata = extra;
        ctx->extradata_size = extrasize;
#ifdef SAVE_ENC
        if(fout != NULL) {
            fwrite(extra, sizeof(char), extrasize, fout);
        }
#endif
        return ctx->extradata;
    }
    free(extra);
    return NULL;
}


int
init_vdecoder(int channel, const char *sprop) {
    AVCodec *codec = NULL; //rtspconf->video_decoder_codec;
    AVCodecContext *ctx;
    AVFrame *frame;
    const char **names = NULL;
    //
    if(channel > VIDEO_SOURCE_CHANNEL_MAX) {
        fprintf(fp_log,"video decoder(%d): too many decoders.\n", channel);
        return -1;
    }
    //
    if(video_codec_name == NULL) {
        fprintf(fp_log,"video decoder: no codec specified.\n");
        return -1;
    }

    if((names = ga_lookup_ffmpeg_decoders(video_codec_name)) == NULL) {
        fprintf(fp_log,"video decoder: cannot find decoder names for %s\n", video_codec_name);
        return -1;
    }
    video_codec_id = ga_lookup_codec_id(video_codec_name);
    

    if((codec = ga_avcodec_find_decoder(names, AV_CODEC_ID_NONE)) == NULL) {
        fprintf(fp_log,"video decoder: cannot find the decoder for %s\n", video_codec_name);
        return -1;
    }
    //rtspconf->video_decoder_codec = codec;
    fprintf(fp_log,"video decoder: use decoder %s\n", names[0]);
    //
    if((frame = av_frame_alloc()) == NULL) {
        fprintf(fp_log,"video decoder(%d): allocate frame failed\n", channel);
        return -1;
    }
    if((ctx = avcodec_alloc_context3(codec)) == NULL) {
        fprintf(fp_log,"video decoder(%d): cannot allocate context\n", channel);
        return -1;
    }
    if(codec->capabilities & CODEC_CAP_TRUNCATED) {
       	fprintf(fp_log,"video decoder(%d): codec support truncated data\n", channel);
        ctx->flags |= CODEC_FLAG_TRUNCATED;
    }

    if(sprop != NULL) {
        if(decode_sprop(ctx, sprop) != NULL) {
            int extrasize = ctx->extradata_size;
            fprintf(fp_log,"video decoder(%d): sprop configured with '%s', decoded-size=%d\n",
                channel, sprop, extrasize);
            fprintf(stderr, "SPROP = [");
            for(unsigned char *ptr = ctx->extradata; extrasize > 0; extrasize--) {
                fprintf(stderr, " %02x", *ptr++);
            }
            fprintf(stderr, " ]\n");
        }
    }
    if(avcodec_open2(ctx, codec, NULL) != 0) {
        fprintf(fp_log,"video decoder(%d): cannot open decoder\n", channel);
        return -1;
    }
    fprintf(fp_log,"video decoder(%d): codec %s (%s)\n", channel, codec->name, codec->long_name);
    //
//printf("channel=%d\n",channel);    
vdecoder[channel] = ctx;
    vframe[channel] = frame;
	screen = SDL_SetVideoMode(vframe[channel]->width, vframe[channel]->height,0, 0);
    if(!screen) {
  	 fprintf(stderr, "SDL: could not set video mode - exiting\n");
 	 exit(1);
    }

    return 0;
}

////////////////////////// Here is modified by mosquito
#define PRIVATE_BUFFER_SIZE     1048576

struct decoder_buffer {
        unsigned int privbuflen;
        unsigned char *privbuf;
        struct timeval lastpts;
        // for alignment
        unsigned int offset;
        unsigned char *privbuf_unaligned;
};

static struct decoder_buffer db[VIDEO_SOURCE_CHANNEL_MAX];

static void
deinit_decoder_buffer() {
        int i;
        for(i = 0; i < VIDEO_SOURCE_CHANNEL_MAX; i++) {
                if(db[i].privbuf_unaligned != NULL) {
                        free(db[i].privbuf_unaligned);
                }
        }
        bzero(db, sizeof(db));
        return;
}
static int
init_decoder_buffer() {
        int i;
        //
        deinit_decoder_buffer();
        //
        for(i = 0; i < VIDEO_SOURCE_CHANNEL_MAX; i++) {
                db[i].privbuf_unaligned = (unsigned char*) malloc(PRIVATE_BUFFER_SIZE+16);
                if(db[i].privbuf_unaligned == NULL) {
                        fprintf(fp_log,"FATAL: cannot allocate private buffer (%d:%d bytes): %s\n",
                                i, PRIVATE_BUFFER_SIZE, strerror(errno));
                        goto adb_failed;
                }
#ifdef __LP64__ /* 64-bit */
                db[i].offset = 16 - (((unsigned long long) db[i].privbuf_unaligned) & 0x0f);
#else
                db[i].offset = 16 - (((unsigned) db[i].privbuf_unaligned) & 0x0f);
#endif
                db[i].privbuf = db[i].privbuf_unaligned + db[i].offset;
        }
return 0;
adb_failed:
        deinit_decoder_buffer();
        return -1;
}
///////////////////////////////

static void
play_video(int channel, unsigned char *buffer, int bufsize, struct timeval pts, bool marker) {
    struct decoder_buffer *pdb = &db[channel];
    int left;
    //
    if(bufsize <= 0 || buffer == NULL) {
        fprintf(fp_log,"empty buffer?\n");
        return;
    }
    if(pts.tv_sec != pdb->lastpts.tv_sec
    || pts.tv_usec != pdb->lastpts.tv_usec) {
        if(pdb->privbuflen > 0) {
            //fprintf(stderr, "DEBUG: video pts=%08ld.%06ld\n",
            //  lastpts.tv_sec, lastpts.tv_usec);
            left = play_video_priv(channel, pdb->privbuf,
                pdb->privbuflen, pdb->lastpts);
            if(left > 0) {
                bcopy(pdb->privbuf + pdb->privbuflen - left,
                    pdb->privbuf, left);
                pdb->privbuflen = left;
                fprintf(fp_log,"decoder: %d bytes left, preserved for next round\n", left);
            } else {
                pdb->privbuflen = 0;
            }
        }
        pdb->lastpts = pts;
    }
    if(pdb->privbuflen + bufsize <= PRIVATE_BUFFER_SIZE) {
        bcopy(buffer, &pdb->privbuf[pdb->privbuflen], bufsize);
        pdb->privbuflen += bufsize;
        if(marker && pdb->privbuflen > 0) {
            left = play_video_priv(channel, pdb->privbuf,
                pdb->privbuflen, pdb->lastpts);
            if(left > 0) {
                bcopy(pdb->privbuf + pdb->privbuflen - left,
                    pdb->privbuf, left);
                pdb->privbuflen = left;
                fprintf(fp_log,"decoder: %d bytes left, leave for next round\n", left);
            } else {
                pdb->privbuflen = 0;
            }
        }
    } else {
        fprintf(fp_log,"WARNING: video private buffer overflow.\n");
        left = play_video_priv(channel, pdb->privbuf,
                pdb->privbuflen, pdb->lastpts);
        if(left > 0) {
            bcopy(pdb->privbuf + pdb->privbuflen - left,
                pdb->privbuf, left);
            pdb->privbuflen = left;
            fprintf(fp_log,"decoder: %d bytes left, leave for next round\n", left);
        } else {
            pdb->privbuflen = 0;
        }
    }
#ifdef ANDROID
    }
#endif
    return;
}

int main(int argc, char** argv) {
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);
  char savefile_yuv[128];
  char savefile_yuvts[128];
	av_register_all();
        avcodec_register_all();
        avformat_network_init();
    //drop_video_frame_init(ga_conf_readint("max-tolerable-video-delay"));

  // We need at least one "rtsp://" URL argument:
  if (argc < 2) {
    usage(*env, argv[0]);
    return 1;
  }
  //
	strcpy(savefile_yuv,"saveyuv.yuv");
	strcpy(savefile_yuvts,"saveyuvts.yuv");
// save-file features
    if(savefp_yuv != NULL)
        fclose(savefp_yuv);
    if(savefp_yuvts != NULL)
        fclose(savefp_yuvts);
    savefp_yuv = savefp_yuvts = NULL;
    //
    //if(ga_conf_readv("save-yuv-image", savefile_yuv, sizeof(savefile_yuv)) != NULL)
       
 savefp_yuv = ga_save_init(savefile_yuv);
    if(savefp_yuv != NULL)
    //&& ga_conf_readv("save-yuv-image-timestamp", savefile_yuvts, sizeof(savefile_yuvts)) != NULL)
        savefp_yuvts = ga_save_init_txt(savefile_yuvts);
  fp_log = fopen("error.log","wb");
    fprintf(fp_log,"*** SAVEFILE: YUV image saved to '%s'; timestamp saved to '%s'.\n",
        savefp_yuv   ? savefile_yuv   : "NULL",
        savefp_yuvts ? savefile_yuvts : "NULL");
//if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
//  fprintf(fp_log, "Could not initialize SDL - %s\n", SDL_GetError());
//  exit(1);
//}
if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        fprintf(fp_log,"SDL init failed: %s\n", SDL_GetError());
        return -1; 
    }   
 
/////////////////////// Here is modified by mosquito
//savefp_yuv = fopen("savefile.yuv","wb+");
  port2channel.clear();
 //////////////////////////

    if(init_decoder_buffer() < 0) {
        fprintf(fp_log,"init decode buffer failed.\n");
        return -1;
    }

  // There are argc-1 URLs: argv[1] through argv[argc-1].  Open and start streaming each one:
  for (int i = 1; i <= argc-1; ++i) {
    openURL(*env, argv[0], argv[i]);
  }

  // All subsequent activity takes place within the event loop:
  env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
    // This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.
  
///////////// Here is modified by mosquito
  fclose(fp_log);
  fclose(savefp_yuv);
/////////////////////
  return 0;

  // If you choose to continue the application past this point (i.e., if you comment out the "return 0;" statement above),
  // and if you don't intend to do anything more with the "TaskScheduler" and "UsageEnvironment" objects,
  // then you can also reclaim the (small) memory used by these objects by uncommenting the following code:
  /*
    env->reclaim(); env = NULL;
    delete scheduler; scheduler = NULL;
  */
}

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

class StreamClientState {
public:
  StreamClientState();
  virtual ~StreamClientState();

public:
  MediaSubsessionIterator* iter;
  MediaSession* session;
  MediaSubsession* subsession;
  TaskToken streamTimerTask;
  double duration;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:

class ourRTSPClient: public RTSPClient {
public:
  static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
				  int verbosityLevel = 0,
				  char const* applicationName = NULL,
				  portNumBits tunnelOverHTTPPortNum = 0);

protected:
  ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
    // called only by createNew();
  virtual ~ourRTSPClient();

public:
  StreamClientState scs;
};

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.

class DummySink: public MediaSink {
public:
  static DummySink* createNew(UsageEnvironment& env,
			      MediaSubsession& subsession, // identifies the kind of data that's being received
			      char const* streamId = NULL); // identifies the stream itself (optional)

private:
  DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
    // called only by "createNew()"
  virtual ~DummySink();

  static void afterGettingFrame(void* clientData, unsigned frameSize,
                                unsigned numTruncatedBytes,
				struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
			 struct timeval presentationTime, unsigned durationInMicroseconds);

private:
  // redefined virtual functions:
  virtual Boolean continuePlaying();

private:
  u_int8_t* fReceiveBuffer;
  MediaSubsession& fSubsession;
  char* fStreamId;
};

#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"

static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.

void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL) {
  // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
  // to receive (even if more than stream uses the same "rtsp://" URL).
  RTSPClient* rtspClient = ourRTSPClient::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
  if (rtspClient == NULL) {
    env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
    return;
  }

  ++rtspClientCount;

  // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
  // Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
  // Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
  rtspClient->sendDescribeCommand(continueAfterDESCRIBE); 
}


// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
      delete[] resultString;
      break;
    }

    char* const sdpDescription = resultString;
    env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

    // Create a media session object from this SDP description:
    scs.session = MediaSession::createNew(env, sdpDescription);
    delete[] sdpDescription; // because we don't need it anymore
    if (scs.session == NULL) {
      env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    } else if (!scs.session->hasSubsessions()) {
      env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }

    // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
    // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
    // (Each 'subsession' will have its own data source.)
    scs.iter = new MediaSubsessionIterator(*scs.session);
    setupNextSubsession(rtspClient);
    return;
  } while (0);

  // An unrecoverable error occurred with this stream.
  shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP False

void setupNextSubsession(RTSPClient* rtspClient) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
  
  scs.subsession = scs.iter->next();
  if (scs.subsession != NULL) {
    if (!scs.subsession->initiate()) {
      env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
      setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
    } else {
///////////////////// Here is modified by mosquito
	char vparam[1024];
      	const char *pvparam = NULL;
        video_sess_fmt = scs.subsession->rtpPayloadFormat();
        video_codec_name = strdup(scs.subsession->codecName());
        //qos_add_source(video_codec_name, scs.subsession->rtpSource());
                                
	//scs.subsession->rtpSource()->setAuxilliaryReadHandler(pktloss_monitor, NULL);                             
	//if(rtp_packet_reordering_threshold > 0)
	//	 scs.subsession->rtpSource()->setPacketReorderingThresholdTime(rtp_packet_reordering_threshold);
                
        if(port2channel.find(scs.subsession->clientPortNum()) == port2channel.end()) {
		int cid = port2channel.size();
		port2channel[scs.subsession->clientPortNum()] = cid;
       		fprintf(fp_log,"Codec type: %s.",video_codec_name);
		video_codec_id = ga_lookup_codec_id(video_codec_name);
        	if(!strcmp(video_codec_name,"H264")) {
			fprintf(fp_log,"I am H264!!!!!!!!!!!!!!!");
                     	pvparam = scs.subsession->fmtp_spropparametersets();
         	} else if(AV_CODEC_ID_H265 == video_codec_id/*!strcmp(video_codec_name,"H265")*/) {
			fprintf(fp_log,"I am H265!!!!!!!!!!!!!!!");
             		snprintf(vparam, sizeof(vparam), "%s,%s,%s",
                        scs.subsession->fmtp_spropvps()==NULL ? "" : scs.subsession->fmtp_spropvps(),
                 	scs.subsession->fmtp_spropsps()==NULL ? "" : scs.subsession->fmtp_spropsps(),
                        scs.subsession->fmtp_sproppps()==NULL ? "" : scs.subsession->fmtp_sproppps());
                        pvparam = vparam;	
		} else {
      	                pvparam = NULL;
                }
          	if(init_vdecoder(cid, pvparam/*scs.subsession->fmtp_spropparametersets()*/) < 0) {
          		fprintf(fp_log,"cannot initialize video decoder(%d)\n", cid);
            		//rtspParam->quitLive555 = 1;
                        return;
              	}
             	fprintf(fp_log,"video decoder(%d) initialized (client port %d)\n",
           		cid, scs.subsession->clientPortNum());
                                   
                                        
        }

//////////////////////////////

      env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
      if (scs.subsession->rtcpIsMuxed()) {
	env << "client port " << scs.subsession->clientPortNum();
      } else {
	env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
      }
      env << ")\n";

      // Continue setting up this subsession, by sending a RTSP "SETUP" command:
      rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
    }
    return;
  }

  // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
  if (scs.session->absStartTime() != NULL) {
    // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
  } else {
    scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
  }
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
      break;
    }

    env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
    if (scs.subsession->rtcpIsMuxed()) {
      env << "client port " << scs.subsession->clientPortNum();
    } else {
      env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
    }
    env << ")\n";

    // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
    // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
    // after we've sent a RTSP "PLAY" command.)

    scs.subsession->sink = DummySink::createNew(env, *scs.subsession, rtspClient->url());
      // perhaps use your own custom "MediaSink" subclass instead
    if (scs.subsession->sink == NULL) {
      env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
	  << "\" subsession: " << env.getResultMsg() << "\n";
      break;
    }

    env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
    scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession 
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
				       subsessionAfterPlaying, scs.subsession);
    // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
    if (scs.subsession->rtcpInstance() != NULL) {
      scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
    }
  } while (0);
  delete[] resultString;

  // Set up the next subsession, if any:
  setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
  Boolean success = False;

  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
      break;
    }

    // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
    // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
    // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
    // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
    if (scs.duration > 0) {
      unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
      scs.duration += delaySlop;
      unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
      scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
    }

    env << *rtspClient << "Started playing session";
    if (scs.duration > 0) {
      env << " (for up to " << scs.duration << " seconds)";
    }
    env << "...\n";

    success = True;
  } while (0);
  delete[] resultString;

  if (!success) {
    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
  }
}


// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

  // Begin by closing this subsession's stream:
  Medium::close(subsession->sink);
  subsession->sink = NULL;

  // Next, check whether *all* subsessions' streams have now been closed:
  MediaSession& session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL) {
    if (subsession->sink != NULL) return; // this subsession is still active
  }

  // All subsessions' streams have now been closed, so shutdown the client:
  shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
  UsageEnvironment& env = rtspClient->envir(); // alias

  env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

  // Now act as if the subsession had closed:
  subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
  ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
  StreamClientState& scs = rtspClient->scs; // alias

  scs.streamTimerTask = NULL;

  // Shut down the stream:
  shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

  // First, check whether any subsessions have still to be closed:
  if (scs.session != NULL) { 
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.session);
    MediaSubsession* subsession;

    while ((subsession = iter.next()) != NULL) {
      if (subsession->sink != NULL) {
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	if (subsession->rtcpInstance() != NULL) {
	  subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
	}

	someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive) {
      // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
      // Don't bother handling the response to the "TEARDOWN".
      rtspClient->sendTeardownCommand(*scs.session, NULL);
    }
  }

  env << *rtspClient << "Closing the stream.\n";
  Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

  if (--rtspClientCount == 0) {
    // The final stream has ended, so exit the application now.
    // (Of course, if you're embedding this code into your own application, you might want to comment this out,
    // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
    exit(exitCode);
  }
}


// Implementation of "ourRTSPClient":

ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
					int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
  return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
			     int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {
}

ourRTSPClient::~ourRTSPClient() {
}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
  : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() {
  delete iter;
  if (session != NULL) {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    UsageEnvironment& env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }
}


// Implementation of "DummySink":

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 1048576

DummySink* DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId) {
  return new DummySink(env, subsession, streamId);
}

DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
  : MediaSink(env),
    fSubsession(subsession) {
  fStreamId = strDup(streamId);
  fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE+MAX_FRAMING_SIZE];
    // setup framing if necessary
    // H264 framing code
    if(strcmp("H264", fSubsession.codecName()) == 0
    || strcmp("H265", fSubsession.codecName()) == 0) {
        video_framing = 4;
        fReceiveBuffer[MAX_FRAMING_SIZE-video_framing+0]
        = fReceiveBuffer[MAX_FRAMING_SIZE-video_framing+1]
        = fReceiveBuffer[MAX_FRAMING_SIZE-video_framing+2] = 0;
        fReceiveBuffer[MAX_FRAMING_SIZE-video_framing+3] = 1;
    }
    return;
}

DummySink::~DummySink() {
  delete[] fReceiveBuffer;
  delete[] fStreamId;
}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds) {
  DummySink* sink = (DummySink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1

void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
 if(fSubsession.rtpPayloadFormat() == video_sess_fmt) {
        bool marker = false;
        int channel = port2channel[fSubsession.clientPortNum()];
	int lost = 0, count = 1;
        RTPSource *rtpsrc = fSubsession.rtpSource();
        RTPReceptionStatsDB::Iterator iter(rtpsrc->receptionStatsDB());
        RTPReceptionStats* stats = iter.next(True);
        if(rtpsrc != NULL) {
            marker = rtpsrc->curPacketMarkerBit();
        }
        //
        if(stats != NULL) {
            //lost = pktloss_monitor_get(stats->SSRC(), &count, 1/*reset*/);
        }
        //
        play_video(channel,
            fReceiveBuffer+MAX_FRAMING_SIZE-video_framing,
            frameSize+video_framing, presentationTime,
            marker);
} 
  // Then continue, to request the next frame of data:
  continuePlaying();
}

Boolean DummySink::continuePlaying() {
  if (fSource == NULL) return False; // sanity check (should not happen)

  // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
  fSource->getNextFrame(fReceiveBuffer+MAX_FRAMING_SIZE, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);
  return True;
}
