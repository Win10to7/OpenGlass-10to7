#include "pch.h"
#include "MainFrame.hpp"

namespace OpenGlass
{
	void MainFrame::ApplyColorizationBalances(DWORD argb)
	{
		const auto parameters = ColorizationPresets::CalculateWindows7Parameters(
			argb,
			!m_chkEnableTransparency->IsChecked());
		const std::pair<Settings::Id, DWORD> values[]
		{
			{ Settings::Id::ColorizationColorBalanceOverride, parameters.colorBalance },
			{ Settings::Id::ColorizationAfterglowBalanceOverride, parameters.afterglowBalance },
			{ Settings::Id::ColorizationBlurBalanceOverride, parameters.blurBalance }
		};
		for (const auto& [id, value] : values)
		{
			TrackSettingChange(id);
			const std::wstring name(Settings::Get(id).name);
			THROW_IF_FAILED(m_config->SetDword(name, value));
		}
		LoadColorizationBalances();
	}

	void MainFrame::ApplyAccentColor(std::optional<DWORD> argb)
	{
		THROW_IF_FAILED(m_colorPreference.Apply(m_targetUserSid.ToStdWstring(), argb, [this](const auto& before)
		{
			m_resources.TrackColor(before);
			TrackSettingChange(Settings::Scope::User, Settings::Id::ColorizationColor);
			TrackSettingChange(Settings::Scope::User, Settings::Id::ColorizationAfterglow);
		}));
		const auto baseline = m_colorPreference.Baseline();
		if (!argb && (!baseline || !baseline->applyChoice || baseline->RestoresAutomatic()))
		{
			// Returning to Automatic leaves no fixed RGB customization to undo.
			m_preview.DiscardBaseline({ Settings::Scope::User, Settings::Id::ColorizationColor });
			m_preview.DiscardBaseline({ Settings::Scope::User, Settings::Id::ColorizationAfterglow });
		}
	}

}
