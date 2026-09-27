#include "pch.h"
#include "MainFrame.hpp"

namespace OpenGlass
{
	const ColorizationPresets::Preset* MainFrame::FindMatchingWindows7Preset(bool opaque) const
	{
		if (!m_config || !m_rbGlassType || m_rbGlassType->GetSelection() != 1)
		{
			return nullptr;
		}

		const DWORD color = ResolveOverridableDword(
			Settings::Id::ColorizationColor,
			Settings::Id::ColorizationColorOverride,
			0xFF000000
		).value;
		const DWORD afterglow = ResolveOverridableDword(
			Settings::Id::ColorizationAfterglow,
			Settings::Id::ColorizationAfterglowOverride,
			0
		).value;
		const DWORD colorBalance = ResolveOverridableDword(
			Settings::Id::ColorizationColorBalance,
			Settings::Id::ColorizationColorBalanceOverride,
			10
		).value;
		const DWORD afterglowBalance = ResolveOverridableDword(
			Settings::Id::ColorizationAfterglowBalance,
			Settings::Id::ColorizationAfterglowBalanceOverride,
			10
		).value;
		const DWORD blurBalance = ResolveOverridableDword(
			Settings::Id::ColorizationBlurBalance,
			Settings::Id::ColorizationBlurBalanceOverride,
			50
		).value;

		for (const auto& preset : ColorizationPresets::Windows7)
		{
			const auto expected = ColorizationPresets::CalculateWindows7Parameters(preset.argb, opaque);
			if (
				color == expected.color
				&& afterglow == expected.afterglow
				&& colorBalance == expected.colorBalance
				&& afterglowBalance == expected.afterglowBalance
				&& blurBalance == expected.blurBalance
			)
			{
				return &preset;
			}
		}

		return nullptr;
	}
}

