#include "pch.h"
#include "MainFrame.hpp"

namespace OpenGlass
{
	void MainFrame::ApplyColorizationBalances(DWORD intensity)
	{
		const auto parameters = ColorizationPresets::CalculateWindows7Parameters(
			ColorizationPresets::CalculateIntensityAlpha(intensity) << 24,
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

	WXLRESULT MainFrame::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
	{
		if (message == WM_SETTINGCHANGE && wParam == 0 && lParam
			&& std::wstring_view(reinterpret_cast<const wchar_t*>(lParam)) == L"ImmersiveColorSet")
		{
			if (IsBeingDeleted() || m_systemColorChangePending) return 0;
			// wxWidgets 3.3.3 still refreshes synchronously on every accent broadcast.
			// Merge queued notifications while retaining its normal system-colour handling.
			m_systemColorChangePending = ::PostMessageW(GetHandle(), WM_SYSCOLORCHANGE, 0, 0) != FALSE;
			if (m_systemColorChangePending) return 0;
			// If posting failed, let the original notification take the normal path.
		}
		if (message == WM_SYSCOLORCHANGE) m_systemColorChangePending = false;

		return wxFrame::MSWWindowProc(message, wParam, lParam);
	}

}
