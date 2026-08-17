#include "pch.h"
#include "cpprt.hpp"
#include "CaptionMetrics.hpp"

using namespace OpenGlass;

SIZE CaptionMetrics::CalculateButtonSize(CaptionButtons style, int cySize, int buttonType)
{
	enum
	{
		LoneButton = 0,
		MinButton,
		MaxButton,
		CloseButton,
	};

	auto [heightRatio, loneWidthRatio, closeWidthRatio, maxWidthRatio, minWidthRatio] = std::make_tuple(0.f, 0.f, 0.f, 0.f, 0.f);

	switch (style)
	{
	case CaptionButtons::WindowsVista:
		std::tie(heightRatio, loneWidthRatio, closeWidthRatio, maxWidthRatio, minWidthRatio) =
			std::make_tuple(0.94736844f, 2.3157895f, 2.3157895f, 1.3157895f, 1.3684211f);
		break;
	case CaptionButtons::Windows7:
	default:
		std::tie(heightRatio, loneWidthRatio, closeWidthRatio, maxWidthRatio, minWidthRatio) =
			std::make_tuple(0.95238096f, 2.3333333f, 2.3333333f, 1.2857143f, 1.3809524f);
		break;
	case CaptionButtons::Windows8:
		std::tie(heightRatio, loneWidthRatio, closeWidthRatio, maxWidthRatio, minWidthRatio) =
			std::make_tuple(0.95454544f, 1.6363636f, 2.2272727f, 1.2272727f, 1.3181819f);
		break;
	}

	SIZE buttonSize = { 0, static_cast<LONG>(std::round(static_cast<float>(cySize) * heightRatio)) };

	// Vista/Win7 scale widths from caption height; Win8 uses the rounded button height.
	// CTopLevelWindow::UpdateNCAreaButton uses this order in 8102, 8400, 9200 and 9600.
	const float widthBase{ static_cast<float>(style == CaptionButtons::Windows8 ? buttonSize.cy : cySize) };

	switch (buttonType)
	{
	case CloseButton:
		buttonSize.cx = static_cast<LONG>(std::round(widthBase * closeWidthRatio));
		break;
	case MaxButton:
		buttonSize.cx = static_cast<LONG>(std::round(widthBase * maxWidthRatio));
		break;
	case MinButton:
		buttonSize.cx = static_cast<LONG>(std::round(widthBase * minWidthRatio));
		break;
	case LoneButton:
		buttonSize.cx = static_cast<LONG>(std::round(widthBase * loneWidthRatio));
		break;
	default:
		break;
	}

	return buttonSize;
}
