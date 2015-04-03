#include "stdafx.h"
#include "SDLPlay.h"
//double synchronize_video(MediaContext *is, AVFrame *src_frame, double pts) {
//
//	double frame_delay;
//
//	if (pts != 0) {
//		/* if we have pts, set video clock to it */
//		is->video_clock = pts;
//	}
//	else {
//		/* if we aren't given a pts, set it to the clock */
//		pts = is->video_clock;
//	}
//	/* update the video clock */
//	frame_delay = av_q2d(is->pVideoCodecCtx->time_base);
//	/* if we are repeating a frame, adjust clock accordingly */
//	frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
//	is->video_clock += frame_delay;
//	return pts;
//}
//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
static  Uint8  *audio_chunk;
static  Uint32  audio_len;
static  Uint8  *audio_pos;

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

SDLPlay::SDLPlay()
{
}


SDLPlay::~SDLPlay()
{
}

void SDLPlay::openVideoFile(const char* filename)
{
	this->filename = filename;

	INITResult result = init_format_context();

	if (result == INITResult::OK)
	{
		result = init_sdl_audio();

		if (result != INITResult::OK)
		{
			exit(1);
		}

		result = init_sdl_window();

		if (result != INITResult::OK)
		{
			exit(1);
		}

		audio_queue = new ConcurrentQueue<AVPacket>();
		video_queue = new ConcurrentQueue<AVPacket>();
	}
	else
	{
		exit(1);
	}
}

void SDLPlay::close()
{

}

void SDLPlay::play()
{
	// create video, audio thread
	std::thread video_decode_thread(&SDLPlay::decode_audio, this);
	std::thread audio_decode_thread(&SDLPlay::decode_video, this);

	decode_frame();
}

void SDLPlay::stop()
{
	// stop thread
}

void SDLPlay::fill_yuv_picture(AVFrame* frame, int width, int height)
{
	uint8_t* out_buffer = (uint8_t*)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, width, height));
	avpicture_fill((AVPicture*)frame, out_buffer, PIX_FMT_YUV420P, width, height);
}

INITResult SDLPlay::init_format_context()
{
	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0)
	{
		fprintf(stderr, "Could not open input stream!");
		return INITResult::FAIL;
	}

	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		fprintf(stderr, "Could not open input stream!");
		return INITResult::FAIL;
	}

	int number_of_streams = pFormatCtx->nb_streams;

	for (int i = 0; i < number_of_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoCtx = new VideoContext(pFormatCtx->streams[i]->codec);
			videoCtx->stream_index = i;
			videoCtx->pCodec = avcodec_find_decoder(videoCtx->pCodecCtx->codec_id);
		}
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioCtx = new AudioContext(pFormatCtx->streams[i]->codec);
			audioCtx->stream_index = i;
			audioCtx->pCodec = avcodec_find_decoder(audioCtx->pCodecCtx->codec_id);
		}
	}

	if (videoCtx->pCodec == NULL)
	{
		fprintf(stderr, "Could not find codec!");
		return INITResult::FAIL;
	}
	if (audioCtx->pCodec == NULL)
	{
		fprintf(stderr, "Could not find codec!");
		return INITResult::FAIL;
	}
	if (avcodec_open2(videoCtx->pCodecCtx, videoCtx->pCodec, NULL) < 0)
	{
		fprintf(stderr, "Could not open video codec!");
		return INITResult::FAIL;
	}
	if (avcodec_open2(audioCtx->pCodecCtx, audioCtx->pCodec, NULL) < 0)
	{
		fprintf(stderr, "Could not open audio codec!");
		return INITResult::FAIL;
	}
	return INITResult::OK;
}

INITResult SDLPlay::init_sdl_audio()
{
	//SDL_AudioSpec
	wanted_spec.freq = audioCtx->out_sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = audioCtx->out_channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = audioCtx->out_nb_samples;
	wanted_spec.callback = fill_audio;
	wanted_spec.userdata = audioCtx->pCodecCtx;

	if (SDL_OpenAudio(&wanted_spec, NULL) < 0)
	{
		fprintf(stderr, "can't open audio.\n %s", SDL_GetError());
		return INITResult::FAIL;
	}
	return INITResult::OK;
}

