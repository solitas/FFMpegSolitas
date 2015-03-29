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
}

#define MAX_AUDIO_FRAME_SIZE 192000 
#define INBUF_SIZE 4096

typedef struct MediaContext {

	AVFormatContext * pFormatCtx;

	AVCodecContext* pVideoCodecCtx;

	AVCodecContext* pAudioCodecCtx;

	AVCodec* pVideoCodec;

	AVCodec* pAudioCodec;
	
	int video_stream_index;

	int audio_stream_index;

} MediaContext;

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
			break;
		}
	}

	pContext->pVideoCodecCtx = pContext->pFormatCtx->streams[pContext->video_stream_index]->codec;

	pContext->pVideoCodec = avcodec_find_decoder(pContext->pVideoCodecCtx->codec_id);

	if (pContext->pVideoCodec == NULL)
	{
		fprintf(stderr, "Could not find codec!");
		return -3;
	}

	if (avcodec_open2(pContext->pVideoCodecCtx, pContext->pVideoCodec, NULL) < 0)
	{
		fprintf(stderr, "Could not open codec!");
		return -4;
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
	const char* filename = "D:/Video/[JTBC] ¸¶³à»ç³É.E78.150206.HDTV.H264.720p-WITH.mp";
	int width; 
	int height;
	int ret = 0;
	int got_picture;

	av_register_all();

	MediaContext* pContext = new MediaContext();

	if (allocateMediaContext(pContext, filename) < 0)
	{
		exit(1);
	}

	width = pContext->pVideoCodecCtx->width;
	height = pContext->pVideoCodecCtx->height;

	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pYuvFrame = av_frame_alloc();

	fillYUVPicture(pYuvFrame, width, height);

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

	// End SDL Init


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

			SDL_Delay(30);
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

	// Dispose 

	SDL_DestroyTexture(texture);
	av_free(pFrame);
	av_free(pYuvFrame);
	avcodec_close(pContext->pVideoCodecCtx);
	avformat_close_input(&pContext->pFormatCtx);

	delete pContext;

	return 0;
}