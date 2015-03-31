#pragma once
#include "stdafx.h"

class VideoContext
{
public:
	VideoContext(AVCodecContext* pCodecCtx);
	~VideoContext();

	AVCodecContext*	 pCodecCtx;
	AVCodec*		 pCodec;

};

VideoContext::VideoContext(AVCodecContext* pCodecCtx)
{
	this->pCodecCtx = pCodecCtx;
}