INITResult SDLPlay::init_sdl_window()
{
	window = NULL;
	texture = NULL;

	window = SDL_CreateWindow("Pause Space to Exit",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		videoCtx->width,
		videoCtx->height,
		SDL_WINDOW_OPENGL | SDL_WindowFlags::SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS);

	renderer = SDL_CreateRenderer(window, -1, 0);

	texture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_IYUV,
		SDL_TEXTUREACCESS_STREAMING,
		videoCtx->width,
		videoCtx->height);

	rect.x = 0;
	rect.y = 0;
	rect.w = videoCtx->width;
	rect.h = videoCtx->height;

	return INITResult::OK;
}

void SDLPlay::decode_frame()
{
	AVPacket packet;
	int count = 0;

	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		count++;

		if (videoCtx->stream_index == packet.stream_index)
		{
			video_queue->push(packet);
			//std::cout << "PTS : " << packet.pts << " DTS " << packet.dts << std::endl;
			SDL_Delay(33);
		}
		else if (audioCtx->stream_index == packet.stream_index)
		{
			audio_queue->push(packet);
		}
		else
		{
			av_free_packet(&packet);
		}
	}
}

void SDLPlay::decode_video()
{
	// video frame 
	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pYuvFrame = av_frame_alloc();
	AVFrame* pAudioFrame = av_frame_alloc();

	fill_yuv_picture(pYuvFrame, videoCtx->width, videoCtx->height);

	AVPacket packet;
	int count = 0;
	double pts;
	int got_picture;

	while (true)
	{
		video_queue->pop(packet);
		pts = 0;

		int ret = avcodec_decode_video2(videoCtx->pCodecCtx, pFrame, &got_picture, &packet);

		if (ret < 0)
		{
			fprintf(stderr, "decode error");
			exit(1);
		}

		if ((pts = av_frame_get_best_effort_timestamp(pFrame)) == AV_NOPTS_VALUE)
		{
			pts = 0;
		}
		pts *= av_q2d(videoCtx->pCodecCtx->time_base);

		if (got_picture)
		{
			//pts = synchronize_video(context, pFrame, pts);
			std::cout << "video pts : " << pts << std::endl;
			sws_scale(videoCtx->pToYUVConvertCtx,
				(const uint8_t* const*)pFrame->data,
				pFrame->linesize,
				0,
				videoCtx->height,
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

void SDLPlay::decode_audio()
{
	int got_picture;
	int ret;
	AVFrame*			pFrame;
	pFrame = av_frame_alloc();

	AVPacket packet;
	int count = 0;
	while (true)
	{
		audio_queue->pop(packet);
		ret = avcodec_decode_audio4(audioCtx->pCodecCtx, pFrame, &got_picture, &packet);

		if (ret < 0)
		{
			printf("Error in decoding audio frame.\n");
			exit(1);
		}

		if (got_picture)
		{
			swr_convert
				(
				audioCtx->au_convert_ctx,
				&audioCtx->out_buffer,
				MAX_AUDIO_FRAME_SIZE,
				(const uint8_t **)pFrame->data,
				pFrame->nb_samples
				);

			if (wanted_spec.samples != pFrame->nb_samples)
			{
				SDL_CloseAudio();
				audioCtx->out_nb_samples = pFrame->nb_samples;
				audioCtx->out_buffer_size
					= av_samples_get_buffer_size(
					NULL,
					audioCtx->out_channels,
					audioCtx->out_nb_samples,
					audioCtx->out_sample_fmt,
					1);
				wanted_spec.samples = audioCtx->out_nb_samples;
				SDL_OpenAudio(&wanted_spec, NULL);
			}
		}

		//Set audio buffer (PCM data)
		audio_chunk = (Uint8 *)audioCtx->out_buffer;
		//Audio buffer length
		audio_len = audioCtx->out_buffer_size;

		audio_pos = audio_chunk;
		//Play
		SDL_PauseAudio(0);
		while (audio_len > 0)//Wait until finish
			SDL_Delay(1);

		av_free_packet(&packet);
	}
}
