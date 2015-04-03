#include "stdafx.h"
#include "SDLPlay.h"

int main(int argc, char* argv[])
{
	const char* filename = "D:/Data/Video/¸¶³à»ç³É/[JTBC] ¸¶³à»ç³É.E78.150206.HDTV.H264.720p-WITH.mp4";

	av_register_all();

	//SDL Init
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}

	SDLPlay * sdlPlayer = new SDLPlay();

	sdlPlayer->openVideoFile(filename);

	sdlPlayer->play();

	return 0;
}
