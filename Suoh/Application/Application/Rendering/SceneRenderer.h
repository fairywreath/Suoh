#pragma once

#include <Core/SuohBase.h>
#include <Graphics/RenderLib/Vulkan/VKRenderDevice.h>
#include <Graphics/Scene/Scene.h>

#include "Renderers/CanvasRenderer.h"
#include "Renderers/ClearRenderer.h"
#include "Renderers/CubeRenderer.h"
#include "Renderers/FinishRenderer.h"
#include "Renderers/GuiRenderer.h"
#include "Renderers/ModelRenderer.h"
#include "Renderers/MultiMeshRenderer.h"
#include "Renderers/QuadRenderer.h"

#include "Camera.h"
#include "FramesPerSecondCounter.h"

namespace Suoh
{

class SceneRenderer
{
public:
    SUOH_NON_COPYABLE(SceneRenderer);
    SUOH_NON_MOVEABLE(SceneRenderer);

private:
    Texture mEnvMap;
    Texture mIrrMap;
};

} // namespace Suoh