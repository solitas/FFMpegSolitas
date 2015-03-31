#include "stdafx.h"

int allocateMediaContext(MediaContext* pContext, const char *filename)
{
	pContext->pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pContext->pFormatCtx, filename, NULL, NULL) != 0)
	{
		fprintf(stderr, "Could not open input stream!");
		return -1;
	}

	if (avformat_find_stream_info(pContext->pFormatCtx, NULL) < 0)
	{
		fprintf(stderr, "Could not open input stream!");
		return -2;
	}

	int number_of_streams = pContext->pFormatCtx->nb_streams;

	for (int i = 0; i < number_of_streams; i++)
	{
		if (pContext->pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			pContext->video_stream_index = i;
			pContext->pVideoCodecCtx = pContext->pFormatCtx->streams[i]->codec;
			pContext->pVideoCodec = avcodec_find_decoder(pContext->pVideoCodecCtx->codec_id);
		}
		if (pContext->pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			pContext->audio_stream_index = i;
			pContext->pAudioCodecCtx = pContext->pFormatCtx->streams[i]->codec;
			pContext->pAudioCodec = avcodec_find_decoder(pContext->pAudioCodecCtx->codec_id);
		}
	}

	if (pContext->pVideoCodec == NULL)
	{
		fprintf(stderr, "Could not find codec!");
		return -3;
	}
	if (pContext->pAudioCodec == NULL)
	{
		fprintf(stderr, "Could not find codec!");
		return -4;
	}
	if (avcodec_open2(pContext->pVideoCodecCtx, pContext->pVideoCodec, NULL) < 0)
	{
		fprintf(stderr, "Could not open codec!");
		return -5;
	}
	if (avcodec_open2(pContext->pAudioCodecCtx, pContext->pAudioCodec, NULL) < 0)
	{
		fprintf(stderr, "Could not open codec!");
		return -6;
	}
	return 0;
}

int fillYUVPicture(AVFrame* frame, int width, int height)
{
	uint8_t* out_buffer = (uint8_t*)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, width, height));
	avpicture_fill((AVPicture*)frame, out_buffer, PIX_FMT_YUV420P, width, height);
	return 0;
}

void decode_video(MediaContext* context, ConcurrentQueue<AVPacket>* queue)
{
	AVCodecContext* pCodecCtx = context->pVideoCodecCtx;
	int got_picture;
	int width = pCodecCtx->width;
	int height = pCodecCtx->height;

	SDL_Window* window = NULL;
	SDL_Texture* texture = NULL;
	SDL_Rect rect;
	SDL_Event evt;

	window = SDL_CreateWindow("Pause Space to Exit",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height,
		SDL_WINDOW_OPENGL | SDL_WindowFlags::SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

	texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_IYUV,
		SDL_TEXTUREACCESS_STREAMING,
		width,
		height);

	rect.x = 0;
	rect.y = 0;
	rect.w = width;
	rect.h = height;
	// video frame 
	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pYuvFrame = av_frame_alloc();
	AVFrame* pAudioFrame = av_frame_alloc();

	fillYUVPicture(pYuvFrame, width, height);

	// bgr-> yuv convert context
	SwsContext* pToYUVConvertCtx = sws_getContext(
		width,
		height,
		pCodecCtx->pix_fmt,
		width,
		height,
		PIX_FMT_YUV420P,
		SWS_BICUBIC,
		NULL,
		NULL,
		NULL);

	AVPacket packet;
	int count = 0;

	while (true)
	{
		queue->pop(packet);

		int ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, &packet);

		if (ret < 0)
		{
			fprintf(stderr, "decode error");
			exit(1);
		}

		if (got_picture)
		{
			sws_scale(pToYUVConvertCtx,
				(const uint8_t* const*)pFrame->data,
				pFrame->linesize,
				0,
				height,
				pYuvFrame->data,
				pYuvFrame->linesize);

			SDL_UpdateTexture(texture, &rect, pYuvFrame->data[0], pYuvFrame->linesize[0]);
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, &rect, &rect);
			SDL_RenderPresent(renderer);
		}
		av_free_packet(&packet);

		SDL_PollEvent(&evt);

		switch (evt.key.keysym.sym)
		{
		case SDLK_SPACE: SDL_Quit();
			exit(1);
			break;
		default:
			break;
		}
	}
}

