//
//  CCInterlacedPngImage.h
//  cocos2d_tests
//
//  Created by sachin on 5/10/14.
//
//

#ifndef COCOS2D_TESTS_CCInterlacedPngImage_H_
#define COCOS2D_TESTS_CCInterlacedPngImage_H_

#include "cocos2d.h"
#include "png.h"
#include <functional>

namespace cocos2d {

class InterlacedPngImage : public cocos2d::Image {
 public:

  InterlacedPngImage();

  bool initWithFilePath(const char* file_path);
  bool initWithFilePath(const char* local_file_path, const char* remote_file_path);
	/*thread safe*/
	void setImageHeader(size_t width, size_t height, int image_color_type, int out_channel);
	void setImageBodyData(char* data, size_t data_size);

	Image::Format getTextureFormat(unsigned char* data, size_t data_size);
	
	size_t WriteData(void *ptr, size_t size, size_t nmemb, void *stream);
	//½ø¶È
	int OnProgress(void *ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded);

	bool DecodeJPG(unsigned char* data, size_t data_size);

	void setUpdateCallBack(const std::function<void()>& func);

 private:
	
	bool initWithPngData(const unsigned char * data, ssize_t dataLen);
	void updateTexture();
	
	std::vector<char> buffer;
	std::function<void()> _updateTexture;
};
  
} // namespace cocos2d

#endif //COCOS2D_TESTS_CCInterlacedPngImage_H_
