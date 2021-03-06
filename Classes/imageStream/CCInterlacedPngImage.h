#ifndef COCOS2D_TESTS_CCInterlacedPngImage_H_
#define COCOS2D_TESTS_CCInterlacedPngImage_H_

#include "cocos2d.h"
#include "png.h"
#include <functional>

namespace cocos2d {

class InterlacedPngImage : public cocos2d::Image {

public:

    InterlacedPngImage();
    
    virtual ~InterlacedPngImage();

	void setImageHeader(size_t width, size_t height, int image_color_type, int out_channel);
	void setImageBodyData(char* data, size_t data_size);

	size_t WriteData(void *ptr, size_t size, size_t nmemb, void *stream);

	int OnProgress(void *ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded);

    void setCompleteCallBack(const std::function<void()>& func);
        
	Image::Format getTextureFormat(unsigned char* data, size_t data_size);
	
	bool DecodeJPG(unsigned char* data, size_t data_size);

	std::vector<char> buffer;
 private:
	
    std::function<void()> _complteFunc;
};
  
} // namespace cocos2d

#endif //COCOS2D_TESTS_CCInterlacedPngImage_H_
