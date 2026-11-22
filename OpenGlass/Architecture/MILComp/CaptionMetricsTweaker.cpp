#include "pch.h"
#include "Shared.hpp"
#include "CaptionMetricsTweaker.hpp"
#include "CaptionMetrics.hpp"

using namespace OpenGlass;

namespace OpenGlass::CaptionMetricsTweaker
{
	HRESULT MyCTopLevelWindow_UpdateNCAreaPositionsAndSizes(uDWM::CTopLevelWindow* This);
	HRESULT MyCTopLevelWindow_UpdateButtonVisuals(uDWM::CTopLevelWindow* This, const uDWM::CTopLevelWindow::WindowFrame* windowFrame);

	Projection::Detour<uDWM::Symbol_CTopLevelWindow_UpdateNCAreaPositionsAndSizes, &MyCTopLevelWindow_UpdateNCAreaPositionsAndSizes> g_CTopLevelWindow_UpdateNCAreaPositionsAndSizes_Org{};
	Projection::Detour<uDWM::Symbol_CTopLevelWindow_UpdateButtonVisuals, &MyCTopLevelWindow_UpdateButtonVisuals> g_CTopLevelWindow_UpdateButtonVisuals_Org{};
	bool g_legacyCaptionButtonGlyphs{};

	using CaptionMetrics::CaptionButtons;
	CaptionButtons g_captionButtons{ 0 };
}

HRESULT CaptionMetricsTweaker::MyCTopLevelWindow_UpdateButtonVisuals(
	uDWM::CTopLevelWindow* This,
	const uDWM::CTopLevelWindow::WindowFrame* windowFrame
)
{
	if (!g_legacyCaptionButtonGlyphs || !windowFrame)
	{
		return g_CTopLevelWindow_UpdateButtonVisuals_Org(This, windowFrame);
	}

	auto& glyphIndex = This->GetCaptionButtonGlyphIndex();
	const auto nativeIndex = glyphIndex;
	const auto restoreNativeIndex = wil::scope_exit([&glyphIndex, nativeIndex] { glyphIndex = nativeIndex; });
	// Keep the native cache untouched so disabling the flag restores DPI-first selection.
	RETURN_IF_FAILED(SelectLegacyGlyphIndex(This));
	return g_CTopLevelWindow_UpdateButtonVisuals_Org(This, windowFrame);
}

