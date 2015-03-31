#pragma once

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <exception>
#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_thread.h>


extern "C"
{
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#define MAX_AUDIO_FRAME_SIZE 192000 
#define INBUF_SIZE 4096

//Output PCM
#define OUTPUT_PCM 1
//Use SDL
#define USE_SDL 1

#include "ConcurrentQueue.h"
#include "AudioContext.h"

typedef struct MediaContext {

	AVFormatContext * pFormatCtx;

	AVCodecContext* pVideoCodecCtx;

	AVCodecContext* pAudioCodecCtx;

	AVCodec* pVideoCodec;

	AVCodec* pAudioCodec;

	int video_stream_index;

	int audio_stream_index;

} MediaContext;
//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
static  Uint8  *audio_chunk;
static  Uint32  audio_len;
static  Uint8  *audio_pos;

static ConcurrentQueue<AVPacket> *audio_queue;
static ConcurrentQueue<AVPacket> *video_queue;

void fill_audio(void *udata, Uint8 *stream, int len){
	//SDL 2.0
	SDL_memset(stream, 0, len);
	if (audio_len == 0)		/*  Only  play  if  we  have  data  left  */
		return;
	len = (len > (int)audio_len ? audio_len : len);	/*  Mix  as  much  data  as  possible  */

	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;
}