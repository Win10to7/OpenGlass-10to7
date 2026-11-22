#include "pch.h"
#include "CaptionMetricsTweaker.hpp"
#include "CaptionButtonGlyphs.hpp"
#include "uDwmProjection.hpp"

using namespace OpenGlass;

HRESULT CaptionMetricsTweaker::SelectLegacyGlyphIndex(uDWM::CTopLevelWindow* window)
{
	const auto* frame = uDWM::CTopLevelWindow::GetWindowFrames()[0];
	RETURN_HR_IF_NULL(E_UNEXPECTED, frame);
	const auto backgrounds = frame->GetCloseButtonBitmaps().views();
	RETURN_HR_IF(E_UNEXPECTED, backgrounds.empty());
	const auto& margins = backgrounds[0]->GetContentMargins();
	const auto verticalPadding = static_cast<std::int64_t>(margins.cyTopHeight) + margins.cyBottomHeight;

	std::array<int, 4> glyphHeights{};
	for (unsigned index = 1; index < glyphHeights.size(); ++index)
	{
		const auto glyphs = frame->GetCloseButtonGlyphBitmaps(index).views();
		RETURN_HR_IF(E_UNEXPECTED, glyphs.empty());
		glyphHeights[index] = glyphs[0]->GetSourceSize().cy;
	}

	const auto captionHeight = GetSystemMetricsForDpi(SM_CYSIZE, window->GetData()->GetWindowDPI());
	window->GetCaptionButtonGlyphIndex() = static_cast<int>(CaptionButtonGlyphs::SelectLegacyIndex(
		captionHeight, verticalPadding, glyphHeights
	));
	return S_OK;
}
