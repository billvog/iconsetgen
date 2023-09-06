#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
	struct AVCodecContext *codecCtx;
	struct AVFrame *frame;
} FrameData;

typedef struct {
	int width, height;
} Size;

FrameData* open_image(const char *image_file);
void close_image(FrameData *frame_data);

bool scale_image(FrameData *frame_data, Size new_size);
bool save_image(FrameData *frame_data, const char *output);

#endif