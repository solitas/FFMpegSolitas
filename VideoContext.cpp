#include "stdafx.h"
#include "VideoContext.h"


VideoContext::VideoContext()
{

}

VideoContext::VideoContext(AVCodecContext* pCodecCtx)
{
	this->pCodecCtx = pCodecCtx;
	width = pCodecCtx->width;
	height = pCodecCtx->height;
	pToYUVConvertCtx = sws_getContext(
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
}

