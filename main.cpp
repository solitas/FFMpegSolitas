#include <iostream>
#include <stdio.h>
#include <string.h>
#include <math.h>

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

//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
static  Uint8  *audio_chunk;
static  Uint32  audio_len;
static  Uint8  *audio_pos;
typedef struct MediaContext {

	AVFormatContext * pFormatCtx;

	AVCodecContext* pVideoCodecCtx;

	AVCodecContext* pAudioCodecCtx;

	AVCodec* pVideoCodec;

	AVCodec* pAudioCodec;

	int video_stream_index;

	int audio_stream_index;

} MediaContext;

AVCodecContext * CreateContext()
{
	av_register_all();

	AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_AAC);

	AVCodecContext *avCtx = avcodec_alloc_context3(codec);

	return avCtx;
}

int32_t DecodeBuffer
(
std::ostream   & output,
uint8_t        * pInput,
uint32_t         cbInputSize,
AVCodecContext * pAVContext
)

{
	int32_t cbDecoded = 0;

	//the input buffer
	AVPacket avPacket;
	av_init_packet(&avPacket);

	avPacket.size = cbInputSize; //input buffer size
	avPacket.data = pInput; // the input bufferra

	AVFrame * pDecodedFrame = av_frame_alloc();

	int nGotFrame = 0;

	cbDecoded = avcodec_decode_audio4(pAVContext,
		pDecodedFrame,
		&nGotFrame,
		&avPacket);

	int data_size = av_samples_get_buffer_size(NULL,
		pAVContext->channels,
		pDecodedFrame->nb_samples,
		pAVContext->sample_fmt,
		1);

	output.write((const char*)pDecodedFrame->data[0], data_size);


	av_frame_free(&pDecodedFrame);

	return cbDecoded;
}


uint8_t * ReceiveBuffer(uint32_t * cbBufferSize)
{
	// TODO implement

	return NULL;
}
void fill_audio(void *udata, Uint8 *stream, int len){
	//SDL 2.0
	SDL_memset(stream, 0, len);
	if (audio_len == 0)		/*  Only  play  if  we  have  data  left  */
		return;
	len = (len > audio_len ? audio_len : len);	/*  Mix  as  much  data  as  possible  */

	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;
}

/*
* Video decoding example
*/

static int allocateMediaContext(MediaContext* pContext, const char *filename)
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
}

static int fillYUVPicture(AVFrame* frame, int width, int height)
{
	uint8_t* out_buffer = (uint8_t*)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, width, height));
	avpicture_fill((AVPicture*)frame, out_buffer, PIX_FMT_YUV420P, width, height);
	return 0;
}

int main(int argc, char* argv[])
{
	const char* filename = "D:/Video/[JTBC] ¸¶³à»ç³É.E78.150206.HDTV.H264.720p-WITH.mp4";
	int width;
	int height;
	int ret = 0;
	int got_picture;
	uint8_t			*out_buffer;
	SDL_AudioSpec wanted_spec;
	struct SwrContext *au_convert_ctx;
	av_register_all();

	MediaContext* pContext = new MediaContext();

	if (allocateMediaContext(pContext, filename) < 0)
	{
		exit(1);
	}

	width = pContext->pVideoCodecCtx->width;
	height = pContext->pVideoCodecCtx->height;

	// video frame 
	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pYuvFrame = av_frame_alloc();
	AVFrame* pAudioFrame = av_frame_alloc();

	fillYUVPicture(pYuvFrame, width, height);

	// audio frame
	//Out Audio Param
	uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
	int out_nb_samples = 1024;
	AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	int out_sample_rate = pContext->pAudioCodecCtx->sample_rate;
	int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
	//Out Buffer Size
	int out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

	out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 3/2);

	// bgr-> yuv convert context
	SwsContext* pToYUVConvertCtx = sws_getContext(
		width,
		height,
		pContext->pVideoCodecCtx->pix_fmt,
		width,
		height,
		PIX_FMT_YUV420P,
		SWS_BICUBIC,
		NULL,
		NULL,
		NULL);

	// SDL Init

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		fprintf(stderr, "Could not init sdl");
		return 1;
	}

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
#if USE_SDL
	//Init
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	//SDL_AudioSpec
	wanted_spec.freq = out_sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = out_channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = out_nb_samples;
	wanted_spec.callback = fill_audio;
	wanted_spec.userdata = pContext->pAudioCodecCtx;

	if (SDL_OpenAudio(&wanted_spec, NULL) < 0){
		printf("can't open audio.\n");
		return -1;
	}
#endif
	// End SDL Init
	printf("Bitrate:\t %3d\n", pContext->pFormatCtx->bit_rate);
	printf("Decoder Name:\t %s\n", pContext->pAudioCodecCtx->codec->long_name);
	printf("Channels:\t %d\n", pContext->pAudioCodecCtx->channels);
	printf("Sample per Second\t %d \n", pContext->pAudioCodecCtx->sample_rate);
	//FIX:Some Codec's Context Information is missing
	int64_t in_channel_layout;
	in_channel_layout = av_get_default_channel_layout(pContext->pAudioCodecCtx->channels);
	//Swr

	au_convert_ctx = swr_alloc();
	au_convert_ctx = swr_alloc_set_opts(au_convert_ctx, out_channel_layout, out_sample_fmt, out_sample_rate,
		in_channel_layout, pContext->pAudioCodecCtx->sample_fmt, pContext->pAudioCodecCtx->sample_rate, 0, NULL);
	swr_init(au_convert_ctx);
	// read frame 
	AVPacket packet;

	while (av_read_frame(pContext->pFormatCtx, &packet) >= 0)
	{
		if (packet.stream_index == pContext->video_stream_index)
		{

			ret = avcodec_decode_video2(pContext->pVideoCodecCtx, pFrame, &got_picture, &packet);

			if (ret < 0)
			{
				fprintf(stderr, "decode error");
				return 1;
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

			//SDL_Delay(30);
		}
		else if (packet.stream_index == pContext->audio_stream_index)
		{
			ret = avcodec_decode_audio4(pContext->pAudioCodecCtx, pAudioFrame, &got_picture, &packet);
			if (ret < 0) {
				printf("Error in decoding audio frame.\n");
				return -1;
			}
			if (got_picture)
			{
				swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)pAudioFrame->data, pAudioFrame->nb_samples);
				if (wanted_spec.samples != pAudioFrame->nb_samples){
					SDL_CloseAudio();
					out_nb_samples = pAudioFrame->nb_samples;
					out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);
					wanted_spec.samples = out_nb_samples;
					SDL_OpenAudio(&wanted_spec, NULL);
				}
			}
			//Set audio buffer (PCM data)
			audio_chunk = (Uint8 *)out_buffer;
			//Audio buffer length
			audio_len = out_buffer_size;

			audio_pos = audio_chunk;
			//Play
			SDL_PauseAudio(0);
			while (audio_len > 0)//Wait until finish
				SDL_Delay(1);
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

	// Dispose 

	SDL_DestroyTexture(texture);
	av_free(pFrame);
	av_free(pYuvFrame);
	avcodec_close(pContext->pVideoCodecCtx);
	avformat_close_input(&pContext->pFormatCtx);

	delete pContext;

	return 0;
}