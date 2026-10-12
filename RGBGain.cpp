// RGBGain.cpp - minimal OpenFX plugin: three sliders (R, G, B) that scale each channel.
// Built against the OpenFX Support library (ofxs*).

#include "ofxsImageEffect.h"
#include "ofxsParam.h"
#include <memory>
#include <string>
#include <cstring>

#define kPluginName        "RGB Gain"
#define kPluginGrouping    "MyPlugins"
#define kPluginDescription "Scales the red, green and blue channels independently."
#define kPluginIdentifier  "com.yourname.RGBGain"   // <-- change "yourname"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

#define kParamRed   "red"
#define kParamGreen "green"
#define kParamBlue  "blue"

// Slider limits. 0 = channel removed, 1 = unchanged.
static const double kSliderMax        = 4.0;  // hard maximum (can type values up to this)
static const double kSliderDisplayMax = 2.0;  // maximum shown on the slider itself

////////////////////////////////////////////////////////////////////////////////
// Plugin instance
////////////////////////////////////////////////////////////////////////////////
class RGBGainPlugin : public OFX::ImageEffect
{
protected:
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;
    OFX::DoubleParam *red_;
    OFX::DoubleParam *green_;
    OFX::DoubleParam *blue_;

public:
    explicit RGBGainPlugin(OfxImageEffectHandle handle)
        : OFX::ImageEffect(handle)
        , dstClip_(NULL), srcClip_(NULL)
        , red_(NULL), green_(NULL), blue_(NULL)
    {
        dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
        srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
        red_     = fetchDoubleParam(kParamRed);
        green_   = fetchDoubleParam(kParamGreen);
        blue_    = fetchDoubleParam(kParamBlue);
    }

    virtual void render(const OFX::RenderArguments &args)
    {
        // We only declare float RGBA support, but verify anyway.
        if (dstClip_->getPixelDepth() != OFX::eBitDepthFloat ||
            dstClip_->getPixelComponents() != OFX::ePixelComponentRGBA) {
            OFX::throwSuiteStatusException(kOfxStatErrFormat);
        }

        std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));
        if (!dst.get()) OFX::throwSuiteStatusException(kOfxStatFailed);

        std::unique_ptr<OFX::Image> src(srcClip_->fetchImage(args.time));

        const float r = static_cast<float>(red_->getValueAtTime(args.time));
        const float g = static_cast<float>(green_->getValueAtTime(args.time));
        const float b = static_cast<float>(blue_->getValueAtTime(args.time));

        const OfxRectI win = args.renderWindow;
        const int width = win.x2 - win.x1;

        for (int y = win.y1; y < win.y2; ++y) {
            if (abort()) break;

            float *d = static_cast<float *>(dst->getPixelAddress(win.x1, y));
            const float *s = src.get()
                ? static_cast<const float *>(src->getPixelAddress(win.x1, y))
                : NULL;

            if (!s) {
                // No source pixels available: output transparent black.
                std::memset(d, 0, sizeof(float) * 4 * width);
                continue;
            }

            for (int x = 0; x < width; ++x) {
                d[0] = s[0] * r;
                d[1] = s[1] * g;
                d[2] = s[2] * b;
                d[3] = s[3];       // alpha untouched
                d += 4;
                s += 4;
            }
        }
    }

    // If all sliders are at 1.0 the effect does nothing, so let the host skip it.
    virtual bool isIdentity(const OFX::IsIdentityArguments &args,
                            OFX::Clip *&identityClip,
                            double &identityTime)
    {
        const double r = red_->getValueAtTime(args.time);
        const double g = green_->getValueAtTime(args.time);
        const double b = blue_->getValueAtTime(args.time);
        if (r == 1.0 && g == 1.0 && b == 1.0) {
            identityClip = srcClip_;
            identityTime = args.time;
            return true;
        }
        return false;
    }
};

////////////////////////////////////////////////////////////////////////////////
// Factory: describes the plugin and its UI to the host
////////////////////////////////////////////////////////////////////////////////
static void defineChannelParam(OFX::ImageEffectDescriptor &desc,
                               OFX::PageParamDescriptor *page,
                               const char *name,
                               const char *label,
                               const char *hint)
{
    OFX::DoubleParamDescriptor *p = desc.defineDoubleParam(name);
    p->setLabels(label, label, label);
    p->setHint(hint);
    p->setDefault(1.0);
    p->setRange(0.0, kSliderMax);
    p->setDisplayRange(0.0, kSliderDisplayMax);
    p->setIncrement(0.01);
    p->setDoubleType(OFX::eDoubleTypeScale);
    if (page) page->addChild(*p);
}

class RGBGainFactory : public OFX::PluginFactoryHelper<RGBGainFactory>
{
public:
    RGBGainFactory()
        : OFX::PluginFactoryHelper<RGBGainFactory>(kPluginIdentifier,
                                                   kPluginVersionMajor,
                                                   kPluginVersionMinor) {}

    virtual void load() {}
    virtual void unload() {}

    virtual void describe(OFX::ImageEffectDescriptor &desc)
    {
        desc.setLabels(kPluginName, kPluginName, kPluginName);
        desc.setPluginGrouping(kPluginGrouping);
        desc.setPluginDescription(kPluginDescription);

        desc.addSupportedContext(OFX::eContextFilter);
        desc.addSupportedContext(OFX::eContextGeneral);
        desc.addSupportedBitDepth(OFX::eBitDepthFloat);

        desc.setSingleInstance(false);
        desc.setHostFrameThreading(false);
        desc.setSupportsMultiResolution(true);
        desc.setSupportsTiles(true);
        desc.setTemporalClipAccess(false);
        desc.setRenderTwiceAlways(false);
        desc.setSupportsMultipleClipPARs(false);
    }

    virtual void describeInContext(OFX::ImageEffectDescriptor &desc,
                                   OFX::ContextEnum /*context*/)
    {
        OFX::ClipDescriptor *srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
        srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
        srcClip->setTemporalClipAccess(false);
        srcClip->setSupportsTiles(true);
        srcClip->setIsMask(false);

        OFX::ClipDescriptor *dstClip = desc.defineClip(kOfxImageEffectOutputClipName);
        dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
        dstClip->setSupportsTiles(true);

        OFX::PageParamDescriptor *page = desc.definePageParam("Controls");

        defineChannelParam(desc, page, kParamRed,   "Red",   "Red channel multiplier");
        defineChannelParam(desc, page, kParamGreen, "Green", "Green channel multiplier");
        defineChannelParam(desc, page, kParamBlue,  "Blue",  "Blue channel multiplier");
    }

    virtual OFX::ImageEffect *createInstance(OfxImageEffectHandle handle,
                                             OFX::ContextEnum /*context*/)
    {
        return new RGBGainPlugin(handle);
    }
};

////////////////////////////////////////////////////////////////////////////////
// Registration
////////////////////////////////////////////////////////////////////////////////
namespace OFX {
namespace Plugin {
    void getPluginIDs(OFX::PluginFactoryArray &ids)
    {
        static RGBGainFactory p;
        ids.push_back(&p);
    }
}
}
