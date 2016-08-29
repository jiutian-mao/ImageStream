#include "HelloWorldScene.h"

#include "CCWebSprite.h"

USING_NS_CC;


Scene* HelloWorld::scene()
{
    // 'scene' is an autorelease object
    auto scene = Scene::create();
    
    // 'layer' is an autorelease object
    HelloWorld *layer = HelloWorld::create();
    
    // add layer as a child to scene
    scene->addChild(layer);
    
    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !Layer::init() )
    {
        return false;
    }
    
    auto visibleSize = Director::getInstance()->getVisibleSize();
    auto origin = Director::getInstance()->getVisibleOrigin();
    
    /////////////////////////////
    // 2. add a menu item with "X" image, which is clicked to quit the program
    //    you may modify it.
    
    // add a "close" icon to exit the progress. it's an autorelease object
    auto closeItem = MenuItemImage::create(
                                           "CloseNormal.png",
                                           "CloseSelected.png",
                                           CC_CALLBACK_1(HelloWorld::menuCloseCallback,this));
    
    closeItem->setPosition(origin + Point(visibleSize) - Point(closeItem->getContentSize() / 2));
    
    // create menu, it's an autorelease object
    auto menu = Menu::create(closeItem, NULL);
    menu->setPosition(Point::ZERO);
    this->addChild(menu, 1);
    
    /////////////////////////////
    // 3. add your codes below...
    
    // add a label shows "Hello World"
    // create and initialize a label
    
    auto label = LabelTTF::create("Hello World", "Arial", 32);
    
    // position the label on the center of the screen
    label->setPosition(Point(origin.x + visibleSize.width/2,
                             origin.y + visibleSize.height - label->getContentSize().height));
    
    // add the label as a child to this layer
    this->addChild(label, 1);
    
    WebSprite* sprite = WebSprite::createWithFileUrl("http://www.zzy1314.com/images/t2.png");//http://i.mmcdn.cn/simba/img/TB1G0mHMVXXXXa6aFXXSutbFXXX.jpg
    // add "HelloWorld" splash screen"
    //auto sprite = Sprite::createWithTexture(texture);
    // position the sprite on the center of the screen
    sprite->setPosition(Point(150, visibleSize.height /2));
    this->addChild(sprite);
//    WebSprite* sprite2 = WebSprite::createWithFileUrl("future.png");
    // add "HelloWorld" splash screen"
    //auto sprite = Sprite::createWithTexture(texture);
    // position the sprite on the center of the screen
//    sprite2->setPosition(Point(400, visibleSize.height / 2));
    // add the sprite as a child to this layer
//    this->addChild(sprite2);
    
    return true;
}

void HelloWorld::menuCloseCallback(Ref* sender)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WP8) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
    MessageBox("You pressed the close button. Windows Store Apps do not implement a close button.","Alert");
    return;
#endif
    
    Director::getInstance()->end();
    
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}