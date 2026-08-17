#include "pch.h"
#include "MainFrame.hpp"
#include "WrappingLayout.hpp"
#include "ColorPresetList.hpp"
#include "BlurSettings.hpp"

namespace OpenGlass
{
	void MainFrame::ApplyChoiceColor(wxChoice* ch, wxColourPickerCtrl* cp, DWORD value, DWORD themeSentinel, DWORD autoSentinel) const
	{
		auto dwordToColor = [](DWORD dw) -> wxColour {
			return wxColour(dw & 0xFF, (dw >> 8) & 0xFF, (dw >> 16) & 0xFF, 0xFF);
		};

		if (value == autoSentinel)
		{
			ch->SetSelection(0); // Auto
			cp->Enable(false);
			cp->SetColour(*wxLIGHT_GREY);
		}
		else if (value == themeSentinel)
		{
			ch->SetSelection(1); // Theme
			cp->Enable(false);
			cp->SetColour(*wxLIGHT_GREY);
		}
		else
		{
			ch->SetSelection(2); // Custom
			cp->Enable(true);
			cp->SetColour(dwordToColor(value));
		}
	}

	void MainFrame::ApplyChoiceColorEx(wxChoice* ch, wxColourPickerCtrl* cp, DWORD value, DWORD themeSentinel, DWORD autoSentinel, DWORD systemSentinel) const
	{
		if (value == systemSentinel)
		{
			ch->SetSelection(3); // System determined
			cp->Enable(false);
			cp->SetColour(*wxLIGHT_GREY);
		}
		else
		{
			ApplyChoiceColor(ch, cp, value, themeSentinel, autoSentinel);
		}
	}

	void MainFrame::ApplyChoiceSlider(wxChoice* ch, wxSlider* sl, DWORD value, DWORD themeSentinel, DWORD autoSentinel, int disabledValue) const
	{
		if (value == autoSentinel)
		{
			ch->SetSelection(0); // Auto
			sl->Enable(false);
			sl->SetValue(disabledValue);
		}
		else if (value == themeSentinel)
		{
			ch->SetSelection(1); // Theme
			sl->Enable(false);
			sl->SetValue(disabledValue);
		}
		else
		{
			ch->SetSelection(2); // Custom
			sl->Enable(true);
			sl->SetValue(std::clamp<int>(value, 0, 100));
		}
	}

	bool MainFrame::CanSyncExternalChanges() const
	{
		return !m_isDirty && !m_preview.IsDirty() && !m_colorPreference.IsDirty() && !m_resources.IsDirty()
			&& !m_presetWorkActive && !m_configurationOperationActive
			&& !m_preview.IsAttemptActive() && !m_closeWhenSymbolDownloadStops && !IsBeingDeleted();
	}

	void MainFrame::SyncExternalChanges()
	{
		if (!CanSyncExternalChanges()) return;
		// Read current state without accepting or replacing the preview checkpoint.
		// RegistryConfig opens each read afresh, so no registry cache needs clearing.
		LoadSettings();
		RefreshTransparencyDiagnostics();
		RefreshDwmCrashDumpConfiguration();
		RefreshPresetPackages();
	}

