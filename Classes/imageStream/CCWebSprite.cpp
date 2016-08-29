//
//  CCWebSprite.cpp
//  cocos2d_tests
//
//  Created by sachin on 5/13/14.
//
//

#include "CCWebSprite.h"
#include "CCInterlacedPngImage.h"
#include "http_connection.h"
#include "png_codec.h"

#include <future>

namespace cocos2d {


// Callback function used by libcurl for collect response data
size_t WebSprite::DataBridge::WriteData(void *ptr, size_t size, size_t nmemb, void *stream) {
	if (stream == nullptr) {
		return 0;
	}
	WebSprite* web_sprite = static_cast<WebSprite*>(stream);
	if (web_sprite == nullptr) {
		return 0;
	}
	size_t sizes = size * nmemb;
	web_sprite->reciverData((unsigned  char*)ptr, sizes);
	return sizes;
}
    
int WebSprite::DataBridge::OnProgress(void *ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded)
{
    InterlacedPngImage* web_sprite = static_cast<InterlacedPngImage*>(ptr);
	return web_sprite->OnProgress(ptr, totalToDownload,nowDownloaded,totalToUpLoad,nowUpLoaded);
}

void WebSprite::DataBridge::ReadHeaderCompleteCallBack(void* ptr) {
	WebSprite* web_sprite = static_cast<WebSprite*>(ptr);
	web_sprite->readHeaderComplete();
}

void WebSprite::DataBridge::ReadRowCompleteCallBack(void* ptr, int pass) {
  WebSprite* web_sprite = static_cast<WebSprite*>(ptr);
  web_sprite->readRowComplete(pass);
}

void WebSprite::DataBridge::ReadAllCompleteCallBack(void* ptr) {
  WebSprite* web_sprite = static_cast<WebSprite*>(ptr);
  web_sprite->readAllComplete();
}


WebSprite::WebSprite() 
	:http_connection_(nullptr),png_coder_(std::make_shared<util::PNGCodec>()),
	interlaced_png_image_buff_(new InterlacedPngImage()), code_pass_(-1),format(Image::Format::UNKOWN)
{

}

WebSprite::~WebSprite() {
	if (http_connection_ != nullptr) {
		http_connection_->SetWriteCallBack(nullptr, WebSprite::DataBridge::WriteData);
        http_connection_->SetProgressCallBack(nullptr, WebSprite::DataBridge::OnProgress);
	}
	png_coder_->SetReadCallBack(nullptr, nullptr, nullptr, nullptr);
	CC_SAFE_RELEASE(interlaced_png_image_buff_);
}

WebSprite* WebSprite::create() {
  WebSprite *sprite = new WebSprite();
  if (sprite && sprite->init())
  {
    sprite->autorelease();
    return sprite;
  }
  CC_SAFE_DELETE(sprite);
  return nullptr;
}

WebSprite* WebSprite::createWithFileUrl(const char *file_url) {
  WebSprite *sprite = new WebSprite();
  if (sprite && sprite->initWithFileUrl(file_url))
  {
    sprite->autorelease();
    return sprite;
  }
  CC_SAFE_DELETE(sprite);
  return nullptr;
}

bool WebSprite::initWithFileUrl(const char *file_url) {
	Sprite::init();
	file_url_ = file_url;
	if (isRemotoeFileUrl(file_url)) {
		return initWithRemoteFile();
	}
	return false;
}

bool WebSprite::initWithRemoteFile() {
	assert(http_connection_ == nullptr);
	http_connection_ = std::make_shared<HttpConnection>();
	http_connection_->Init(file_url_.c_str());
	
	http_connection_->SetWriteCallBack(this, WebSprite::DataBridge::WriteData);
    http_connection_->SetProgressCallBack(interlaced_png_image_buff_, WebSprite::DataBridge::OnProgress);
	interlaced_png_image_buff_->setUpdateCallBack(CC_CALLBACK_0(WebSprite::UpdateTexture,this));
	std::thread http_thread = std::thread(std::bind(&HttpConnection::PerformGet, http_connection_));
	http_thread.detach();
	return true;
}

bool WebSprite::isRemotoeFileUrl(const char *file_url) {
	if (strlen(file_url) > 4 && (strncmp(file_url, "http", 4) == 0)) {
		return true;
	}
	return false;
}

void WebSprite::reciverData(unsigned char* data, size_t data_size) {
	if(format == Image::Format::UNKOWN){ //首次读取
		format = interlaced_png_image_buff_->getTextureFormat(data,data_size);

		if(format == Image::Format::PNG){
			//解码初始化
			png_coder_->PrepareDecode();
			png_coder_->SetReadCallBack(this, WebSprite::DataBridge::ReadHeaderCompleteCallBack, WebSprite::DataBridge::ReadRowCompleteCallBack, WebSprite::DataBridge::ReadAllCompleteCallBack);
		}
	}

	if(format == Image::Format::PNG)
	{
		png_coder_->Decoding(data, data_size);
	}else if(format == Image::Format::JPG)
	{
		interlaced_png_image_buff_->DecodeJPG(data,data_size);
	}
}

void WebSprite::UpdateTexture() {
    Director::getInstance()->getScheduler()->performFunctionInCocosThread([&,this]{
        cocos2d::Texture2D* texture = cocos2d::Director::getInstance()->getTextureCache()->addImage(interlaced_png_image_buff_, file_url_);
        if(format == Image::Format::JPG)
		{
			texture->initWithImage(interlaced_png_image_buff_);
		}else if(format == Image::Format::PNG){
        	texture->updateWithData(interlaced_png_image_buff_->getData(), 0, 0, interlaced_png_image_buff_->getWidth(),
        				interlaced_png_image_buff_->getHeight());
		}

        SpriteFrame* sprite_frame = cocos2d::SpriteFrame::createWithTexture(texture,
                                                                            CCRectMake(0,0,texture->getContentSize().width, texture->getContentSize().height));
        Sprite::setSpriteFrame(sprite_frame);
    });
}

void WebSprite::readHeaderComplete() {
	interlaced_png_image_buff_->setImageHeader(png_coder_->png_width(), png_coder_->png_height(), png_coder_->png_color_type(), png_coder_->png_output_channels());
}

void WebSprite::readRowComplete(int pass) {
  if (code_pass_ < pass) {
		interlaced_png_image_buff_->setImageBodyData((char*)png_coder_->png_data_buffer(), png_coder_->png_data_size());
      code_pass_ = pass;
      this->UpdateTexture();
  }
}

// run on sub thread
void WebSprite::readAllComplete() {
	interlaced_png_image_buff_->setImageBodyData((char*)png_coder_->png_data_buffer(), png_coder_->png_data_size());
    this->UpdateTexture();
}
  
} // namespace cocos2d
