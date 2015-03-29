#include <stdio.h>

extern "C" 
{
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

//#pragma comment( lib, "avcodec.lib")
//#pragma comment( lib, "avformat.lib")
//#pragma comment( lib, "avutil.lib")

void main()
{
	av_register_all();
}