void decode_audio(AudioContext* context, ConcurrentQueue<AVPacket>* queue)
{
	int got_picture;
	int ret;

	AVCodecContext*		pCodecCtx = context->pCodecCtx;
	AVFrame*			pFrame;
	SwrContext*			au_convert_ctx;

	//Swr
	au_convert_ctx = swr_alloc();
	au_convert_ctx = swr_alloc_set_opts
		(
		au_convert_ctx,
		context->out_channel_layout,
		context->out_sample_fmt,
		context->out_sample_rate,
		context->in_channel_layout,
		pCodecCtx->sample_fmt,
		pCodecCtx->sample_rate,
		0,
		NULL
		);

	swr_init(au_convert_ctx);

	pFrame = av_frame_alloc();

	AVPacket packet;
	int count = 0;
	while (true)
	{
		queue->pop(packet);
		ret = avcodec_decode_audio4(pCodecCtx, pFrame, &got_picture, &packet);

		if (ret < 0)
		{
			printf("Error in decoding audio frame.\n");
			exit(1);
		}

		if (got_picture)
		{
			swr_convert
				(
				au_convert_ctx,
				&context->out_buffer,
				MAX_AUDIO_FRAME_SIZE,
				(const uint8_t **)pFrame->data,
				pFrame->nb_samples
				);

			//if (wanted_spec.samples != pFrame->nb_samples)
			//{
			//	SDL_CloseAudio();
			//	context->out_nb_samples = pFrame->nb_samples;
			//	context->out_buffer_size
			//		= av_samples_get_buffer_size(
			//		NULL,
			//		context->out_channels,
			//		context->out_nb_samples,
			//		context->out_sample_fmt,
			//		1);
			//	wanted_spec.samples = context->out_nb_samples;
			//	SDL_OpenAudio(&wanted_spec, NULL);
			//}
		}

		//Set audio buffer (PCM data)
		audio_chunk = (Uint8 *)context->out_buffer;
		//Audio buffer length
		audio_len = context->out_buffer_size;

		audio_pos = audio_chunk;
		//Play
		SDL_PauseAudio(0);
		while (audio_len > 0)//Wait until finish
			SDL_Delay(1);

		av_free_packet(&packet);
	}
}

void play(MediaContext* context)
{
	AVFormatContext* pFormatCtx = context->pFormatCtx;
	AVPacket packet;
	int count = 0;
	SDL_Event evt;

	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		if (context->video_stream_index == packet.stream_index)
		{
			video_queue->push(packet);
			SDL_Delay(33);
		}
		else if (context->audio_stream_index == packet.stream_index)
		{
			audio_queue->push(packet);
		}
		else
		{
			av_free_packet(&packet);
		}
	}
}

int main(int argc, char* argv[])
{
	const char* filename = "D:/Video/[JTBC] ¸¶³à»ç³É.E78.150206.HDTV.H264.720p-WITH.mp4";

	SDL_AudioSpec		wanted_spec;
	AudioContext*		pAudioCtx;

	audio_queue = new ConcurrentQueue<AVPacket>();
	video_queue = new ConcurrentQueue<AVPacket>();
	MediaContext* pContext = new MediaContext();

	av_register_all();

	//SDL Init
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}

	//std::thread video_thread(decode_video, pContext, video_queue);
	if (allocateMediaContext(pContext, filename) < 0)
	{
		exit(1);
	}

	pAudioCtx = new AudioContext(pContext->pAudioCodecCtx);

	//SDL_AudioSpec
	wanted_spec.freq = pAudioCtx->out_sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = pAudioCtx->out_channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = pAudioCtx->out_nb_samples;
	wanted_spec.callback = fill_audio;
	wanted_spec.userdata = pAudioCtx->pCodecCtx;

	if (SDL_OpenAudio(&wanted_spec, NULL) < 0)
	{
		fprintf(stderr, "can't open audio.\n %s", SDL_GetError());
		exit(1);
	}

	std::thread audio_thread(decode_audio, pAudioCtx, audio_queue);
	std::thread video_thread(decode_video, pContext, video_queue);
	// 
	play(pContext);

	avformat_close_input(&pContext->pFormatCtx);
	// 
	audio_thread.join();
	delete pContext;

	return 0;
}

