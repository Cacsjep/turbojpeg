#include <string.h>
#include <stdio.h>
#include <jpeglib.h>
#include <stdlib.h>
#include <stdbool.h>
typedef enum {
    DECOMPRESS_SUCCESS = 0,
    ERROR_READING_HEADER = -1,
    ERROR_DURING_DECOMPRESSION = -2,
    ERROR_MEMORY_ALLOCATION = -3,
    ERROR_READING_SCANLINES = -4,
    ERROR_TRAGET_RESO_NO_SET = -5,
    ERROR_RGBA_SUPPORT_MISSING = -6,
    ERROR_DECODER_NOT_INTI = -7,
} DecompressError;

// Structure to hold JPEG decoder configuration and state
typedef struct {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    unsigned char *input_buffer; // Compressed YUV data
    int input_buffer_length;           // Length of the input buffer
    unsigned char *output_buffer; // Decompressed output buffer (RGB)
    int output_buffer_length;           // Length of the input buffer
    int original_frame_width;                    // Width of the decompressed image
    int original_frame_height;                    // Width of the decompressed image
    int decodec_frame_height;                    // Width of the decompressed image
    int decodec_frame_width;                    // Width of the decompressed image
    int scaled_width;                    // Width of the decompressed image
    int scaled_height;                    // Width of the decompressed image
} jpeg_turbo_decoder;

static J_COLOR_SPACE getJCS_EXT_RGBA(void) {
#ifdef JCS_ALPHA_EXTENSIONS
	return JCS_EXT_RGBA;
#endif
  return JCS_UNKNOWN;
}

jpeg_turbo_decoder* alloc_decoder() {
    jpeg_turbo_decoder *decoder = (jpeg_turbo_decoder *)malloc(sizeof(jpeg_turbo_decoder));
    if (decoder == NULL) {
        return NULL;
    }
    decoder->cinfo.err = jpeg_std_error(&decoder->jerr);
    jpeg_create_decompress(&decoder->cinfo);
    decoder->input_buffer = NULL;
    decoder->input_buffer_length = 0;
    decoder->output_buffer = NULL;
    // The orignal frame w,h
    decoder->original_frame_height = 0;
    decoder->original_frame_width = 0;

    // The frame dims after decoding
    decoder->decodec_frame_height = 0;
    decoder->decodec_frame_width = 0;

    // the scale dims
    decoder->scaled_height = 0;
    decoder->scaled_width = 0;

    return decoder;
}

void update_input_buffer(jpeg_turbo_decoder *decoder, unsigned char *new_buffer, int new_size) {
    if (decoder != NULL) {
        free(decoder->input_buffer); // Free the old buffer if it exists
        decoder->input_buffer = (unsigned char *)malloc(new_size);
        if (decoder->input_buffer != NULL) {
            memcpy(decoder->input_buffer, new_buffer, new_size);
            decoder->input_buffer_length = new_size;
        }
    }
}

int set_original_frame_dims(jpeg_turbo_decoder *decoder, int *width, int *height) {
    if (decoder == NULL) {
        return ERROR_DECODER_NOT_INTI;
    }
    jpeg_create_decompress(&decoder->cinfo);
    jpeg_mem_src(&decoder->cinfo, decoder->input_buffer, decoder->input_buffer_length);
    if (jpeg_read_header(&decoder->cinfo, TRUE) != JPEG_HEADER_OK) {
        jpeg_destroy_decompress(&decoder->cinfo);
        return ERROR_READING_HEADER;
    }
    decoder->original_frame_width = decoder->cinfo.image_width;
    decoder->original_frame_height = decoder->cinfo.image_height;
    jpeg_finish_decompress(&decoder->cinfo);
    return DECOMPRESS_SUCCESS;
}

int decompress_jpeg_to_rgb(jpeg_turbo_decoder *decoder) {

    jpeg_create_decompress(&decoder->cinfo);
    jpeg_mem_src(&decoder->cinfo, decoder->input_buffer, decoder->input_buffer_length);

    if (jpeg_read_header(&decoder->cinfo, TRUE) != JPEG_HEADER_OK) {
        return ERROR_READING_HEADER;
    }

    decoder->cinfo.do_block_smoothing = false;
    decoder->cinfo.do_fancy_upsampling = false;
    decoder->cinfo.dct_method = JDCT_IFAST;

    // Set the orignal frame dims
    decoder->original_frame_width = decoder->cinfo.output_width;
    decoder->original_frame_height = decoder->cinfo.output_height;

    // Determine the appropriate scaling factor, update it when scale w and h is set
    decoder->cinfo.scale_num = 1;
    decoder->cinfo.scale_denom = 1;

    if (decoder->scaled_height > 0 && decoder->scaled_width > 0){
        if (decoder->cinfo.image_width > decoder->scaled_width || decoder->cinfo.image_height > decoder->scaled_height) {
            while (decoder->cinfo.scale_denom < 8 && (decoder->cinfo.image_width / (decoder->cinfo.scale_denom * 2) > decoder->scaled_width || decoder->cinfo.image_height / (decoder->cinfo.scale_denom * 2) > decoder->scaled_height)) {
                decoder->cinfo.scale_denom *= 2;
            }
        }
    }

    decoder->cinfo.out_color_space = getJCS_EXT_RGBA();
    if (decoder->cinfo.out_color_space == JCS_UNKNOWN) {
        decoder->cinfo.out_color_space = JCS_RGB;
        return ERROR_RGBA_SUPPORT_MISSING;
    }

    // Start decompression with scaling
    if (!jpeg_start_decompress(&decoder->cinfo)) {
        return ERROR_DURING_DECOMPRESSION;
    }

    decoder->decodec_frame_width = decoder->cinfo.output_width;
    decoder->decodec_frame_height = decoder->cinfo.output_height;

    // Reallocate output buffer if necessary
    int required_size = decoder->decodec_frame_width * decoder->decodec_frame_height * decoder->cinfo.output_components;
    if (decoder->output_buffer == NULL || decoder->output_buffer_length != required_size) {

        // Free the old buffer if it exists
        free(decoder->output_buffer);
        decoder->output_buffer = NULL;

        // Allocate new buffer with the required size
        decoder->output_buffer = (unsigned char *)malloc(required_size);
        if (decoder->output_buffer == NULL) {
            return ERROR_MEMORY_ALLOCATION;
        }
        decoder->output_buffer_length = required_size;
    }

    while (decoder->cinfo.output_scanline < decoder->cinfo.output_height) {
        unsigned char *row_address[1] = {decoder->output_buffer + (decoder->cinfo.output_scanline * (decoder->decodec_frame_width * decoder->cinfo.output_components))};
        if (jpeg_read_scanlines(&decoder->cinfo, row_address, 1) != 1) {
            return ERROR_READING_SCANLINES;
        }
    }
    
    jpeg_finish_decompress(&decoder->cinfo);
    return DECOMPRESS_SUCCESS;
}

void update_scale_resolution(jpeg_turbo_decoder *decoder, int w, int h){
    decoder->scaled_height = h;
    decoder->scaled_width = w;
}


void free_decoder(jpeg_turbo_decoder *decoder) {
    if (decoder != NULL) {
        if (decoder->output_buffer != NULL) {
            free(decoder->output_buffer);
        }
        jpeg_destroy_decompress(&decoder->cinfo);
        free(decoder);
    }
}