	void MainFrame::LoadSettings()
	{
		// Sync must recheck files even when the stored path has not changed.
		for (auto& warning : m_pathWarnings) warning.initialized = false;
		// System
		m_chkDisableGlassOnBattery->SetValue(m_config->GetDword(L"DisableGlassOnBattery", 1) != 0);

		DWORD hookMask = m_config->GetDword(L"DisabledHooks", 0);
		m_clDisabledHooks->Check(0, (hookMask & 0x1) != 0);
		m_clDisabledHooks->Check(1, (hookMask & 0x2) != 0);
		m_clDisabledHooks->Check(2, (hookMask & 0x4) != 0);
		m_clDisabledHooks->Check(3, (hookMask & 0x8) != 0);
		m_clDisabledHooks->Check(4, (hookMask & 0x10) != 0);
		m_clUxMitigation->Check(0, (m_config->GetDword(L"EnableUxMitigation", UxMitigation::DefaultFlags) & UxMitigation::RestrictAccentOverride) != 0);

		// Theme
		std::wstring atlas = m_config->GetString(L"CustomThemeAtlas", L"");
		m_fpCustomThemeAtlas->SetPath(atlas);
		m_chkCustomThemeAtlas->SetValue(!atlas.empty());
		m_fpCustomThemeAtlas->Enable(!atlas.empty());

		std::wstring reflection = m_config->GetString(L"CustomThemeReflection", L"");
		m_fpCustomThemeReflection->SetPath(reflection);
		m_chkCustomThemeReflection->SetValue(!reflection.empty());
		m_fpCustomThemeReflection->Enable(!reflection.empty());

		auto syncSliderTooltip = [](wxSlider* slider) {
			if (slider)
			{
				slider->SetToolTip(wxString::Format(L"%d", slider->GetValue()));
			}
		};

		m_slReflectionIntensity->SetValue(m_config->GetDword(L"ColorizationGlassReflectionIntensity", 0));
		syncSliderTooltip(m_slReflectionIntensity);

		LoadOpacitySettings(true);

		m_slReflectionParallax->SetValue(m_config->GetDword(L"ColorizationGlassReflectionParallaxIntensity", 13));
		syncSliderTooltip(m_slReflectionParallax);

		DWORD refPolicy = m_config->GetDword(L"ColorizationGlassReflectionPolicy", 0xFFFFFFFF);
		if (refPolicy == 0xFFFFFFFF)
		{
			if (m_chkReflectionPolicyTitlebar) m_chkReflectionPolicyTitlebar->SetValue(true);
			if (m_chkReflectionPolicyPeek) m_chkReflectionPolicyPeek->SetValue(true);
			if (m_chkReflectionPolicySnap) m_chkReflectionPolicySnap->SetValue(true);
		}
		else
		{
			if (m_chkReflectionPolicyTitlebar) m_chkReflectionPolicyTitlebar->SetValue((refPolicy & (1 << 0)) != 0);
			if (m_chkReflectionPolicyPeek) m_chkReflectionPolicyPeek->SetValue((refPolicy & (1 << 2)) != 0);
			if (m_chkReflectionPolicySnap) m_chkReflectionPolicySnap->SetValue((refPolicy & (1 << 3)) != 0);
		}

		std::wstring material = m_config->GetString(L"CustomThemeMaterial", L"");
		m_fpCustomThemeMaterial->SetPath(material);
		m_chkCustomThemeMaterial->SetValue(!material.empty());
		m_fpCustomThemeMaterial->Enable(!material.empty());

		m_slMaterialOpacity->SetValue(m_config->GetDword(L"MaterialOpacity", 0));
		syncSliderTooltip(m_slMaterialOpacity);

		const bool useD3D = m_config->GetDword(L"UseDirect3DRendering", 0) != 0;
		m_chkUseD3D->SetValue(useD3D);

		const DWORD encodedDeviation = m_config->GetDword(L"BlurDeviation", BlurSettings::DefaultEncodedDeviation);
		m_slBlurAmount->SetValue(BlurSettings::DecodeGuiBlurAmount(encodedDeviation));
		m_slBlurAmount->Enable(!useD3D);
		syncSliderTooltip(m_slBlurAmount);

		m_chBlurOptimization->SetSelection(std::clamp<int>(m_config->GetDword(L"BlurOptimization", 0), 0, 2));
		m_chBlurOptimization->Enable(!useD3D);
		m_chkGlassSafetyZone->SetValue(m_config->GetDword(L"GlassSafetyZoneMode", 1) == 0);

		// RoundRect
		DWORD radius = m_config->GetDword(L"RoundRectRadius", 0);
		m_scRoundRectRadius->SetValue(radius);
		if (radius == 0)
			m_chRoundRectProfile->SetSelection(0); // Win8
		else if (radius == 6)
			m_chRoundRectProfile->SetSelection(1); // Win7
		else
			m_chRoundRectProfile->SetSelection(2); // Custom
		m_scRoundRectRadius->Enable(m_chRoundRectProfile->GetSelection() == 2);

		// TextGlow
		DWORD glowVal = m_config->GetDword(L"TextGlowMode", 1);
		int glowMode = glowVal & 0xFFFF;
		int glowSize = (glowVal >> 16) & 0xFFFF;

		m_chTextGlowMode->SetSelection(std::clamp<int>(glowMode, 0, 3));
		m_scTextGlowSize->SetValue(glowSize);
		m_scTextGlowSize->Enable(glowMode == 3);

		m_chCaptionButtons->SetSelection(std::clamp<int>(m_config->GetDword(L"CaptionButtons", 0), 0, 3));
		m_chCenterCaption->SetSelection(std::clamp<int>(m_config->GetDword(L"CenterCaption", 0), 0, 2));
		m_chCaptionTextAliasing->SetSelection(std::clamp<int>(m_config->GetDword(L"CaptionTextAliasing", 0), 0, 1));
		m_chkDisableModernBorders->SetValue(m_config->GetDword(L"DisableModernBorders", 0) != 0);

		// Colors
		auto dwordToColor = [](DWORD dw) -> wxColour {
			return wxColour((dw >> 16) & 0xFF, (dw >> 8) & 0xFF, dw & 0xFF, (dw >> 24) & 0xFF);
		};

		m_rbGlassType->SetSelection(std::clamp<int>(m_config->GetDword(L"GlassType", 0), 0, 1));
		m_chkEnableTransparency->SetValue(m_systemTransparency
			? m_userConfig->GetDword(L"EnableTransparency", 0) != 0
			: m_config->GetDword(L"ColorizationOpaqueBlend", 0) == 0);

		m_slColorIntensity->SetRange(
			ColorizationPresets::ClassicIntensityMinimum,
			ColorizationPresets::ClassicIntensityMaximum
		);
		const DWORD colorIntensity = std::clamp<DWORD>(m_config->GetDword(L"GlassOpacity", 63), ColorizationPresets::ClassicIntensityMinimum, ColorizationPresets::ClassicIntensityMaximum);
		m_slColorIntensity->SetValue(colorIntensity);
		syncSliderTooltip(m_slColorIntensity);
		LoadColorizationBalances();

		LoadCaptionColors();

		// Advanced Blending
		auto loadBaseColor = [&](wxChoice* ch, wxColourPickerCtrl* cp, wxSpinCtrl* sc, const std::wstring& key, DWORD themeSentinel, DWORD autoSentinel) {
			DWORD val = m_config->GetDword(key, autoSentinel);
			if (val == autoSentinel) {
				ch->SetSelection(0);
				cp->Enable(false);
				sc->Enable(false);
				cp->SetColour(*wxLIGHT_GREY);
				sc->SetValue(255);
			} else if (val == themeSentinel) {
				ch->SetSelection(1);
				cp->Enable(false);
				sc->Enable(false);
				cp->SetColour(*wxLIGHT_GREY);
				sc->SetValue(255);
			} else {
				ch->SetSelection(2);
				cp->Enable(true);
				sc->Enable(true);
				cp->SetColour(dwordToColor(val));
				sc->SetValue((val >> 24) & 0xFF);
			}
		};

		loadBaseColor(m_chModeBaseTransparent, m_cpBaseTransparent, m_scBaseTransparentAlpha, L"ColorizationBaseTransparent", 0xFFFFFFFF, 0xFFFFFFFE);
		loadBaseColor(m_chModeBaseMaximized, m_cpBaseMaximized, m_scBaseMaximizedAlpha, L"ColorizationBaseMaximized", 0xFFFFFFFF, 0xFFFFFFFE);
		loadBaseColor(m_chModeBaseOpaque, m_cpBaseOpaque, m_scBaseOpaqueAlpha, L"ColorizationBaseOpaque", 0xFFFFFFFF, 0xFFFFFFFE);

		DWORD priority = m_config->GetDword(L"ColorizationOpaqueBlendPriority", 0xFFFFFFFF);
		m_chOpaqueBlendPriority->SetSelection(priority == 0xFFFFFFFF ? 2 : std::clamp<int>(priority, 0, 1));

		LoadOpacitySettings(false);

		// Accent
		m_chkGlassOverrideAccent->SetValue(m_config->GetDword(L"GlassOverrideAccent", 0) != 0);

		// Update UI State
		UpdateUIVisibility();
	}

