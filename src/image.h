#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

typedef struct {
	AVCodecContext *codecCtx;
	AVFrame *frame;
} FrameData;

FrameData* open_image(const char *image_file);
void close_image(FrameData *frame_data);

bool scale_image(FrameData *frame_data, float scale_factor);
bool save_image(FrameData *frame_data, const char *output);

#endif