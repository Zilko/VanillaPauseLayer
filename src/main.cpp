using namespace geode::prelude;

#include <Geode/modify/PauseLayer.hpp>

$on_mod(Loaded) {

    if (!Mod::get()->hasSavedValue("vanilla-layer"))
        Mod::get()->setSavedValue("vanilla-layer", true);

}

class SwipeLayer : public CCLayer {

private:

    CCNode* m_pauseLayer = nullptr;
    CCNode* m_vanillaLayer = nullptr;
    
    bool m_isVanillaLayer = false;
    bool m_isMoving = false;
    bool m_isAnimating = false;

    float m_startX = 0.f;
    float m_startLayerPosX = 0.f;
    float m_currentMove = 0.f;

    SwipeLayer(CCNode* pauseLayer, CCNode* vanillaLayer, bool isVanillaLayer)
        : m_pauseLayer(pauseLayer), m_vanillaLayer(vanillaLayer), m_isVanillaLayer(isVanillaLayer) {}

    void stoppedAnimating(float) {
        m_isAnimating = false;
    }

    bool init() override {
        if (!CCLayer::init()) return false;

        setTouchEnabled(true);
        setTouchMode(kCCTouchesOneByOne);

        return true;
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override {
        CCLayer::ccTouchBegan(touch, event);

        if (m_isAnimating) return false;

        m_startX = touch->getLocation().x;
        m_startLayerPosX = m_pauseLayer->getPositionX();

        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent* event) override {
        CCLayer::ccTouchMoved(touch, event);

        m_currentMove = touch->getLocation().x - m_startX;

        if (!m_isMoving && abs(m_currentMove) > 20) {
            m_startX = touch->getLocation().x;
            m_currentMove = 0.f;
            m_isMoving = true;
        } else {
            if (m_isVanillaLayer) {
                if (m_currentMove < 0) {
                    m_pauseLayer->setPositionX(m_pauseLayer->getContentWidth());
                    m_vanillaLayer->setPositionX(-m_pauseLayer->getContentWidth());

                    m_startLayerPosX = m_pauseLayer->getPositionX();
                } else {
                    m_pauseLayer->setPositionX(-m_pauseLayer->getContentWidth());
                    m_vanillaLayer->setPositionX(m_pauseLayer->getContentWidth());

                    m_startLayerPosX = m_pauseLayer->getPositionX();
                }
            } else {
                if (m_currentMove < 0)
                    m_vanillaLayer->setPositionX(m_pauseLayer->getContentWidth());
                else
                    m_vanillaLayer->setPositionX(-m_pauseLayer->getContentWidth());
            }
        }

        if (m_isMoving)
            m_pauseLayer->setPositionX(m_startLayerPosX + m_currentMove);
    }

    void ccTouchEnded(CCTouch* touch, CCEvent* event) override {
        CCLayer::ccTouchEnded(touch, event);

        if (abs(m_currentMove) > 99.f) {
            if (m_isVanillaLayer)
                m_pauseLayer->runAction(CCEaseSineInOut::create(CCMoveTo::create(0.15f, {0, m_pauseLayer->getPositionY()})));
            else {
                if (m_currentMove < 0)
                    m_pauseLayer->runAction(CCEaseSineInOut::create(CCMoveTo::create(0.15f, {-m_pauseLayer->getContentWidth(), m_pauseLayer->getPositionY()})));
                else
                    m_pauseLayer->runAction(CCEaseSineInOut::create(CCMoveTo::create(0.15f, {m_pauseLayer->getContentWidth(), m_pauseLayer->getPositionY()})));
            }

            m_isVanillaLayer = !m_isVanillaLayer;

            Mod::get()->setSavedValue("vanilla-layer", m_isVanillaLayer);
        } else
            m_pauseLayer->runAction(CCEaseSineInOut::create(CCMoveTo::create(0.15f, {m_startLayerPosX, m_pauseLayer->getPositionY()})));

        if (m_isMoving)
            m_isAnimating = true;

        m_startX = 0.f;
        m_startLayerPosX = 0.f;
        m_currentMove = 0.f;
        m_isMoving = false;

        scheduleOnce(schedule_selector(SwipeLayer::stoppedAnimating), 0.15f);
    }

    virtual void registerWithTouchDispatcher() {
        CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, 0, false);
    }

public:

