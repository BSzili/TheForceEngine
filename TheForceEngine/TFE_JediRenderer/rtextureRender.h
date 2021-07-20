#pragma once
//////////////////////////////////////////////////////////////////////
// Wall
// Dark Forces Derived Renderer - Wall functions
//////////////////////////////////////////////////////////////////////
#include <TFE_System/types.h>
#include <TFE_Asset/textureAsset.h>
#include <TFE_Level/core_math.h>

namespace TFE_JediRenderer
{
	TextureFrame* texture_getFrame(Texture* texture, u32 baseFrame = 0);
}
