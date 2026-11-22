#include "pch.h"
#include "TestHarness.hpp"
#include "../Common/CaptionButtonGlyphs.hpp"

namespace OpenGlassTests
{
	void TestLegacyCaptionButtonGlyphSelection()
	{
		using OpenGlass::CaptionButtonGlyphs::SelectLegacyIndex;
		constexpr std::array<int, 4> glyphHeights{ 12, 16, 20, 24 };

		Check(SelectLegacyIndex(10, 0, glyphHeights) == 0);
		Check(SelectLegacyIndex(16, 0, glyphHeights) == 0);
		Check(SelectLegacyIndex(17, 0, glyphHeights) == 1);
		Check(SelectLegacyIndex(20, 0, glyphHeights) == 1);
		Check(SelectLegacyIndex(21, 0, glyphHeights) == 2);
		Check(SelectLegacyIndex(26, 0, glyphHeights) == 3);
		Check(SelectLegacyIndex(100, 0, glyphHeights) == 3);

		Check(SelectLegacyIndex(21, 4, glyphHeights) == 1);
		Check(SelectLegacyIndex(21, 5, glyphHeights) == 0);
		constexpr std::array<int, 4> nativeGlyphHeights{ 12, 15, 18, 20 };
		Check(SelectLegacyIndex(21, 6, nativeGlyphHeights) == 0);
		Check(SelectLegacyIndex(22, 6, nativeGlyphHeights) == 1);
		Check(SelectLegacyIndex(25, 6, nativeGlyphHeights) == 2);
		Check(SelectLegacyIndex(27, 6, nativeGlyphHeights) == 3);
		Check(SelectLegacyIndex(100, 2147483647, glyphHeights) == 0);
		Check(SelectLegacyIndex(21, 0, { 12, 16, 40, 18 }) == 1);
	}
}
