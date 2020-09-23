#include "D4See.h"
#include "ImageFormats.h"
#include "Decoder_JPEG.h"

#include <algorithm>
#include <limits>

#define M_SOI    0xD8

using namespace D4See;

static void exif_loader_write_file_descriptor(ExifLoader* l, FILE* f);
static int get_rotation_from_exif_orientation(short orientation);
static void show_tag(ExifData* d, ExifIfd ifd, ExifTag tag);
static ExifShort get_tag_as_short(ExifData* d, ExifIfd ifd, ExifTag tag);

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


bool JPEGDecoder::Open(FILE* f, const wchar_t* filename, ImageFormat format) {

	if (!IsValid(f)) {
		LOG("Not a valid JPEG file");
		throw std::runtime_error("Not a valid JPEG file");
		return false;
	}
	spec.filedesc = f;


	// EXIF parsing

	ExifData* edata;
	ExifLoader* loader;
	uint16_t orientation = 1;

	loader = exif_loader_new();
	exif_loader_write_file_descriptor(loader, f);
	edata = exif_loader_get_data(loader);
	exif_loader_unref(loader);

	if (edata) {
		orientation = get_tag_as_short(edata, EXIF_IFD_0, EXIF_TAG_ORIENTATION);
	}
	exif_data_unref(edata);

	// END EXIF

	int rotation = get_rotation_from_exif_orientation(orientation);

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




// EXIF stuff

static void
exif_loader_write_file_descriptor(ExifLoader* l, FILE* f)
{
	int size;
	unsigned char data[1024];

	if (!l || !f)
		return;

	// search just a little into a file
	for (int i = 0; i < 10; i++) {
		//while(1) {
		size = fread(data, 1, sizeof(data), f);
		if (size <= 0)
			break;
		if (!exif_loader_write(l, data, size))
			break;
	}
	fseek(f, 0, SEEK_SET);
}

//{ EXIF_TAG_ORIENTATION,
	//{ "",
	//N_("Top-left"), // 0 degrees, no flip
	//N_("Top-right"), // 0 degrees, flip
	//N_("Bottom-right"), // 180 degrees, no flip
	//N_("Bottom-left"), // 180 degrees, flip
	//N_("Left-top"), // 270 degrees, flip
	//N_("Right-top"), // 270 degrees, no flip
	//N_("Right-bottom"), // 90 degrees, flip
	//N_("Left-bottom"), // 90 degrees, no flip
	//NULL }},

static int get_rotation_from_exif_orientation(short orientation) {
	switch (orientation)
	{
	case 1:
	case 2:
		return 0;
	case 3:
	case 4:
		return 180;
	case 5:
	case 6:
		return 270;
	case 7:
	case 8:
		return 90;
	default:
		break;
	}
	return 0;
}

/* Remove spaces on the right of the string */
static void trim_spaces(char* buf)
{
	char* s = buf - 1;
	for (; *buf; ++buf) {
		if (*buf != ' ')
			s = buf;
	}
	*++s = 0; /* nul terminate the string on the first of the final spaces */
}


static ExifShort get_tag_as_short(ExifData* d, ExifIfd ifd, ExifTag tag) {
	ExifEntry* e = exif_content_get_entry(d->ifd[ifd], tag);
	if (e) {
		const ExifByteOrder o = exif_data_get_byte_order(e->parent->parent);
		if (e->format == EXIF_FORMAT_SHORT) {
			ExifShort v_short;
			v_short = exif_get_short(e->data, o);
			return v_short;
		}
	}
	return 0;
}

/* Show the tag name and contents if the tag exists */
static void show_tag(ExifData* d, ExifIfd ifd, ExifTag tag)
{
	/* See if this tag exists */
	ExifEntry* entry = exif_content_get_entry(d->ifd[ifd], tag);
	if (entry) {
		char buf[1024];

		/* Get the contents of the tag in human-readable form */
		exif_entry_get_value(entry, buf, sizeof(buf));

		/* Don't bother printing it if it's entirely blank */
		trim_spaces(buf);
		if (*buf) {
			printf("EXIF TAG %s: %s\n", exif_tag_get_name_in_ifd(tag, ifd), buf);
		}
	}
}