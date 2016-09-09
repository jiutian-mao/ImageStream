#include "CCInterlacedPngImage.h"
#include <future>

extern "C"
{
#if CC_USE_JPEG
#include "jpeg/include/win32/jpeglib.h"
#endif // CC_USE_JPEG
}

#if CC_USE_JPEG
    struct MyErrorMgr
    {
        struct jpeg_error_mgr pub;	/* "public" fields */
        jmp_buf setjmp_buffer;	/* for return to caller */
    };
    
    typedef struct MyErrorMgr * MyErrorPtr;
    
    /*
     * Here's the routine that will replace the standard error_exit method:
     */
    
    METHODDEF(void)
    myErrorExit(j_common_ptr cinfo)
    {
        /* cinfo->err really points to a MyErrorMgr struct, so coerce pointer */
        MyErrorPtr myerr = (MyErrorPtr) cinfo->err;
        
        /* Always display the message. */
        /* We could postpone this until after returning, if we chose. */
        /* internal message function cann't show error message in some platforms, so we rewrite it here.
         * edit it if has version confilict.
         */
        //(*cinfo->err->output_message) (cinfo);
        char buffer[JMSG_LENGTH_MAX];
        (*cinfo->err->format_message) (cinfo, buffer);
        CCLOG("jpeg error: %s", buffer);
        
        /* Return control to the setjmp point */
        longjmp(myerr->setjmp_buffer, 1);
    }
#endif // CC_USE_JPEG


namespace cocos2d {

namespace {
struct GimpImage{
  int  	 width;
  int  	 height;
  int  	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  unsigned char 	 pixel_data[8 * 8 * 4 + 1];
}null_content_image = {
  1, 1, 3,
  "\275\275\275"
};

} //namespace anonymous

    InterlacedPngImage::InterlacedPngImage():
    _complteFunc(nullptr)
{
	
}
    
InterlacedPngImage::~InterlacedPngImage()
{
    this->_complteFunc = nullptr;
    this->buffer.clear();
}

void InterlacedPngImage::setImageHeader(size_t width, size_t height, int image_color_type, int out_channel) {
    _width = width;
    _height = height;
	png_uint_32 color_type = (png_uint_32)image_color_type;
	switch (color_type)
	{
	case PNG_COLOR_TYPE_GRAY:
		_renderFormat = cocos2d::Texture2D::PixelFormat::I8;
		break;
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		_renderFormat = cocos2d::Texture2D::PixelFormat::AI88;
		break;
	case PNG_COLOR_TYPE_RGB:
		_renderFormat = cocos2d::Texture2D::PixelFormat::RGB888;
		break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		_renderFormat = cocos2d::Texture2D::PixelFormat::RGBA8888;
		break;
	case PNG_COLOR_TYPE_PALETTE:
		_renderFormat = cocos2d::Texture2D::PixelFormat::RGBA8888;
		break;
	default:
		break;
	}

   	_dataLen = _width * _height * out_channel * sizeof(unsigned char);
	_data = static_cast<unsigned char*>(malloc(_dataLen + 1));
	memset(_data, 0, _dataLen);
}

void InterlacedPngImage::setImageBodyData(char* data, size_t data_size) {
	memcpy(_data, data, _dataLen);
}

void InterlacedPngImage::setCompleteCallBack(const std::function<void()>& func){
    this->_complteFunc = func;
}

Image::Format InterlacedPngImage::getTextureFormat(unsigned char* data, size_t data_size)
{
	if(isPng(data,data_size)){
		return Image::Format::PNG;
	}else if(isJpg(data,data_size)){
		return Image::Format::JPG;
	}
	
	return Image::Format::UNKOWN;
}

int InterlacedPngImage::OnProgress(void *ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded)
{
    if(fabs(totalToDownload) < 0.00000001)
    {
        return 0;
    }
    InterlacedPngImage* web_sprite = static_cast<InterlacedPngImage*>(ptr);
    if(web_sprite->buffer.capacity() < (long)totalToDownload){
        web_sprite->buffer.reserve((long)totalToDownload);
    }
    
    return 0;
}

bool InterlacedPngImage::DecodeJPG(unsigned char* data, size_t dataLen)
{
	#if CC_USE_JPEG
    /* these are standard libjpeg structures for reading(decompression) */
    struct jpeg_decompress_struct cinfo;
    /* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct MyErrorMgr jerr;
    /* libjpeg data structure for storing one row, that is, scanline of an image */
    JSAMPROW row_pointer[1] = {0};
    unsigned long location = 0;

    bool ret = false;
    do 
    {
        /* We set up the normal JPEG error routines, then override error_exit. */
		cinfo.err = jpeg_std_error(&jerr.pub);
		jerr.pub.error_exit = myErrorExit;
		/* Establish the setjmp return context for MyErrorExit to use. */
		if (setjmp(jerr.setjmp_buffer))
        {
			/* If we get here, the JPEG code has signaled an error.
			 * We need to clean up the JPEG object, close the input file, and return.
			 */
			jpeg_destroy_decompress(&cinfo);
			break;
		}

        /* setup decompression process and source, then read JPEG header */
        jpeg_create_decompress( &cinfo );
#ifndef CC_TARGET_QT5
        jpeg_mem_src(&cinfo, const_cast<unsigned char*>(data), dataLen);
#endif /* CC_TARGET_QT5 */

        /* reading the image header which contains image information */
#if (JPEG_LIB_VERSION >= 90)
        // libjpeg 0.9 adds stricter types.
        jpeg_read_header(&cinfo, TRUE);
#else
        jpeg_read_header(&cinfo, TRUE);
#endif

        // we only support RGB or grayscale
        if (cinfo.jpeg_color_space == JCS_GRAYSCALE)
        {
            _renderFormat = Texture2D::PixelFormat::I8;
        }else
        {
            cinfo.out_color_space = JCS_RGB;
            _renderFormat = Texture2D::PixelFormat::RGB888;
        }

        /* Start decompression jpeg here */
        jpeg_start_decompress( &cinfo );

        if(_data == nullptr && _dataLen == 0){
            
            /* init image info */
            _width  = cinfo.output_width;
            _height = cinfo.output_height;
            _hasPremultipliedAlpha = false;
            
            _dataLen = cinfo.output_width*cinfo.output_height*cinfo.output_components;
            _data = static_cast<unsigned char*>(malloc(_dataLen * sizeof(unsigned char)));
        }
        CC_BREAK_IF(! _data);

        /* now actually read the jpeg into the raw buffer */
        /* read one scan line at a time */
        while (cinfo.output_scanline < cinfo.output_height)
        {
            row_pointer[0] = _data + location;
            location += cinfo.output_width*cinfo.output_components;
            jpeg_read_scanlines(&cinfo, row_pointer, 1);
        }

	/* When read image file with broken data, jpeg_finish_decompress() may cause error.
	 * Besides, jpeg_destroy_decompress() shall deallocate and release all memory associated
	 * with the decompression object.
	 * So it doesn't need to call jpeg_finish_decompress().
	 */
	//jpeg_finish_decompress( &cinfo );
        jpeg_destroy_decompress( &cinfo );
        /* wrap up decompression, destroy objects, free pointers and close open files */        
        ret = true;
    } while (0);

    return ret;
#else
    return false;
#endif // CC_USE_JPEG
}

} // namespace cocos2d


