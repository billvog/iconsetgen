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

FrameData* open_image(const char *image_file) {
	AVFormatContext *formatCtx = avformat_alloc_context();

	if (avformat_open_input(&formatCtx, image_file, NULL, NULL) != 0) {
		printf("E: Can't open image file '%s'\n", image_file);
		return NULL;
	}

	// av_dump_format(formatCtx, 0, image_file, false);

	AVCodecParameters *codecParams = formatCtx->streams[0]->codecpar;

	AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);
	if (!codec) {
		printf("E: Codec not found\n");
		return NULL;
    }

	AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
	
	if (avcodec_parameters_to_context(codecCtx, codecParams) < 0) {
		printf("E: Failed converting AVCodecParameters to AVCodecContext\n");
		return NULL;
	}

	codecCtx->width = 1024;
	codecCtx->height = 1024;
	codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	codecCtx->time_base = (AVRational) {1, 25};
	
	if (avcodec_open2(codecCtx, codec, NULL) < 0) {
		printf("E: Could not open codec\n");
		return NULL;	
	}

	AVFrame *frame = av_frame_alloc();
	if (!frame) {
		printf("E: Could not allocate memory for AVFrame\n");
		return NULL;
	}
	
	int frame_finished;
	int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, codecCtx->width, codecCtx->height, 0);

	uint8_t *src_buf[4];
    int src_linesize[4];
	uint8_t *buffer = av_malloc(num_bytes * sizeof(uint8_t));
	av_image_fill_arrays(src_buf, src_linesize, buffer, AV_PIX_FMT_YUV420P, codecCtx->width, codecCtx->height, 0);

	AVPacket *packet = av_packet_alloc();

	int response;
	while (av_read_frame(formatCtx, packet) >= 0) {
		if (packet->stream_index != 0)
			continue;

		response = avcodec_send_packet(codecCtx, packet);
		if (response < 0) {
			printf("Failed to decode packet: %s\n", av_err2str(response));
            return NULL;
		}

		response = avcodec_receive_frame(codecCtx, frame);
		if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            continue;
        } else if (response < 0) {
            printf("Failed to decode packet: %s\n", av_err2str(response));
            return NULL;
        }

		break;
	}
	
	FrameData *frame_data = malloc(sizeof(FrameData));
	frame_data->codecCtx = codecCtx;
	frame_data->frame = frame;

	return frame_data;
}

bool save_image(FrameData *frame_data, const char *output) {
	AVCodec *imageCodec = avcodec_find_encoder(frame_data->codecCtx->codec_id);
	if (imageCodec == NULL) {
		printf("E: Cannot find encoder for PNG\n");
		return false;
	}

	AVCodecContext *imageCodecCtx = avcodec_alloc_context3(imageCodec);
	if  (imageCodecCtx == NULL) {
		printf("E: Cannot allocate memory for AVCodecContext\n");
		return false;
	}

	imageCodecCtx->pix_fmt = frame_data->codecCtx->pix_fmt;
	imageCodecCtx->width = frame_data->frame->width;
	imageCodecCtx->height = frame_data->frame->height;
	imageCodecCtx->time_base = frame_data->codecCtx->time_base;

	int err = avcodec_open2(imageCodecCtx, imageCodec, NULL);
	if (err < 0) {
		printf("E: Cannot open codec: %s\n", av_err2str(err));
		return false;
	}

	AVPacket *packet = av_packet_alloc();
	if (avcodec_send_frame(imageCodecCtx, frame_data->frame) < 0) {
		printf("E: Cannot encode video frame\n");
		return false;
	}

	FILE *fp = fopen(output, "wb");
	if (fp == NULL) {
		printf("E: Cannot create output image file: %s", strerror(errno));
		return false;
	}

	int response;
	do {
		response = avcodec_receive_packet(imageCodecCtx, packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
            return false;
        else if (response < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }

        printf("Write packet (size=%5d)\n", packet->size);
        fwrite(packet->data, packet->size, 1, fp);

		break;
	} while (response >= 0);

	fwrite(packet->data, packet->size, 1, fp);
	fclose(fp);

	av_packet_unref(packet);
	avcodec_close(imageCodecCtx);

	return true;
}

bool scale_image(FrameData *frame_data, float scale_factor) {
	AVFrame *originalFrame = frame_data->frame;
	int originalWidth = originalFrame->width;
	int originalHeight = originalFrame->height;

	int newWidth = (int)((float) originalWidth * scale_factor);
	int newHeight = (int)((float) originalHeight * scale_factor);

	AVFrame* newFrame = av_frame_alloc();
	if (!newFrame) {
		printf("E: Could not allocate memory for new frame\n");
		return false;
	}

	newFrame->width = newWidth;
	newFrame->height = newHeight;
	newFrame->format = AV_PIX_FMT_RGB0;
	int res = av_frame_get_buffer(newFrame, 32);
	if (res != 0) {
		printf("E: Could not get frame buffer: %s\n", av_err2str(res));
		return false;
	}

	printf("New image dimensions: %dx%d\n", newWidth, newHeight);

	struct SwsContext *swsCtx = sws_getContext(originalWidth, originalHeight, originalFrame->format,
											   newWidth, newHeight, AV_PIX_FMT_RGB0,
											   SWS_BICUBIC, NULL, NULL, NULL);
	if (!swsCtx) {
		printf("E: Could not get sws context\n");
		return false;
	}

	sws_scale(swsCtx, (const uint8_t* const*)&originalFrame->data, 
			originalFrame->linesize, 0, originalHeight, newFrame->data, newFrame->linesize);

	frame_data->frame = newFrame;

	return true;
}

int main(int argc, char *argv[]) {
	if (argc < 3) {
		printf("usage: %s <icon file> <output name>\n", argv[0]);
		return 1;
	}

	const char *icon_file = argv[1];
	const char *output_name = argv[2];

	printf("Reading image...\n");
	
	FrameData *icon_frame = open_image(icon_file);
	if (!icon_file) {
		exit(1);
	}

	printf("Image read!\n");
	printf("Scalling image down...\n");

	bool scale_ok = scale_image(icon_frame, 0.5);
	if (!scale_ok) {
		exit(1);
	}

	printf("Image scalled down!\n");
	printf("Writing image...\n");

	bool save_ok = save_image(icon_frame, output_name);
	if (!save_ok) {
		exit(1);
	}

	printf("Image has been written!\n");

	return 0;
}