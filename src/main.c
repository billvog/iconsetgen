#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "image.h"

#define break_error(hadError) hadError = true; break

int main(int argc, char *argv[]) {
	if (argc < 3) {
		printf("usage: %s <icon file> <output name>\n", argv[0]);
		return 1;
	}

	const char *icon_file = argv[1];
	const char *output_name = argv[2];

	// Open image
	FrameData *icon_frame = open_image(icon_file);
	if (!icon_file) {
		exit(1);
	}

	// Process it
	float scale_factor = 1;
	FrameData *temp_frame = malloc(sizeof(icon_frame));

	bool hadError = false;
	for (int i = 0; i < 4; i++) {
		*temp_frame = *icon_frame;

		int newWidth = (int)((float) temp_frame->frame->width * scale_factor);
		int newHeight = (int)((float) temp_frame->frame->height * scale_factor);
		printf("Generating %dx%d...\n", newWidth, newHeight);

		bool ok = scale_image(temp_frame, scale_factor);
		if (!ok) {
			break_error(hadError);
		}

		char *output_filename = malloc(strlen(output_name) + 32);
		sprintf(output_filename, "%s/%dx%d_icon.png", output_name, newWidth, newHeight);
		ok = save_image(temp_frame, output_filename);
		if (!ok) {
			break_error(hadError);
		}

		// update scale factor
		scale_factor /= 2;
	}

	free(temp_frame);
	close_image(icon_frame);

	if (hadError) {
		return 1;
	}

	return 0;
}