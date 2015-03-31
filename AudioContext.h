#pragma once
#include "stdafx.h"

class AudioContext
{
public:
	AudioContext(AVCodecContext *pCodecCtx);
	~AudioContext();

	AVCodecContext*	 pCodecCtx;
	AVCodec*		 pCodec;

	int out_nb_samples;
	int out_sample_rate;
	int out_channels;
	int out_buffer_size;

	uint64_t out_channel_layout			= AV_CH_LAYOUT_STEREO;
	uint64_t in_channel_layout;
	AVSampleFormat out_sample_fmt		= AV_SAMPLE_FMT_S16;

	uint8_t				*out_buffer;		// pcm data
};

AudioContext::AudioContext(AVCodecContext *pCodecCtx)
{
	out_nb_samples = 1024;
	out_sample_rate = 48000;
	out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
	out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

	out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 3 / 2);

	this->pCodecCtx = pCodecCtx;

	in_channel_layout = av_get_default_channel_layout(pCodecCtx->channels);
}

AudioContext::~AudioContext()
{
	if (pCodecCtx)
	{
		avcodec_free_context(&pCodecCtx);
	}
}


