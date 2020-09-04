#include "D4See.h"
#include "ImageFormats.h"
#include "Decoder_JPEG.h"

using namespace D4See;

bool JPEGDecoder::open(const wchar_t* filename) {

	mutex.lock();

	FILE* f;
	_wfopen_s(&f, filename, L"rb");
	if (!f) {
		//errorf("Could not open file \"%s\"", m_filename);
		return false;
	}
	spec.filedesc = f;


	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_stdio_src(&cinfo, f);
	jpeg_read_header(&cinfo, TRUE);
	cinfo.out_color_space = JCS_EXT_RGB;//JCS_RGB;//JCS_EXT_RGBX;//JCS_RGB;//JCS_EXT_BGRX;//JCS_EXT_RGBX; -- only for turbo


	// Buffered mode is pretty slow, not really worth using
	
	//if (jpeg_has_multiple_scans(&cinfo)) {
	//	cinfo.buffered_image = TRUE;	// select buffered-image mode
	//}

	jpeg_start_decompress(&cinfo);

	spec.format = ImageFormat::JPEG;
	spec.filedesc = f;
	spec.nchannels = 3;
	spec.width = cinfo.output_width;
	spec.height = cinfo.output_height;
	spec.rowPitch = cinfo.output_components * spec.width;
	spec.size = static_cast<uint64_t>(spec.height) * spec.rowPitch;
	spec.flipRowOrder = false;

	mutex.unlock();
}


unsigned int JPEGDecoder::read(int startLine, int numLines, uint8_t* pDst) {
	mutex.lock();
	// Start a pass on a current mip level
	if (cinfo.output_scanline == 0 || cinfo.output_scanline == cinfo.output_height) {
		spec.linesRead = 0;
		if (cinfo.buffered_image) {
			jpeg_start_output(&cinfo, cinfo.input_scan_number);
		}
	}
	//jpeg_start_output(&cinfo, cinfo.input_scan_number);
	int linesRead = jpeg_read_scanlines(&cinfo, &pDst, numLines);
	spec.linesRead += linesRead;
	mutex.unlock();

	// End pass on a current mip level
	if (spec.linesRead >= cinfo.output_height) {
		if(cinfo.buffered_image) {
			jpeg_finish_output(&cinfo);
			if (jpeg_input_complete(&cinfo)) {
				jpeg_finish_decompress(&cinfo);
				spec.isFinished = true;
				close();
			}
		}
		else {
			jpeg_finish_decompress(&cinfo);
			spec.isFinished = true;
			close();
		}
		
	}
	return linesRead;
	//jpeg_finish_output(&cinfo);
	
}

void JPEGDecoder::close() {
	mutex.lock();
	if (spec.filedesc) {
		
		jpeg_destroy_decompress(&cinfo);
		fclose(spec.filedesc);
		LOG_DEBUG("Closing decoder");
		spec.filedesc = NULL;
	}
	mutex.unlock();
}

JPEGDecoder::~JPEGDecoder() {
	//close();
}