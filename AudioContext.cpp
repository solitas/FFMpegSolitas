#include "stdafx.h"
#include "AudioContext.h"


AudioContext::AudioContext(AVCodecContext *pCodecCtx)
{
	out_nb_samples = 1024;
	out_sample_rate = 48000;
	out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
	out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

	out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 3 / 2);

	this->pCodecCtx = pCodecCtx;

	in_channel_layout = av_get_default_channel_layout(pCodecCtx->channels);

	//Swr
	au_convert_ctx = swr_alloc();
	au_convert_ctx = swr_alloc_set_opts
		(
		au_convert_ctx,
		out_channel_layout,
		out_sample_fmt,
		out_sample_rate,
		in_channel_layout,
		pCodecCtx->sample_fmt,
		pCodecCtx->sample_rate,
		0,
		NULL
		);

	swr_init(au_convert_ctx);
}

AudioContext::AudioContext()
{

}

AudioContext::~AudioContext()
{
	if (pCodecCtx)
	{
		avcodec_free_context(&pCodecCtx);
	}
}