#pragma once

class SDLPlay
{
public:
	SDLPlay();
	~SDLPlay();

	void openVideoFile(const char* filename);
	void close();
	void play();
	void stop();

	const char* filename;
	AVFormatContext* pFormatCtx;
	VideoContext* videoCtx;
	AudioContext* audioCtx;

	SDL_AudioSpec wanted_spec;
	SDL_Window* window;
	SDL_Texture* texture;
	SDL_Rect rect;
	SDL_Event evt;
	SDL_Renderer* renderer;

private:
	ConcurrentQueue<AVPacket> *audio_queue;
	ConcurrentQueue<AVPacket> *video_queue;

	INITResult init_format_context();
	INITResult init_sdl_audio();
	INITResult init_sdl_window();


	void fill_yuv_picture(AVFrame* frame, int width, int height);

	void decode_frame();

	void decode_video();
	void decode_audio();
};

