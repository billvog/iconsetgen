#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#include "image.h"

#define break_error(hadError) hadError = true; break

int main(int argc, char *argv[]) {
	if (argc < 3) {
		printf("Usage: %s <icon file> <output>\n", argv[0]);
		return 1;
	}

	const char *icon_file = argv[1];
	const char *output_name = argv[2];

	// Create output folder
	int ok = mkdir(output_name, 0700);
	if (ok != 0) {
		printf("E: Failed to create output folder: %s\n", strerror(errno));
		return 1;
	}

	// Open image
	FrameData *icon_frame = open_image(icon_file);
	if (!icon_file) {
		return 1;
	}

	// Process it
	float scale_factor = 1;
	FrameData *temp_frame = malloc(sizeof(icon_frame));

	bool hadError = false;
	for (int i = 0; i < 6; i++) {
		*temp_frame = *icon_frame;

		int newWidth = (int)((float) temp_frame->frame->width * scale_factor);
		int newHeight = (int)((float) temp_frame->frame->height * scale_factor);
		printf("Generating %dx%d...\n", newWidth, newHeight);

		bool ok = scale_image(temp_frame, scale_factor);
		if (!ok) {
			break_error(hadError);
		}

		char *output_filename = malloc(strlen(output_name) + 32);
		sprintf(output_filename, "%s/icon_%dx%d.png", output_name, newWidth, newHeight);
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
	else {
		printf("The iconset has been created at %s\n", output_name);
	}

	return 0;
}