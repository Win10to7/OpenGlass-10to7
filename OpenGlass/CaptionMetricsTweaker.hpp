#pragma once
#include "framework.hpp"
#include "cpprt.hpp"
#include "GlassEngine.hpp"

namespace OpenGlass::uDWM
{
	struct CTopLevelWindow;
}

namespace OpenGlass::CaptionMetricsTweaker
{
	HRESULT SelectLegacyGlyphIndex(uDWM::CTopLevelWindow* window);

	void Update(GlassEngine::UpdateType type);
	void Startup();
	void Shutdown();
	void Cleanup();
}
