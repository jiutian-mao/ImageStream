
#include "CCWebSprite.h"
#include "CCInterlacedPngImage.h"
#include "http_connection.h"
#include "png_codec.h"
#include <future>

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#define strcasecmp stricmp //忽略大小写比较
#endif

namespace cocos2d {


// Callback function used by libcurl for collect response data
size_t WebSprite::DataBridge::WriteData(void *ptr, size_t size, size_t nmemb, void *stream) {
	if (stream == nullptr || ptr == nullptr) {
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
	interlaced_png_image_buff_(new InterlacedPngImage()), code_pass_(-1),format(Image::Format::UNKOWN),
    _size(0,0)
{

}

WebSprite::~WebSprite() {
	CCLOG("~WebSprite:%p",this);
	if (http_connection_ != nullptr) {
		http_connection_->SetWriteCallBack(nullptr, nullptr);
		http_connection_->SetProgressCallBack(nullptr, nullptr);
		http_connection_->setCompleteCallBack(nullptr);
		http_connection_->clear();
	}
	png_coder_->SetReadCallBack(nullptr, nullptr, nullptr, nullptr);
    interlaced_png_image_buff_->setCompleteCallBack(nullptr);
	
    
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

	WebSprite * sp = new WebSprite();
	sp->file_url_ = file_url;
	sp->autorelease();
	return sp;
  return nullptr;
}

bool WebSprite::initWithFileUrl(const char *file_url) {
	Sprite::init();
	if (isRemotoeFileUrl(file_url)) {
		return initWithRemoteFile();
	}
	return false;
}

bool WebSprite::initWithRemoteFile() {
	assert(http_connection_ == nullptr);
	http_connection_ = std::make_shared<HttpConnection>();
	http_connection_->Init(file_url_.c_str());
	this->retain();
	http_connection_->SetWriteCallBack(this, WebSprite::DataBridge::WriteData);
    http_connection_->SetProgressCallBack(interlaced_png_image_buff_, WebSprite::DataBridge::OnProgress);
	http_connection_->setCompleteCallBack(CC_CALLBACK_0(WebSprite::saveURLPic,this));

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


void WebSprite::setURL(std::string url)
{
	this->file_url_ = url;
}

void WebSprite::clearPic()
{
	
}

void WebSprite::getHttpPic(cocos2d::Size size_)
{
	std::string _storagePath = FileUtils::getInstance()->getWritablePath();
	int posLast = file_url_.find_last_of('.');
	std::string lastStr = file_url_.substr(posLast+1);
	if(!(strcasecmp(lastStr.c_str(),"png") == 0 || strcasecmp(lastStr.c_str(),"jpg") == 0)) //?Ǳ??ͼƬurl
	{
		m_picName ="0.jpg";
	}else{

		if (posLast  == std::string::npos)
		{
			CCLOG("URL wrong!!!");
			return;
		}

		int pos = file_url_.find_last_of('/');
		if (pos  == std::string::npos)
		{
			CCLOG("pos wrong!!!");
			return;
		}
        
        m_picName = file_url_.substr(pos+1);
	}

	if (FileUtils::getInstance()->isFileExist(_storagePath + m_picName))
	{
		CCLOG("file is isFileExist!!");
		bool v = this->initWithFile(_storagePath + m_picName);
		if((this->getContentSize().width > size_.width || this->getContentSize().height > size_.height)  && size_.width > 0 && size_.height > 0)
		{
			this->setScaleX(size_.width/this->getContentSize().width);
			this->setScaleY(size_.height/this->getContentSize().height);
		}

		return;
	}

	this->initWithRemoteFile();
}

void WebSprite::reciverData(unsigned char* data, size_t data_size) {
	
	if(format == Image::Format::UNKOWN){
		format = interlaced_png_image_buff_->getTextureFormat(data,data_size);

		if(format == Image::Format::PNG){
			//png初始化
			png_coder_->PrepareDecode();
			png_coder_->SetReadCallBack(this, WebSprite::DataBridge::ReadHeaderCompleteCallBack, WebSprite::DataBridge::ReadRowCompleteCallBack, WebSprite::DataBridge::ReadAllCompleteCallBack);
		}
	}

    interlaced_png_image_buff_->buffer.insert(interlaced_png_image_buff_->buffer.end(), (char*)data, (char*)((char*)data+data_size));
	if(format == Image::Format::PNG)
	{
		png_coder_->Decoding(data, data_size);
	}else if(format == Image::Format::JPG)
	{
        //解析JPG图片
		interlaced_png_image_buff_->DecodeJPG((unsigned char*)interlaced_png_image_buff_->buffer.data(),interlaced_png_image_buff_->buffer.size());
		UpdateTexture();
	}
}

void WebSprite::UpdateTexture() {
	if(interlaced_png_image_buff_->getWidth()==0 || interlaced_png_image_buff_->getHeight()==0)
	{
		return;
	}
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
        
        if(_size.width != 0 && _size.height != 0)
        {
            this->setScaleX(_size.width/this->getContentSize().width);
            this->setScaleY(_size.height/this->getContentSize().height);
        }
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
    
void WebSprite::saveURLPic()
{
	Director::getInstance()->getScheduler()->performFunctionInCocosThread([&,this]{
		do{
			if(interlaced_png_image_buff_->getWidth()==0 || interlaced_png_image_buff_->getHeight()==0)
			{
				//无效图片
				break;
			}
			std::vector<char>& _data = interlaced_png_image_buff_->buffer;
			if (_data.size() <= 0)
			{
				CCLOG("WebSprite::saveURLPic-date size <= 0!!!!");
				break;
			}
			std::string _storagePath = FileUtils::getInstance()->getWritablePath();
			CCLOG("WebSprite::saveURLPic-path:%s,%s", _storagePath.c_str(),m_picName.c_str());
			if (strlen(_storagePath.c_str()) == 0 || strlen(m_picName.c_str()) == 0) {
				break;
			}
    
			std::string fullFile = _storagePath + m_picName;
			FILE * file_ = NULL;
			file_ = fopen(fullFile.c_str(),"wb+");
			if(file_){
				fwrite(_data.data(),_data.size(),1,file_);
				fclose(file_);
			}
			_data.clear();
		}while(0);
		this->autorelease();
	});
}
    
} // namespace cocos2d