	void MainFrame::LoadCaptionColors(bool inheritedOnly)
	{
		auto load = [&](Settings::Id id, wxChoice* choice, wxColourPickerCtrl* picker, DWORD fallback, bool active = false)
		{
			DWORD value{};
			const bool configured = m_config->TryGetDword(std::wstring(Settings::Get(id).name), value);
			if (!configured) value = fallback;
			// An ordinary edit preserves explicit controls, including a new Custom choice.
			if (!inheritedOnly || (!active && !configured))
				ApplyChoiceColorEx(choice, picker, value, 0xFFFFFFFE, 0xFFFFFFFD, 0xFFFFFFFF);
			return value;
		};
		const auto active = load(Settings::Id::ColorizationColorCaption, m_chModeColorCaption, m_cpColorCaption, 0xFFFFFFFD, true);
		const auto inactive = load(Settings::Id::ColorizationColorCaptionInactive, m_chModeColorCaptionInactive, m_cpColorCaptionInactive, active);
		load(Settings::Id::ColorizationColorCaptionMaximized, m_chModeColorCaptionMaximized, m_cpColorCaptionMaximized, active);
		load(Settings::Id::ColorizationColorCaptionInactiveMaximized, m_chModeColorCaptionInactiveMaximized, m_cpColorCaptionInactiveMaximized, inactive);
	}

