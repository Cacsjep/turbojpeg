package turbojpeg

import (
	"fmt"
	"image"
	"image/jpeg"
	"os"
	"testing"
)

func decode(raw []byte, decoder *Decoder, w, h int, exi *image.RGBA) error {
	decoder.UpdateScaleResolution(w, h)
	err := decoder.DecodeIntoRgba(raw, exi)
	if err != nil {
		return err
	}
	return nil
}

func decode_and_create(t *testing.T, decoder *Decoder, w, h int, exi *image.RGBA) {
	demofile, err := os.ReadFile("test.jpeg")

	if err != nil {
		t.Fatalf("Error reading demo file: %s", err.Error())
	}

	err = decode(demofile, decoder, w, h, exi)
	if err != nil {
		t.Fatalf("on decompressing: %s", err.Error())
	}

	file, err := os.Create(fmt.Sprintf("out/result%d_%d.jpg", w, h))
	if err != nil {
		fmt.Println("Error creating file:", err)
		return
	}
	defer file.Close()
	if err := jpeg.Encode(file, exi, nil); err != nil {
		fmt.Println("Error encoding JPEG:", err)
		return
	}
}

func TestDecoder(t *testing.T) {
	decoder, err := NewDecoder()
	if err != nil {
		t.Fatalf("Error initializing decoder: %s", err.Error())
		return
	}

	exi := image.RGBA{}
	decode_and_create(t, decoder, 100, 100, &exi)
	//	decode_and_create(t, decoder, 300, 300, &exi)
	//	decode_and_create(t, decoder, 500, 500, &exi)
	decoder.Free()
}
