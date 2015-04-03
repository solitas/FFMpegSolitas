#pragma once
class AudioContext
{
public:
	AudioContext();
	AudioContext(AVCodecContext *pCodecCtx);
	~AudioContext();

	AVCodecContext*	 pCodecCtx;
	AVCodec*		 pCodec;
	SwrContext*		 au_convert_ctx;

	int out_nb_samples;
	int out_sample_rate;
	int out_channels;
	int out_buffer_size;

	uint64_t out_channel_layout			= AV_CH_LAYOUT_STEREO;
	uint64_t in_channel_layout;
	AVSampleFormat out_sample_fmt		= AV_SAMPLE_FMT_S16;

	uint8_t				*out_buffer;		// pcm data

	int stream_index;
};
