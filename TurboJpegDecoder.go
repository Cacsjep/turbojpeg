package TurboJpegDecoder

/*
#cgo CFLAGS: -I.
#cgo LDFLAGS: -ljpeg
#include "TurboJpegDecoder.h"
*/
import "C"
import (
	"errors"
	"fmt"
	"image"
	"unsafe"
)

type Decoder struct {
	c            *C.jpeg_turbo_decoder
	lastScalingW int
	lastScalingH int
}

type DecompressError C.DecompressError

const (
	RGBA_COLOR_COMPONENTS       int             = 4
	DecompressSuccess           DecompressError = DecompressError(C.DECOMPRESS_SUCCESS)
	ErrorReadingHeader          DecompressError = DecompressError(C.ERROR_READING_HEADER)
	ErrorDuringDecompression    DecompressError = DecompressError(C.ERROR_DURING_DECOMPRESSION)
	ErrorMemoryAllocation       DecompressError = DecompressError(C.ERROR_MEMORY_ALLOCATION)
	ErrorReadingScanlines       DecompressError = DecompressError(C.ERROR_READING_SCANLINES)
	ErrorTargetResolutionNotSet DecompressError = DecompressError(C.ERROR_TRAGET_RESO_NO_SET)
	ErrorNoRgbaSupport          DecompressError = DecompressError(C.ERROR_RGBA_SUPPORT_MISSING)
)

func NewDecoder() (*Decoder, error) {
	cDecoder := C.alloc_decoder()
	if cDecoder == nil {
		return nil, errors.New("failed to allocate jpeg turbo decoder")
	}
	return &Decoder{c: cDecoder}, nil
}

func (d *Decoder) DecodeIntoRgba(rawFrame []byte, img *image.RGBA) error {
	d.SetInputBuffer(rawFrame)
	result := C.decompress_jpeg_to_rgb(d.c)
	if err := DecompressError(result); err != DecompressSuccess {
		return fmt.Errorf("decompression error: %s", err)
	}
	img.Rect = image.Rect(0, 0, int(d.c.decodec_frame_width), int(d.c.decodec_frame_height))
	img.Stride = int(d.c.decodec_frame_width) * 4
	img.Pix = C.GoBytes(unsafe.Pointer(d.c.output_buffer), C.int(d.c.output_buffer_length))
	return nil
}

func (d *Decoder) SetInputBuffer(rawFrame []byte) {
	cBuffer := C.CBytes(rawFrame)
	defer C.free(unsafe.Pointer(cBuffer))
	C.update_input_buffer(d.c, (*C.uchar)(cBuffer), C.int(len(rawFrame)))
}

func (d *Decoder) UpdateScaleResolution(width, height int) {
	C.update_scale_resolution(d.c, C.int(width), C.int(height))
}

func (d *Decoder) GetOriginalFrameResolution() (int, int) {
	return int(d.c.original_frame_width), int(d.c.original_frame_height)
}

func (d *Decoder) GetDecodedResolution() (int, int) {
	return int(d.c.decodec_frame_width), int(d.c.decodec_frame_height)
}

func (d *Decoder) CreateRgbaImage() *image.RGBA {
	w, h := d.GetDecodedResolution()
	return image.NewRGBA(image.Rect(0, 0, w, h))
}

func (e DecompressError) Error() string {
	switch e {
	case DecompressSuccess:
		return "Decompression successful"
	case ErrorReadingHeader:
		return "Error reading JPEG header"
	case ErrorDuringDecompression:
		return "Error during decompression"
	case ErrorMemoryAllocation:
		return "Error in memory allocation"
	case ErrorReadingScanlines:
		return "Error reading scanlines"
	case ErrorTargetResolutionNotSet:
		return "Target resolution not set"
	case ErrorNoRgbaSupport:
		return "No rgba support"
	default:
		return "Unknown error"
	}
}

func (d *Decoder) Free() {
	if d.c != nil {
		C.free_decoder(d.c)
	}
}