HRESULT CaptionMetricsTweaker::MyCTopLevelWindow_UpdateNCAreaPositionsAndSizes(uDWM::CTopLevelWindow* This)
{
	const auto hr = g_CTopLevelWindow_UpdateNCAreaPositionsAndSizes_Org(This);

	if (!g_captionButtons)
	{
		if (Shared::g_captionHeight.has_value())
		{
			// CButton's SendSetSize is a no-op. Clamp its stored size before native
			// canvas layout updates the button's children; leave its position intact.
			const auto captionHeight = static_cast<LONG>(Shared::g_captionHeight.value() * uDWM::CDesktopManager::GetInstance()->GetDPIValue());
			for (int buttonType = 0; buttonType != 4; ++buttonType)
			{
				if (auto button = This->GetButton(buttonType))
				{
					auto size = button->GetSize();
					size.cy = std::min(size.cy, captionHeight);
					button->SetSize(&size);
				}
			}
		}
		return hr;
	}

	auto data = This->GetData();
	if (!data)
	{
		return hr;
	}

	auto maximized = This->IsWindowMaximized();
	auto toolWindow = This->HasThinRenderedBorder();
	auto loneButton = This->IsLoneButton();

	auto& visibleMargins = This->GetFrameOutsideMargins(maximized);
	auto& borderMargins = This->GetBorderMargins();

	int cxLeft = borderMargins.cxLeftWidth ? borderMargins.cxLeftWidth : data->GetFrameThickness();

	auto UpdateButton = [&](int buttonType, int offsetRight, int offsetTop, SIZE buttonSize)
	{
		if (auto button = This->GetButton(buttonType); button)
		{
			MARGINS inset = { 0x7FFFFFFF, offsetRight, offsetTop, 0x7FFFFFFF };

			button->SetSize(&buttonSize);
			button->SetInsetFromParent(inset);
			button->GetGlyphOpacity() = 1.f;

			return true;
		}
		return false;
	};

	int cySize = GetSystemMetricsForDpi(SM_CYSIZE, data->GetWindowDPI());

	int offsetRight = maximized ? borderMargins.cxRightWidth + 2 : (borderMargins.cxRightWidth ? borderMargins.cxRightWidth - 2 : data->GetFrameThickness() - 2);
	int offsetTop = maximized ? visibleMargins.cyTopHeight - 1 : visibleMargins.cyTopHeight + 1;

	auto closeButtonSize = loneButton ? CaptionMetrics::CalculateButtonSize(g_captionButtons, cySize, 0) : CaptionMetrics::CalculateButtonSize(g_captionButtons, cySize, 3);
	auto maxButtonSize = CaptionMetrics::CalculateButtonSize(g_captionButtons, cySize, 2);
	auto minButtonSize = CaptionMetrics::CalculateButtonSize(g_captionButtons, cySize, 1);

	if (UpdateButton(3, offsetRight, offsetTop, closeButtonSize) && !toolWindow)
	{
		offsetRight = closeButtonSize.cx + offsetRight;
	}

	if (UpdateButton(2, offsetRight, offsetTop, maxButtonSize))
	{
		offsetRight += maxButtonSize.cx;
	}

	if (UpdateButton(1, offsetRight, offsetTop, minButtonSize))
	{
		offsetRight += minButtonSize.cx;
	}

	UpdateButton(0, offsetRight, offsetTop, minButtonSize);

	if (toolWindow)
	{
		int cySmSize = GetSystemMetricsForDpi(SM_CYSMSIZE, data->GetWindowDPI());
		SIZE toolButtonSize = { cySmSize , cySmSize };

		offsetTop = (borderMargins.cyTopHeight - toolButtonSize.cy - 4 > offsetTop) ? borderMargins.cyTopHeight - toolButtonSize.cy - 4 : offsetTop;
		UpdateButton(3, offsetRight, offsetTop, toolButtonSize);
		offsetRight = toolButtonSize.cx + offsetRight;
	}

	if (auto iconVisual = This->GetIconVisual(); iconVisual && cxLeft > 0)
	{
		iconVisual->SetInsetFromParentLeft(cxLeft);
	}

	if (auto dwriteTextVisual = This->GetDWriteTextVisual(); dwriteTextVisual)
	{
		if (auto iconVisual = This->GetIconVisual(); iconVisual && cxLeft > 0)
		{
			cxLeft += iconVisual->GetWidth() ? iconVisual->GetWidth() + 5 : 0;
		}
		MARGINS inset = { cxLeft, offsetRight, visibleMargins.cyTopHeight, 0x7FFFFFFF };
		dwriteTextVisual->SetInsetFromParent(inset);
	}

	return hr;
}

void CaptionMetricsTweaker::Update(GlassEngine::UpdateType type)
{
	if (type & GlassEngine::UpdateType::Theme)
	{
		g_captionButtons = static_cast<CaptionButtons>(GlassEngine::GetDwordFromRegistry(L"CaptionButtons", 0));
		g_legacyCaptionButtonGlyphs = GlassEngine::GetDwordFromRegistry(L"LegacyCaptionButtonGlyphs", 0) != 0;
	}
}

void CaptionMetricsTweaker::Startup()
{
	if (Shared::g_disabledHooks.test(Shared::DisabledHooks_CaptionMetricsTweaker))
	{
		return;
	}

	HookHelper::ApplyInlineHooks(
		std::initializer_list<HookHelper::DetourInfo>
		{
			{ &g_CTopLevelWindow_UpdateNCAreaPositionsAndSizes_Org },
			{ &g_CTopLevelWindow_UpdateButtonVisuals_Org },
		}
	);
}

void CaptionMetricsTweaker::Shutdown()
{
	if (Shared::g_disabledHooks.test(Shared::DisabledHooks_CaptionMetricsTweaker))
	{
		return;
	}

	HookHelper::ApplyInlineHooks(
		std::initializer_list<HookHelper::DetourInfo>
		{
			{ &g_CTopLevelWindow_UpdateNCAreaPositionsAndSizes_Org },
			{ &g_CTopLevelWindow_UpdateButtonVisuals_Org },
		}
	);
}

void CaptionMetricsTweaker::Cleanup()
{
}