	void MainFrame::LoadOpacitySettings(bool reflection, bool inheritedOnly)
	{
		auto load = [&](Settings::Id id, wxChoice* choice, wxSlider* slider, DWORD fallback, bool active = false)
		{
			DWORD value{};
			const bool configured = m_config->TryGetDword(std::wstring(Settings::Get(id).name), value);
			if (!configured) value = fallback;
			if (!inheritedOnly || (!active && !configured))
			{
				ApplyChoiceSlider(choice, slider, value, 0xFFFFFFFF, 0xFFFFFFFE, reflection ? 50 : 100);
				slider->SetToolTip(wxString::Format(L"%d", slider->GetValue()));
			}
			return value;
		};
		if (reflection)
		{
			const auto active = load(Settings::Id::ColorizationGlassReflectionOpacity, m_chModeReflectionOpacity, m_slReflectionOpacity, 0xFFFFFFFE, true);
			const auto inactive = load(Settings::Id::ColorizationGlassReflectionOpacityInactive, m_chModeReflectionOpacityInactive, m_slReflectionOpacityInactive, active);
			load(Settings::Id::ColorizationGlassReflectionOpacityMaximized, m_chModeReflectionOpacityMaximized, m_slReflectionOpacityMaximized, active);
			load(Settings::Id::ColorizationGlassReflectionOpacityInactiveMaximized, m_chModeReflectionOpacityInactiveMaximized, m_slReflectionOpacityInactiveMaximized, inactive);
		}
		else
		{
			const auto active = load(Settings::Id::ColorizationOpacity, m_chModeColorizationOpacity, m_slColorizationOpacity, 0xFFFFFFFE, true);
			const auto inactive = load(Settings::Id::ColorizationOpacityInactive, m_chModeColorizationOpacityInactive, m_slColorizationOpacityInactive, active);
			load(Settings::Id::ColorizationOpacityMaximized, m_chModeColorizationOpacityMaximized, m_slColorizationOpacityMaximized, active);
			load(Settings::Id::ColorizationOpacityInactiveMaximized, m_chModeColorizationOpacityInactiveMaximized, m_slColorizationOpacityInactiveMaximized, inactive);
		}
	}

	void MainFrame::LoadColorizationBalances()
	{
		auto load = [this](wxSlider* slider, Settings::Id id, DWORD defaultValue)
		{
			const auto value = std::min<DWORD>(m_config->GetDword(std::wstring(Settings::Get(id).name), defaultValue), 100);
			slider->SetValue(value);
			slider->SetToolTip(wxString::Format(L"%d", slider->GetValue()));
		};
		load(m_slColorBalance, Settings::Id::ColorizationColorBalanceOverride, 10);
		load(m_slAfterglowBalance, Settings::Id::ColorizationAfterglowBalanceOverride, 10);
		load(m_slBlurBalance, Settings::Id::ColorizationBlurBalanceOverride, 50);
	}

	void MainFrame::UpdateUIVisibility()
	{
		// 0 = Vista blur, 1 = Win7 blur
		bool isVista = (m_rbGlassType->GetSelection() == 0);
		m_compositionParametersPane->Show(!isVista);

		if (m_colorPresetList)
		{
			m_colorPresetList->SetFamily(isVista ? ColorizationPresets::Family::Vista : ColorizationPresets::Family::Windows7);
		}

		// IMPORTANT: m_rbGlassType->GetContainingSizer() is the *row* that hosts the radiobox,
		// not the page root sizer. Use the Glass Colors page for visibility changes.
		if (m_glassColorsPanel)
		{
			LayoutWrappingPage(m_glassColorsPanel);
		}

		UpdateOptionStatusIcons();
		UpdatePathWarningIcons();
		UpdateColorizationPresetSelection();
	}
}
