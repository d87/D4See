#include "D4See.h"
#include "ImageFormats.h"
#include "Decoder_JPEG.h"

#include <algorithm>
#include <limits>

#define M_SOI    0xD8

using namespace D4See;



static void
my_error_exit(j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	JPEGDecoder::my_error_ptr myerr = (JPEGDecoder::my_error_ptr)cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	//  (*cinfo->err->output_message) (cinfo);
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

static void
my_output_message(j_common_ptr cinfo)
{
	JPEGDecoder::my_error_ptr myerr = (JPEGDecoder::my_error_ptr)cinfo->err;

	// Create the message
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message)(cinfo, buffer);
	LOG(buffer);
	//myerr->jpginput->jpegerror(myerr, true);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}


bool JPEGDecoder::Open(const wchar_t* filename, ImageFormat format) {


	FILE* f;
	_wfopen_s(&f, filename, L"rb");
	if (!f) {
		//errorf("Could not Open file \"%s\"", m_filename);
		return false;
	}

	if (!IsValid(f)) {
		LOG("Not a valid JPEG file");
		return false;
	}
	spec.filedesc = f;

	jerr.jpegdecoder = this;
	jerr.pub.error_exit = my_error_exit;
	jerr.pub.output_message = my_output_message;
	cinfo.err = jpeg_std_error((jpeg_error_mgr*)&jerr);
	if (setjmp(jerr.setjmp_buffer)) {
		// Jump to here if there's a libjpeg internal error
		// Prevent memory leaks, see example.c in jpeg distribution
		Close();
		throw std::runtime_error("JPEG fatal error");
		//jpeg_destroy_decompress(&m_cinfo);
		//close_file();
		return false;
	}
	
	jpeg_create_decompress(&cinfo);

	jpeg_stdio_src(&cinfo, f);
	jpeg_read_header(&cinfo, TRUE);
	//auto image_color_space = cinfo.jpeg_color_space;
	if (cinfo.jpeg_color_space == JCS_CMYK || cinfo.jpeg_color_space == JCS_YCCK) {
		// CMYK jpegs get converted by us to RGB
		cinfo.out_color_space = JCS_CMYK;  // pre-convert YCbCrK->CMYK
		//numChannels = 3;
		m_isCMYK = true;
	}
	else {
		cinfo.out_color_space = JCS_RGB;//JCS_RGB;//JCS_EXT_RGBX;//JCS_RGB;//JCS_EXT_BGRX;//JCS_EXT_RGBX; -- only for turbo
	}
	


	// Buffered mode is pretty slow, not really worth using
	
	//if (jpeg_has_multiple_scans(&cinfo)) {
	//	cinfo.buffered_image = TRUE;	// select buffered-image mode
	//}

	jpeg_start_decompress(&cinfo);

	spec.format = ImageFormat::JPEG;
	spec.filedesc = f;
	spec.numChannels = 3;
	spec.width = cinfo.output_width;
	spec.height = cinfo.output_height;
	spec.rowPitch = spec.numChannels * spec.width;
	spec.size = static_cast<uint64_t>(spec.height) * spec.rowPitch;
	spec.flipRowOrder = false;

	return true;
}

bool JPEGDecoder::IsValid(FILE* f) {
	uint8_t buf[2];
	fread(&buf, 1, 2, f);
	fseek(f, 0, SEEK_SET);
	if (buf[0] == 0xFF && buf[1] == M_SOI)
		return 1;
	/*char buf[16];
	fread(buf, 1,16,f);
	fseek(f,0,SEEK_SET);

	for (int i=0; i<16; i++)
		if (buf[i] == 0x4A)
			if ((buf[i+1] == 0x46) && (buf[i+2] == 0x49) && (buf[i+3] == 0x46))
				return 1;*/
				//4A 46 49 46  = JFIF
	return 0;
}


#undef max
#undef min


// big_enough_float<T>::float_t is a floating-point type big enough to
// handle the range and precision of a <T>. It's a float, unless T is big.
template <typename T> struct big_enough_float { typedef float float_t; };
template<> struct big_enough_float<int> { typedef double float_t; };
template<> struct big_enough_float<unsigned int> { typedef double float_t; };
template<> struct big_enough_float<int64_t> { typedef double float_t; };
template<> struct big_enough_float<uint64_t> { typedef double float_t; };
template<> struct big_enough_float<double> { typedef double float_t; };


template <class T>
T
clamp(const T& a, const T& low, const T& high)
{
	// This looks clunky, but it generates minimal code. For float, it
	// should result in just a max and min instruction, thats it.
	// This implementation is courtesy of Alex Wells, Intel, via OSL.
	T val = a;
	if (!(low <= val))  // Forces clamp(NaN,low,high) to return low
		val = low;
	if (val > high)
		val = high;
	return val;
}

template<typename S, typename D, typename F>
inline D
scaled_conversion(const S& src, F scale, F min, F max)
{
	if (std::numeric_limits<S>::is_signed) {
		F s = src * scale;
		s += (s < 0 ? (F)-0.5 : (F)0.5);
		return (D)clamp(s, min, max);
	}
	else {
		return (D)clamp((F)src * scale + (F)0.5, min, max);
	}
}


template<typename S, typename D>
inline D
convert_type(const S& src)
{
	if (std::is_same<S, D>::value) {
		// They must be the same type.  Just return it.
		return (D)src;
	}
	typedef typename big_enough_float<D>::float_t F;
	F scale = std::numeric_limits<S>::is_integer ?
		F(1) / F(std::numeric_limits<S>::max()) : F(1);
	if (std::numeric_limits<D>::is_integer) {
		// Converting to an integer-like type.
		constexpr F min = (F)std::numeric_limits<D>::min();
		constexpr F max = (F)std::numeric_limits<D>::max();
		scale *= max;
		return scaled_conversion<S, D, F>(src, scale, min, max);
	}
	else {
		// Converting to a float-like type, so we don't need to remap
		// the range
		return (D)((F)src * scale);
	}
}

inline float
ByteToFloat(uint8_t byte)
{
	return convert_type<unsigned char, float>(byte);
}

inline BYTE
FloatToByte(float src)
{
	return convert_type<float, unsigned char>(src);
}



static void
cmyk_to_rgb(int n, const unsigned char* cmyk, size_t cmyk_stride,
	unsigned char* rgb, size_t rgb_stride)
{
	for (; n; --n, cmyk += cmyk_stride, rgb += rgb_stride) {
		// JPEG seems to store CMYK as 1-x
		float C = ByteToFloat(cmyk[0]);
		float M = ByteToFloat(cmyk[1]);
		float Y = ByteToFloat(cmyk[2]);
		float K = ByteToFloat(cmyk[3]);
		float R = C * K;
		float G = M * K;
		float B = Y * K;
		rgb[0] = FloatToByte(R);
		rgb[1] = FloatToByte(G);
		rgb[2] = FloatToByte(B);
	}
}

unsigned int JPEGDecoder::Read(int startLine, int numLines, uint8_t* pDst) {
	// Start a pass on a current mip level
	if (cinfo.output_scanline == 0 || cinfo.output_scanline == cinfo.output_height) {
		spec.linesRead = 0;
		if (cinfo.buffered_image) {
			jpeg_start_output(&cinfo, cinfo.input_scan_number);
		}
	}

	int linesRead = 0;
	//int remains = (spec.height - startLine);
	//numLines = std::min(remains, numLines);
	if (m_isCMYK) {
		
		// libjpeg can't covert CMYK to RGB automatically, so doing it here
		m_cmyk_buf.resize(numLines * spec.width * 4);
		void* pCMYKDst = &m_cmyk_buf[0];
		linesRead = jpeg_read_scanlines(&cinfo, (JSAMPLE**)&pCMYKDst, numLines);

		cmyk_to_rgb(spec.width*numLines, (uint8_t*)pCMYKDst, 4,
			(uint8_t*)pDst, 3);
	}
	else {
		linesRead = jpeg_read_scanlines(&cinfo, &pDst, numLines);
	}
	spec.linesRead += linesRead;
	
	// End pass on a current mip level
	if (spec.linesRead >= cinfo.output_height) {
		if(cinfo.buffered_image) {
			jpeg_finish_output(&cinfo);
			if (jpeg_input_complete(&cinfo)) {
				jpeg_finish_decompress(&cinfo);
				spec.isFinished = true;
				Close();
			}
		}
		else {
			jpeg_finish_decompress(&cinfo);
			spec.isFinished = true;
			Close();
		}
		
	}
	
	return linesRead;	
}

void JPEGDecoder::Close() {
	if (spec.filedesc) {
		jpeg_destroy_decompress(&cinfo);
		fclose(spec.filedesc);
		spec.filedesc = NULL;
	}
}

JPEGDecoder::~JPEGDecoder() {
	//Close();
}