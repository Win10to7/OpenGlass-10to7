#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace OpenGlass::CaptionButtonGlyphs
{
	[[nodiscard]] inline unsigned SelectLegacyIndex(
		int captionHeight,
		std::int64_t verticalPadding,
		const std::array<int, 4>& glyphHeights
	) noexcept
	{
		// Glyph sizing uses the Windows 7 fit budget, independently of button geometry.
		const float target = std::floor(static_cast<float>(captionHeight) * 0.95238096f + 0.5f);
		const double availableHeight = static_cast<double>(target) - static_cast<double>(verticalPadding);
		unsigned index = 0;
		while (index < 3 && glyphHeights[index + 1] <= availableHeight)
		{
			++index;
		}
		return index;
	}
}
