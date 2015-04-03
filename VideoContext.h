#pragma once


class VideoContext
{
public:
	VideoContext();
	VideoContext(AVCodecContext* pCodecCtx);
	~VideoContext();

	AVCodecContext*	 pCodecCtx;
	AVCodec*		 pCodec;
	SwsContext*		 pToYUVConvertCtx;
	int width;
	int height;

	int stream_index;
};