    static SwipeLayer* create(CCNode* pauseLayer, CCNode* vanillaLayer, bool isVanillaLayer) {
        SwipeLayer* ret = new SwipeLayer(pauseLayer, vanillaLayer, isVanillaLayer);

        if (ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

};

class VanillaPauseLayer : public PauseLayer {

private:

    PauseLayer* m_real = nullptr;

    VanillaPauseLayer(PauseLayer* real) 
        : m_real(real) {}

    void myMusicSliderChanged(CCObject* thumb) {
        m_real->musicSliderChanged(thumb);
        m_real->getChildByType<Slider>(0)->setValue(static_cast<SliderThumb*>(thumb)->getValue());
    }

    void mySfxSliderChanged(CCObject* thumb) {
        m_real->sfxSliderChanged(thumb);
        m_real->getChildByType<Slider>(1)->setValue(static_cast<SliderThumb*>(thumb)->getValue());
    }

    void customSetup() override {
        PauseLayer::customSetup();

        for (CCMenuItem* button : CCArrayExt<CCMenuItem*>(getChildByType<CCMenu>(0)->getChildren()))
            button->m_pListener = m_real;

        m_musicSlider = getChildByType<Slider>(0);
        m_sfxSlider = getChildByType<Slider>(1);
        
        m_musicSlider->getThumb()->m_pfnSelector = menu_selector(VanillaPauseLayer::myMusicSliderChanged);
        m_sfxSlider->getThumb()->m_pfnSelector = menu_selector(VanillaPauseLayer::mySfxSliderChanged);
    }


    virtual bool ccTouchBegan(CCTouch*, CCEvent*) override {
        return false;
    }

    virtual void keyDown(enumKeyCodes) override {}

public:

    Slider* m_musicSlider = nullptr;
    Slider* m_sfxSlider = nullptr;

    static VanillaPauseLayer* create(PauseLayer* real, bool unfocused) {
        VanillaPauseLayer* ret = new VanillaPauseLayer(real);

        ret->m_unfocused = unfocused;
        ret->m_tryingQuit = false;

        ret->CCBlockLayer::init();
        ret->autorelease();

        return ret;
    }

};

class $modify(PauseLayer) {

    struct Fields {
        VanillaPauseLayer* m_vanillaLayer = nullptr;
    };

    static void onModify(auto& self) {
        (void)self.setHookPriorityPre("PauseLayer::customSetup", Priority::Last);
    }

    void musicSliderChanged(CCObject* thumb) {
        PauseLayer::musicSliderChanged(thumb);

        m_fields->m_vanillaLayer->m_musicSlider->setValue(static_cast<SliderThumb*>(thumb)->getValue());
    }

    void sfxSliderChanged(CCObject* thumb) {
        PauseLayer::sfxSliderChanged(thumb);

        m_fields->m_vanillaLayer->m_sfxSlider->setValue(static_cast<SliderThumb*>(thumb)->getValue());
    }

    void customSetup() {
        PauseLayer::customSetup();

        auto f = m_fields.self();
        
        f->m_vanillaLayer = VanillaPauseLayer::create(this, m_unfocused);
        f->m_vanillaLayer->setID("vanilla-pause-layer"_spr);
        f->m_vanillaLayer->setPositionX(getContentWidth());

        addChild(f->m_vanillaLayer);

        bool isVanillaLayer = Mod::get()->getSavedValue<bool>("vanilla-layer");

        SwipeLayer* swipeLayer = SwipeLayer::create(this, f->m_vanillaLayer, isVanillaLayer);

        f->m_vanillaLayer->addChild(swipeLayer);
        
        if (isVanillaLayer)
            setPositionX(-getContentWidth());

        Loader::get()->queueInMainThread([self = Ref(this)] {
            PauseLayer* layer = self;
            self->m_fields->m_vanillaLayer->setZOrder(reinterpret_cast<CCScene*>(layer)->getHighestChildZ() + 1);
        });
    }

};