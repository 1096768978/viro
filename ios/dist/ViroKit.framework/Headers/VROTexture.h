//
//  VROTexture.h
//  ViroRenderer
//
//  Created by Raj Advani on 11/17/15.
//  Copyright © 2015 Viro Media. All rights reserved.
//

#ifndef VROTexture_h
#define VROTexture_h

#include <memory>
#include <vector>
#include "VRODefines.h"

// Constants for ETC2 ripped from NDKr9 headers
#define GL_COMPRESSED_RGB8_ETC2                          0x9274
#define GL_COMPRESSED_RGBA8_ETC2_EAC                     0x9278

class VROTextureSubstrate;
class VRODriver;
class VROImage;
class VROData;
class VROFrameScheduler;

enum class VROTextureType {
    None = 1,
    Texture2D = 2,
    TextureCube = 4,
    TextureEGLImage = 8
};

// Texture formats for source data
enum class VROTextureFormat {
    ETC2_RGBA8_EAC,
    ASTC_4x4_LDR,
    RGBA8,
    RGB565,
};

// Texture formats for storage on the GPU
// (e.g. we can load an RGBA8 texture and store it as RGBA4 to
//       preserve GPU memory)
enum class VROTextureInternalFormat {
    RGBA8,
    RGBA4,
    RGB565,
    YCBCR
};

enum class VROMipmapMode {
    None,          // Do not use mipmaps
    Pregenerated,  // Mipmaps are baked into the texture data
    Runtime,       // Build mipmaps at texture loading time
};

enum class VROStereoMode{
    None = 1,       // No stereo is applied, image is fully represented in the texture.
    LeftRight = 2,  // Side by side stereoscopic image, with the left image shown to the left eye.
    RightLeft = 3,  // Side by side stereoscopic image, with the right image shown to the left eye.
    TopBottom = 4,  // Over/Under stereoscopic image, with the top image shown to the left eye.
    BottomTop = 5   // Over/Under stereoscopic image, with the bottom image shown to the left eye.
};

class VROTexture : public std::enable_shared_from_this<VROTexture> {
    
public:
    
    /*
     Create a new VROTexture with no underlying image data.
     The image data must be injected via setImage*() or setSubstrate().
     */
    VROTexture(VROTextureType type,
               VROTextureInternalFormat internalFormat,
               VROStereoMode stereoMode = VROStereoMode::None);
    
    /*
     Create a new VROTexture with the given underlying substrate.
     */
    VROTexture(VROTextureType type,
               std::unique_ptr<VROTextureSubstrate> substrate,
               VROStereoMode stereoMode = VROStereoMode::None);
    
    /*
     Create a new VROTexture from a VROImage.
     */
    VROTexture(VROTextureInternalFormat internalFormat,
               VROMipmapMode mipmapMode,
               std::shared_ptr<VROImage> image,
               VROStereoMode stereoMode = VROStereoMode::None);

    VROTexture(VROTextureInternalFormat internalFormat,
               std::vector<std::shared_ptr<VROImage>> &images,
               VROStereoMode stereoMode = VROStereoMode::None);
    
    /*
     Create a new VROTexture from the given raw data in the given format.
     */
    VROTexture(VROTextureType type,
               VROTextureFormat format,
               VROTextureInternalFormat internalFormat,
               VROMipmapMode mipmapMode,
               std::vector<std::shared_ptr<VROData>> &data,
               int width, int height,
               std::vector<uint32_t> mipSizes,
               VROStereoMode stereoMode = VROStereoMode::None);
    
    virtual ~VROTexture();
    
    VROTextureType getType() const {
        return _type;
    }
    uint32_t getTextureId() const {
        return _textureId;
    }
    
    /*
     Get the texture ready for usage now, in advance of when it's visible. If not invoked,
     the texture will be initialized when it is made visible.
     */
    void prewarm(std::shared_ptr<VRODriver> driver);
    
    /*
     Get the substrates for this texture, loading them if necessary. If a scheduler is provided,
     then the substrates will be loaded asynchronously via the scheduler; otherwise, the
     substrates will be loaded immediately in a blocking fashion.
     */
    int getNumSubstrates() const;
    VROTextureSubstrate *getSubstrate(int index, std::shared_ptr<VRODriver> &driver, VROFrameScheduler *scheduler);
    
    /*
     Textures may have their substrates set externally if they are created and
     managed elsewhere.
     */
    void setSubstrate(int index, std::unique_ptr<VROTextureSubstrate> substrate);

    VROTextureInternalFormat getInternalFormat() const {
        return _internalFormat;
    }
    VROStereoMode getStereoMode() const {
        return _stereoMode;
    }
    
private:
    
    uint32_t _textureId;
    const VROTextureType _type;
    
    /*
     The image is retained until the texture is hydrated, after which the
     substrate is populated.
     
     Vector of images is used for cube textures.
     */
    std::vector<std::shared_ptr<VROImage>> _images;
    
    /*
     If the underlying texture is compressed, its raw data is retined until the
     substrate is populated. Cube textures contain six faces, all other
     textures will only have one element in this vector.
     */
    std::vector<std::shared_ptr<VROData>> _data;
    
    /*
     The format of the source data (_data).
     */
    VROTextureFormat _format;
    
    /*
     The format in which we want to store the data on the GPU.
     (defaults to RGBA8, and is ignored if we're using a compressed
     source data format: compressed textures are always stored in their
     source format).
     */
    VROTextureInternalFormat _internalFormat;
    int _width, _height;
    
    /*
     The mipmap generation mode for this texture. Determines if mipmaps
     are loaded from the source data (pregenerated), generated at runtime,
     or not used at all. The _mipSizes vector indicates the compessed size
     of each mip-level in the source data, if _mipmapMode = Pregenerated.
     */
    VROMipmapMode _mipmapMode;
    std::vector<uint32_t> _mipSizes;
    
    /*
     Representation of the texture in the underlying hardware. Textures typically
     have one substrate, but some textures (e.g. YCbCr) may have multiple planes,
     each represented by a separate substrate.
     */
    std::vector<std::unique_ptr<VROTextureSubstrate>> _substrates;

    /*
    Represents the stereo property of the image, if any.
    */
    VROStereoMode _stereoMode;

    /*
     Converts the image(s) into a substrate. May be asynchronously executed.
     */
    void hydrate(std::shared_ptr<VRODriver> &driver);
    
    /*
     Set the number of substrates to be used by this texture.
     */
    void setNumSubstrates(int numSubstrates);
    
    /*
     Get the number of substrates used by the given texture format.
     */
    int getNumSubstratesForFormat(VROTextureInternalFormat format) const;
};

#endif /* VROTexture_h */