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

#include "SDLPlayError.h"
#include "ConcurrentQueue.h"
#include "AudioContext.h"
#include "VideoContext.h"

