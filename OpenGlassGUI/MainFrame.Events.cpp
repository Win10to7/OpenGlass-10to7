#include "pch.h"
#include "MainFrame.hpp"
#include "BlurSettings.hpp"

namespace OpenGlass
{
	void MainFrame::BindEvents()
	{
		Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);
		Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& e) {
			const int keyCode = e.GetKeyCode();
			if ((e.ControlDown() && (keyCode == 'S' || keyCode == 's')) || keyCode == WXK_F2)
			{
				if (m_isDirty)
				{
					SaveSettings();
					return;
				}
				e.Skip();
				return;
			}
			e.Skip();
		});

		auto updateDword = [this](Settings::Id id, DWORD val) {
			const auto& spec = Settings::Get(id);
			const std::wstring name(spec.name);
			if (spec.type != Settings::ValueType::Dword)
			{
				ReportRegistryError(E_INVALIDARG, name);
				return;
			}
			RegistryConfig* config = m_config;
			if (!config)
			{
				return;
			}
			RunPreview([&]
			{
				TrackSettingChange(id);
				THROW_IF_FAILED(config->SetDword(name, val));
			}, spec.impact);
		};
		auto colorToDwordBgr = [](const wxColour& c) -> DWORD {
			return (c.Red()) | (c.Green() << 8) | (c.Blue() << 16);
		};

		auto syncSliderTooltip = [](wxSlider* slider) {
			if (slider)
			{
				slider->SetToolTip(wxString::Format(L"%d", slider->GetValue()));
			}
		};
		auto setSliderTooltipValue = [](wxSlider* slider, int value) {
			if (slider)
			{
				slider->SetToolTip(wxString::Format(L"%d", value));
			}
		};


		auto updateString = [this](Settings::Id id, const std::wstring& val) {
			const auto& spec = Settings::Get(id);
			const std::wstring name(spec.name);
			if (spec.type != Settings::ValueType::String)
			{
				ReportRegistryError(E_INVALIDARG, name);
				return;
			}
			RegistryConfig* config = m_config;
			if (!config)
			{
				return;
			}
			try
			{
				const auto prepared = !val.empty() && m_resources.NeedsImport(m_editScope, id, val) ? m_resources.PrepareFile(m_editScope, id, val) : nullptr;
				if (RunPreview([&]
				{
					if (prepared) m_resources.Install(*prepared);
					TrackSettingChange(id);
					THROW_IF_FAILED(config->SetString(name, prepared ? m_resources.Path(m_editScope, id).wstring() : val));
				}, spec.impact)) LoadSettings();
			}
			catch (...) { ReportRegistryError(wil::ResultFromCaughtException(), name); }
		};

		auto deleteValue = [this](Settings::Id id) {
			const std::wstring name(Settings::Get(id).name);
			RegistryConfig* config = m_config;
			if (!config)
			{
				return;
			}
			RunPreview([&]
			{
				TrackSettingChange(id);
				THROW_IF_FAILED(config->DeleteValue(name));
			}, Settings::Get(id).impact);
		};


		m_chkDisableGlassOnBattery->Bind(wxEVT_CHECKBOX, [this, updateDword, deleteValue]([[maybe_unused]] wxCommandEvent& e) {
			if (e.IsChecked())
			{
				deleteValue(Settings::Id::DisableGlassOnBattery);
			}
			else
			{
				updateDword(Settings::Id::DisableGlassOnBattery, 0);
			}
		});

		m_clDisabledHooks->Bind(wxEVT_CHECKLISTBOX, [this, updateDword, deleteValue](wxCommandEvent&) {
			DWORD mask = 0;
			if (m_clDisabledHooks->IsChecked(0)) mask |= 0x1;
			if (m_clDisabledHooks->IsChecked(1)) mask |= 0x2;
			if (m_clDisabledHooks->IsChecked(2)) mask |= 0x4;
			if (m_clDisabledHooks->IsChecked(3)) mask |= 0x8;
			if (m_clDisabledHooks->IsChecked(4)) mask |= 0x10;

			if (mask == 0)
			{
				deleteValue(Settings::Id::DisabledHooks);
			}
			else
			{
				updateDword(Settings::Id::DisabledHooks, mask);
			}
		});

		m_clUxMitigation->Bind(wxEVT_CHECKLISTBOX, [this, updateDword](wxCommandEvent&) {
			auto flags = m_config->GetDword(L"EnableUxMitigation", UxMitigation::DefaultFlags);
			flags &= ~UxMitigation::RestrictAccentOverride;
			if (m_clUxMitigation->IsChecked(0)) flags |= UxMitigation::RestrictAccentOverride;
			updateDword(Settings::Id::EnableUxMitigation, flags);
		});

		m_fpCustomThemeAtlas->Bind(wxEVT_FILEPICKER_CHANGED, [this, updateString](wxFileDirPickerEvent& e) {
			if (m_chkCustomThemeAtlas->IsChecked())
			{
				updateString(Settings::Id::CustomThemeAtlas, e.GetPath().ToStdWstring());
			}
			UpdatePathWarningIcons();
		});
		m_chkCustomThemeAtlas->Bind(wxEVT_CHECKBOX, [this, updateString, deleteValue]([[maybe_unused]] wxCommandEvent& e) {
			bool checked = e.IsChecked();
			m_fpCustomThemeAtlas->Enable(checked);
			if (!checked)
			{
				m_fpCustomThemeAtlas->SetPath(wxEmptyString);
				deleteValue(Settings::Id::CustomThemeAtlas);
			}
			else
			{
				if (m_fpCustomThemeAtlas->GetPath().empty())
				{
					deleteValue(Settings::Id::CustomThemeAtlas);
					UpdatePathWarningIcons();
					return;
				}
				updateString(Settings::Id::CustomThemeAtlas, m_fpCustomThemeAtlas->GetPath().ToStdWstring());
			}
			UpdatePathWarningIcons();
		});

		m_fpCustomThemeReflection->Bind(wxEVT_FILEPICKER_CHANGED, [this, updateString](wxFileDirPickerEvent& e) {
			if (m_chkCustomThemeReflection->IsChecked())
			{
				updateString(Settings::Id::CustomThemeReflection, e.GetPath().ToStdWstring());
			}
			UpdatePathWarningIcons();
		});
		m_chkCustomThemeReflection->Bind(wxEVT_CHECKBOX, [this, updateString, deleteValue]([[maybe_unused]] wxCommandEvent& e) {
			bool checked = e.IsChecked();
			m_fpCustomThemeReflection->Enable(checked);
			if (!checked)
			{
				m_fpCustomThemeReflection->SetPath(wxEmptyString);
				deleteValue(Settings::Id::CustomThemeReflection);
			}
			else
			{
				if (m_fpCustomThemeReflection->GetPath().empty())
				{
					deleteValue(Settings::Id::CustomThemeReflection);
					UpdatePathWarningIcons();
					return;
				}
				updateString(Settings::Id::CustomThemeReflection, m_fpCustomThemeReflection->GetPath().ToStdWstring());
			}
			UpdatePathWarningIcons();
		});

		m_slReflectionIntensity->Bind(wxEVT_SLIDER, [this, updateDword, deleteValue, setSliderTooltipValue]([[maybe_unused]] wxCommandEvent& e) {
			int val = e.GetInt();
			if (val == 0)
			{
				deleteValue(Settings::Id::ColorizationGlassReflectionIntensity);
			}
			else
			{
				updateDword(Settings::Id::ColorizationGlassReflectionIntensity, val);
			}
			setSliderTooltipValue(m_slReflectionIntensity, val);
		});

		auto bindOpacity = [&](wxChoice* ch, wxSlider* sl, Settings::Id id, bool reflection) {
			auto update = [this, ch, sl, id, reflection, updateDword, deleteValue]() {
				int sel = ch->GetSelection();
				sl->Enable(sel == 2);
				if (sel == 0) deleteValue(id);
				else if (sel == 1) updateDword(id, 0xFFFFFFFF);
				else updateDword(id, sl->GetValue());
				sl->SetToolTip(wxString::Format(L"%d", sl->GetValue()));
				LoadOpacitySettings(reflection, true);
			};
			ch->Bind(wxEVT_CHOICE, [update](wxCommandEvent&) { update(); });
			sl->Bind(wxEVT_SLIDER, [update](wxCommandEvent&) { update(); });
		};

		bindOpacity(m_chModeReflectionOpacity, m_slReflectionOpacity, Settings::Id::ColorizationGlassReflectionOpacity, true);
		bindOpacity(m_chModeReflectionOpacityInactive, m_slReflectionOpacityInactive, Settings::Id::ColorizationGlassReflectionOpacityInactive, true);
		bindOpacity(m_chModeReflectionOpacityMaximized, m_slReflectionOpacityMaximized, Settings::Id::ColorizationGlassReflectionOpacityMaximized, true);
		bindOpacity(m_chModeReflectionOpacityInactiveMaximized, m_slReflectionOpacityInactiveMaximized, Settings::Id::ColorizationGlassReflectionOpacityInactiveMaximized, true);

		m_slReflectionParallax->Bind(wxEVT_SLIDER, [this, updateDword, deleteValue, setSliderTooltipValue]([[maybe_unused]] wxCommandEvent& e) {
			int val = e.GetInt();
			if (val == 13)
			{
				deleteValue(Settings::Id::ColorizationGlassReflectionParallaxIntensity);
			}
			else
			{
				updateDword(Settings::Id::ColorizationGlassReflectionParallaxIntensity, val);
			}
			setSliderTooltipValue(m_slReflectionParallax, val);
		});

		auto updateReflectionPolicy = [this, updateDword, deleteValue]() {
			DWORD mask = 0;
			if (m_chkReflectionPolicyTitlebar && m_chkReflectionPolicyTitlebar->IsChecked()) mask |= (1 << 0);
			if (m_chkReflectionPolicyPeek && m_chkReflectionPolicyPeek->IsChecked()) mask |= (1 << 2);
			if (m_chkReflectionPolicySnap && m_chkReflectionPolicySnap->IsChecked()) mask |= (1 << 3);
			if ((mask & 0xD) == 0xD)
			{
				deleteValue(Settings::Id::ColorizationGlassReflectionPolicy);
			}
			else
			{
				updateDword(Settings::Id::ColorizationGlassReflectionPolicy, mask);
			}
		};
		if (m_chkReflectionPolicyTitlebar)
		{
			m_chkReflectionPolicyTitlebar->Bind(wxEVT_CHECKBOX, [updateReflectionPolicy](wxCommandEvent&) { updateReflectionPolicy(); });
		}
		if (m_chkReflectionPolicyPeek)
		{
			m_chkReflectionPolicyPeek->Bind(wxEVT_CHECKBOX, [updateReflectionPolicy](wxCommandEvent&) { updateReflectionPolicy(); });
		}
		if (m_chkReflectionPolicySnap)
		{
			m_chkReflectionPolicySnap->Bind(wxEVT_CHECKBOX, [updateReflectionPolicy](wxCommandEvent&) { updateReflectionPolicy(); });
		}

		m_fpCustomThemeMaterial->Bind(wxEVT_FILEPICKER_CHANGED, [this, updateString](wxFileDirPickerEvent& e) {
			if (m_chkCustomThemeMaterial->IsChecked())
			{
				updateString(Settings::Id::CustomThemeMaterial, e.GetPath().ToStdWstring());
			}
			UpdatePathWarningIcons();
		});
		m_chkCustomThemeMaterial->Bind(wxEVT_CHECKBOX, [this, updateString, deleteValue]([[maybe_unused]] wxCommandEvent& e) {
			bool checked = e.IsChecked();
			m_fpCustomThemeMaterial->Enable(checked);
			if (!checked)
			{
				m_fpCustomThemeMaterial->SetPath(wxEmptyString);
				deleteValue(Settings::Id::CustomThemeMaterial);
			}
			else
			{
				if (m_fpCustomThemeMaterial->GetPath().empty())
				{
					deleteValue(Settings::Id::CustomThemeMaterial);
					UpdatePathWarningIcons();
					return;
				}
				updateString(Settings::Id::CustomThemeMaterial, m_fpCustomThemeMaterial->GetPath().ToStdWstring());
			}
			UpdatePathWarningIcons();
		});

		m_slMaterialOpacity->Bind(wxEVT_SLIDER, [this, updateDword, deleteValue, setSliderTooltipValue]([[maybe_unused]] wxCommandEvent& e) {
			int val = e.GetInt();
			if (val == 0)
			{
				deleteValue(Settings::Id::MaterialOpacity);
			}
			else
			{
				updateDword(Settings::Id::MaterialOpacity, val);
			}
			setSliderTooltipValue(m_slMaterialOpacity, val);
		});
		m_slBlurAmount->Bind(wxEVT_SLIDER, [this, updateDword, deleteValue, setSliderTooltipValue]([[maybe_unused]] wxCommandEvent& e) {
			const DWORD encodedDeviation = BlurSettings::EncodeGuiBlurAmount(e.GetInt());
			if (encodedDeviation == BlurSettings::DefaultEncodedDeviation)
			{
				deleteValue(Settings::Id::BlurDeviation);
			}
			else
			{
				updateDword(Settings::Id::BlurDeviation, encodedDeviation);
			}
			setSliderTooltipValue(m_slBlurAmount, e.GetInt());
		});
		m_chBlurOptimization->Bind(wxEVT_CHOICE, [this, updateDword, deleteValue]([[maybe_unused]] wxCommandEvent& e) {
			int sel = e.GetSelection();
			if (sel == 0)
			{
				deleteValue(Settings::Id::BlurOptimization);
			}
			else
			{
				updateDword(Settings::Id::BlurOptimization, sel);
			}
		});
		auto updateD3DControls = [this]() {
			const bool enabled = !m_chkUseD3D->IsChecked();
			const DWORD encodedDeviation = m_config->GetDword(L"BlurDeviation", BlurSettings::DefaultEncodedDeviation);
			m_slBlurAmount->SetValue(BlurSettings::DecodeGuiBlurAmount(encodedDeviation));
			m_slBlurAmount->Enable(enabled);
			m_slBlurAmount->SetToolTip(wxString::Format(L"%d", m_slBlurAmount->GetValue()));

			const int blurOptimization = std::clamp<int>(m_config->GetDword(L"BlurOptimization", 0), 0, 2);
			m_chBlurOptimization->SetSelection(blurOptimization);
			m_chBlurOptimization->Enable(enabled);
		};

		m_chkUseD3D->Bind(wxEVT_CHECKBOX, [this, updateDword, updateD3DControls, deleteValue]([[maybe_unused]] wxCommandEvent& e) {
			bool checked = e.IsChecked();
			if (!checked)
			{
				deleteValue(Settings::Id::UseDirect3DRendering);
			}
			else
			{
				updateDword(Settings::Id::UseDirect3DRendering, 1);
			}
			updateD3DControls();
		});
		m_chkGlassSafetyZone->Bind(wxEVT_CHECKBOX, [this, updateDword, deleteValue]([[maybe_unused]] wxCommandEvent& e) {
			bool checked = e.IsChecked();
			if (checked)
			{
				updateDword(Settings::Id::GlassSafetyZoneMode, 0);
			}
			else
			{
				deleteValue(Settings::Id::GlassSafetyZoneMode);
			}
		});

		m_chRoundRectProfile->Bind(wxEVT_CHOICE, [this, updateDword, deleteValue]([[maybe_unused]] wxCommandEvent& e) {
			int sel = e.GetSelection();
			if (sel == 0)
			{
				m_scRoundRectRadius->SetValue(0);
				m_scRoundRectRadius->Disable();
				deleteValue(Settings::Id::RoundRectRadius);
			}
			else if (sel == 1)
			{
				m_scRoundRectRadius->SetValue(6);
				m_scRoundRectRadius->Disable();
				updateDword(Settings::Id::RoundRectRadius, 6);
			}
			else
			{
				m_scRoundRectRadius->Enable();
				DWORD r = m_scRoundRectRadius->GetValue();
				if (r == 0)
				{
					deleteValue(Settings::Id::RoundRectRadius);
				}
				else
				{
					updateDword(Settings::Id::RoundRectRadius, r);
				}
			}
		});
		m_scRoundRectRadius->Bind(wxEVT_SPINCTRL, [this, updateDword, deleteValue](wxSpinEvent& e) {
			DWORD val = e.GetPosition();
			if (val == 0)
			{
				deleteValue(Settings::Id::RoundRectRadius);
			}
			else
			{
				updateDword(Settings::Id::RoundRectRadius, val);
			}
		});

		auto updateGlow = [this, updateDword, deleteValue]() {
			int mode = m_chTextGlowMode->GetSelection();
			int size = m_scTextGlowSize->GetValue();
			DWORD val = (DWORD)mode | ((DWORD)size << 16);
			if (val == 1)
			{
				deleteValue(Settings::Id::TextGlowMode);
			}
			else
			{
				updateDword(Settings::Id::TextGlowMode, val);
			}
		};

		m_chTextGlowMode->Bind(wxEVT_CHOICE, [this, updateGlow]([[maybe_unused]] wxCommandEvent& e) {
			m_scTextGlowSize->Enable(e.GetSelection() == 3);
			updateGlow();
		});
		m_scTextGlowSize->Bind(wxEVT_SPINCTRL, [this, updateGlow](wxSpinEvent&) {
			updateGlow();
		});

		m_chCaptionButtons->Bind(wxEVT_CHOICE, [this, updateDword, deleteValue]([[maybe_unused]] wxCommandEvent& e) {
			int sel = e.GetSelection();
			if (sel == 0)
			{
				deleteValue(Settings::Id::CaptionButtons);
			}
			else
			{
				updateDword(Settings::Id::CaptionButtons, sel);
			}
		});
		m_chCenterCaption->Bind(wxEVT_CHOICE, [this, updateDword, deleteValue]([[maybe_unused]] wxCommandEvent& e) {
			int sel = e.GetSelection();
			if (sel == 0)
			{
				deleteValue(Settings::Id::CenterCaption);
			}
			else
			{
				updateDword(Settings::Id::CenterCaption, sel);
			}
		});
		m_chCaptionTextAliasing->Bind(wxEVT_CHOICE, [updateDword, deleteValue](wxCommandEvent& e) {
			if (e.GetSelection() == 0)
			{
				deleteValue(Settings::Id::CaptionTextAliasing);
			}
			else
			{
				updateDword(Settings::Id::CaptionTextAliasing, e.GetSelection());
			}
		});
		m_chkDisableModernBorders->Bind(wxEVT_CHECKBOX, [this, updateDword, deleteValue]([[maybe_unused]] wxCommandEvent& e) {
			if (!e.IsChecked())
			{
				deleteValue(Settings::Id::DisableModernBorders);
			}
			else
			{
				updateDword(Settings::Id::DisableModernBorders, 1);
			}
		});

		m_rbGlassType->Bind(wxEVT_RADIOBOX, [this, updateDword, deleteValue]([[maybe_unused]] wxCommandEvent& e) {
			int sel = e.GetSelection();
			if (sel == 0)
			{
				deleteValue(Settings::Id::GlassType);
			}
			else
			{
				updateDword(Settings::Id::GlassType, sel);
			}
			LoadSettings();
		});

		auto bindColorOverride = [this, updateDword](wxColourPickerCtrl* picker, Settings::Id id, Settings::Id baseId)
		{
			picker->Bind(wxEVT_COLOURPICKER_CHANGED, [this, updateDword, id, baseId](wxColourPickerEvent& event)
			{
				const auto& color = event.GetColour();
				const DWORD current = m_config->GetDword(std::wstring(Settings::Get(id).name),
					m_config->GetDword(std::wstring(Settings::Get(baseId).name), 0xFF000000));
				updateDword(id, (current & 0xFF000000)
					| (static_cast<DWORD>(color.Red()) << 16)
					| (static_cast<DWORD>(color.Green()) << 8)
					| static_cast<DWORD>(color.Blue()));
			});
		};
		bindColorOverride(m_cpColorizationColor, Settings::Id::ColorizationColorOverride, Settings::Id::ColorizationColor);
		bindColorOverride(m_cpAfterglow, Settings::Id::ColorizationAfterglowOverride, Settings::Id::ColorizationAfterglow);

		m_chkEnableTransparency->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
			if (RunPreview([&]
			{
				if (m_systemTransparency)
				{
					if ((m_userConfig->GetDword(L"EnableTransparency", 0) != 0) != e.IsChecked())
					{
						TrackSettingChange(Settings::Scope::User, Settings::Id::EnableTransparency);
						THROW_IF_FAILED(m_userConfig->SetDword(L"EnableTransparency", e.IsChecked()));
					}
				}
				else
				{
					TrackSettingChange(Settings::Id::ColorizationOpaqueBlend);
					if (e.IsChecked()) THROW_IF_FAILED(m_config->DeleteValue(L"ColorizationOpaqueBlend"));
					else THROW_IF_FAILED(m_config->SetDword(L"ColorizationOpaqueBlend", 1));
				}
				if (m_rbGlassType->GetSelection() == 1)
					ApplyColorizationBalances(ColorizationPresets::CalculateIntensityAlpha(m_slColorIntensity->GetValue()) << 24);
			}, Settings::UpdateType::Backdrop)) LoadSettings();
		});

		m_slColorIntensity->Bind(wxEVT_SLIDER, [this](wxCommandEvent& e) {
			if (RunPreview([&]
			{
				const DWORD intensity = static_cast<DWORD>(e.GetInt());
				TrackSettingChange(Settings::Id::GlassOpacity);
				THROW_IF_FAILED(m_config->SetDword(L"GlassOpacity", intensity));
				if (m_rbGlassType->GetSelection() == 1)
					ApplyColorizationBalances(ColorizationPresets::CalculateIntensityAlpha(intensity) << 24);
			}, Settings::UpdateType::Backdrop)) LoadSettings();
			m_slColorIntensity->SetToolTip(wxString::Format(L"%d", m_slColorIntensity->GetValue()));
		});

		for (const auto& [slider, id] : {
			std::pair{ m_slColorBalance, Settings::Id::ColorizationColorBalanceOverride },
			std::pair{ m_slAfterglowBalance, Settings::Id::ColorizationAfterglowBalanceOverride },
			std::pair{ m_slBlurBalance, Settings::Id::ColorizationBlurBalanceOverride } })
		{
			slider->Bind(wxEVT_SLIDER, [slider, id, updateDword, syncSliderTooltip](wxCommandEvent& event)
			{
				updateDword(id, event.GetInt());
				syncSliderTooltip(slider);
			});
		}

		auto bindChoiceColorEx = [&](wxChoice* ch, wxColourPickerCtrl* cp, Settings::Id id, DWORD themeSentinel, DWORD systemSentinel) {
			auto update = [this, ch, cp, id, themeSentinel, systemSentinel, updateDword, deleteValue, colorToDwordBgr]() {
				int sel = ch->GetSelection();
				cp->Enable(sel == 2);
				if (sel == 0) deleteValue(id);
				else if (sel == 1) updateDword(id, themeSentinel);
				else if (sel == 3) updateDword(id, systemSentinel);
				else updateDword(id, colorToDwordBgr(cp->GetColour()));
				LoadCaptionColors(true);
			};
			ch->Bind(wxEVT_CHOICE, [update](wxCommandEvent&) { update(); });
			cp->Bind(wxEVT_COLOURPICKER_CHANGED, [update](wxColourPickerEvent&) { update(); });
		};

		bindChoiceColorEx(m_chModeColorCaption, m_cpColorCaption, Settings::Id::ColorizationColorCaption, 0xFFFFFFFE, 0xFFFFFFFF);
		bindChoiceColorEx(m_chModeColorCaptionInactive, m_cpColorCaptionInactive, Settings::Id::ColorizationColorCaptionInactive, 0xFFFFFFFE, 0xFFFFFFFF);
		bindChoiceColorEx(m_chModeColorCaptionMaximized, m_cpColorCaptionMaximized, Settings::Id::ColorizationColorCaptionMaximized, 0xFFFFFFFE, 0xFFFFFFFF);
		bindChoiceColorEx(m_chModeColorCaptionInactiveMaximized, m_cpColorCaptionInactiveMaximized, Settings::Id::ColorizationColorCaptionInactiveMaximized, 0xFFFFFFFE, 0xFFFFFFFF);

		auto bindBaseColor = [updateDword, deleteValue](wxChoice* choice, wxColourPickerCtrl* picker, wxSpinCtrl* alphaSpin, Settings::Id id, DWORD themeVal) {
			auto update = [choice, picker, alphaSpin, id, themeVal, updateDword, deleteValue]() {
				int sel = choice->GetSelection();
				if (sel == 0)
				{
					picker->Disable();
					alphaSpin->Disable();
					deleteValue(id);
				}
				else if (sel == 1)
				{
					picker->Disable();
					alphaSpin->Disable();
					updateDword(id, themeVal);
				}
				else
				{
					picker->Enable();
					alphaSpin->Enable();
					wxColour c = picker->GetColour();
					int a = alphaSpin->GetValue();
					DWORD val = (a << 24) | (c.Red() << 16) | (c.Green() << 8) | c.Blue();
					updateDword(id, val);
				}
			};

			choice->Bind(wxEVT_CHOICE, [update](wxCommandEvent&) { update(); });
			picker->Bind(wxEVT_COLOURPICKER_CHANGED, [update](wxColourPickerEvent&) { update(); });
			alphaSpin->Bind(wxEVT_SPINCTRL, [update](wxSpinEvent&) { update(); });
			alphaSpin->Bind(wxEVT_TEXT, [update](wxCommandEvent&) { update(); });
		};

		bindBaseColor(m_chModeBaseTransparent, m_cpBaseTransparent, m_scBaseTransparentAlpha, Settings::Id::ColorizationBaseTransparent, 0xFFFFFFFF);
		bindBaseColor(m_chModeBaseMaximized, m_cpBaseMaximized, m_scBaseMaximizedAlpha, Settings::Id::ColorizationBaseMaximized, 0xFFFFFFFF);
		bindBaseColor(m_chModeBaseOpaque, m_cpBaseOpaque, m_scBaseOpaqueAlpha, Settings::Id::ColorizationBaseOpaque, 0xFFFFFFFF);

		m_chOpaqueBlendPriority->Bind(wxEVT_CHOICE, [this, updateDword, deleteValue](wxCommandEvent& e) {
			int sel = e.GetSelection();
			if (sel == 2)
			{
				deleteValue(Settings::Id::ColorizationOpaqueBlendPriority);
			}
			else
			{
				updateDword(Settings::Id::ColorizationOpaqueBlendPriority, sel);
			}
		});


		bindOpacity(m_chModeColorizationOpacity, m_slColorizationOpacity, Settings::Id::ColorizationOpacity, false);
		bindOpacity(m_chModeColorizationOpacityInactive, m_slColorizationOpacityInactive, Settings::Id::ColorizationOpacityInactive, false);
		bindOpacity(m_chModeColorizationOpacityMaximized, m_slColorizationOpacityMaximized, Settings::Id::ColorizationOpacityMaximized, false);
		bindOpacity(m_chModeColorizationOpacityInactiveMaximized, m_slColorizationOpacityInactiveMaximized, Settings::Id::ColorizationOpacityInactiveMaximized, false);

		m_chkGlassOverrideAccent->Bind(wxEVT_CHECKBOX, [this, updateDword, deleteValue](wxCommandEvent& e) {
			if (!e.IsChecked())
			{
				deleteValue(Settings::Id::GlassOverrideAccent);
			}
			else
			{
				updateDword(Settings::Id::GlassOverrideAccent, 1);
			}
		});

		m_btnExportAtlas->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
			wxFileDialog saveDialog(this, L"Save atlas file", L"", L"theme.png", L"PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
			if (saveDialog.ShowModal() != wxID_OK) return;

			WCHAR themeFileName[MAX_PATH]{};
			if (FAILED(GetCurrentThemeName(themeFileName, MAX_PATH, nullptr, 0, nullptr, 0)))
			{
				wxMessageBox(L"Failed to get current system theme name.", L"Export Failed", wxICON_ERROR);
				return;
			}

			wil::unique_hmodule themeResource{ LoadLibraryExW(themeFileName, nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_SEARCH_SYSTEM32) };
			if (!themeResource)
			{
				wxMessageBox(L"Failed to load msstyle.", L"Export Failed", wxICON_ERROR);
				return;
			}

			HTHEME hTheme = OpenThemeData(nullptr, L"DWMWindow");
			if (!hTheme)
			{
				wxMessageBox(L"Failed to open DWMWindow theme data.", L"Export Failed", wxICON_ERROR);
				return;
			}
			auto closeTheme = wil::scope_exit([&] { CloseThemeData(hTheme); });

			VOID* streamAddress = nullptr;
			DWORD streamSize = 0;

			if (FAILED(GetThemeStream(hTheme, 0, 0, TMT_DISKSTREAM, &streamAddress, &streamSize, themeResource.get())))
			{
				wxMessageBox(L"Failed to retrieve theme stream (Atlas).", L"Export Failed", wxICON_ERROR);
				return;
			}

			if (streamSize == 0 || !streamAddress)
			{
				wxMessageBox(L"Retrieved atlas stream is empty.", L"Export Failed", wxICON_ERROR);
				return;
			}

			{
				wil::unique_hfile file{ CreateFileW(saveDialog.GetPath().wc_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr) };
				if (!file)
				{
					wxMessageBox(L"Failed to create output file.", L"Export Failed", wxICON_ERROR);
					return;
				}

				DWORD bytesWritten = 0;
				if (!WriteFile(file.get(), streamAddress, streamSize, &bytesWritten, nullptr) || bytesWritten != streamSize)
				{
					wxMessageBox(L"Failed to write all data to file.", L"Export Failed", wxICON_ERROR);
					return;
				}
			}

			wxString layoutPath = saveDialog.GetPath() + L".layout";
			wil::unique_hfile layoutFile{ CreateFileW(layoutPath.wc_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr) };

			if (layoutFile)
			{
				std::string layoutContent;
				char buffer[256];

				layoutContent += "# Rects\n";
				layoutContent += "# 8002 = TMT_ATLASRECT\n";
				for (int iPartId = 1; iPartId < 100; ++iPartId)
				{
					RECT rc;
					if (SUCCEEDED(GetThemeRect(hTheme, iPartId, 0, TMT_ATLASRECT, &rc)))
					{
						int len = sprintf_s(buffer, "%u;0;8002=%ld,%ld,%ld,%ld\n",
							iPartId, rc.left, rc.top, rc.right, rc.bottom);
						layoutContent.append(buffer, len);
					}
				}

				layoutContent += "\n# Margins\n";
				layoutContent += "# 3601 = TMT_SIZINGMARGINS\n";
				layoutContent += "# 3602 = TMT_CONTENTMARGINS\n";
				for (int i = 1; i < 100; ++i)
				{
					MARGINS mg;
					if (SUCCEEDED(GetThemeMargins(hTheme, nullptr, i, 0, TMT_SIZINGMARGINS, nullptr, &mg)))
					{
						int len = sprintf_s(buffer, "%u;0;3601=%d,%d,%d,%d\n",
							i, mg.cxLeftWidth, mg.cxRightWidth, mg.cyTopHeight, mg.cyBottomHeight);
						layoutContent.append(buffer, len);
					}

					if (SUCCEEDED(GetThemeMargins(hTheme, nullptr, i, 0, TMT_CONTENTMARGINS, nullptr, &mg)))
					{
						int len = sprintf_s(buffer, "%u;0;3602=%d,%d,%d,%d\n",
							i, mg.cxLeftWidth, mg.cxRightWidth, mg.cyTopHeight, mg.cyBottomHeight);
						layoutContent.append(buffer, len);
					}
				}

				if (!layoutContent.empty())
				{
					DWORD bytesWritten = 0;
					WriteFile(layoutFile.get(), layoutContent.c_str(), static_cast<DWORD>(layoutContent.size()), &bytesWritten, nullptr);
				}
			}

		});

		m_btnDownloadSymbols->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
			StartSymbolDownload();
		});

		m_btnRefreshTransparencyDiagnostics->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
			RefreshTransparencyDiagnostics();
		});

		m_btnCancelSymbolDownload->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
			if (!m_symbolDownloadRunning)
			{
				return;
			}

			m_symbolDownloadThread.request_stop();
			m_btnCancelSymbolDownload->Enable(false);
			UpdateSymbolDownloadProgress(SymbolDownloadProgress{
				m_gaugeSymbolDownload ? m_gaugeSymbolDownload->GetValue() : 0,
				true,
				L"Cancelling symbol download...",
				L"Waiting for the current network operation to stop."
			});
		});

		m_btnEnableDwmCrashDumps->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
			SetDwmCrashDumpsEnabled(true);
		});

		m_btnDisableDwmCrashDumps->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
			SetDwmCrashDumpsEnabled(false);
		});

		m_btnSave->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
			SaveSettings();
		});

		m_btnRevert->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
			RevertSettings();
		});
	}

	void MainFrame::OnClose(wxCloseEvent& event)
	{
		if (m_configurationOperationActive)
		{
			if (event.CanVeto()) event.Veto();
			return;
		}
		if (m_presetWorkActive)
		{
			m_presetWorkStop.request_stop();
			if (event.CanVeto()) event.Veto();
			return;
		}
		if (!m_config)
		{
			event.Skip();
			return;
		}
		if (m_symbolDownloadRunning)
		{
			m_closeWhenSymbolDownloadStops = true;
			m_symbolDownloadThread.request_stop();
			if (m_btnCancelSymbolDownload)
			{
				m_btnCancelSymbolDownload->Enable(false);
			}
			UpdateSymbolDownloadProgress(SymbolDownloadProgress{
				m_gaugeSymbolDownload ? m_gaugeSymbolDownload->GetValue() : 0,
				true,
				L"Cancelling symbol download before closing...",
				L"The window will close after the download has stopped."
			});
			if (event.CanVeto()) event.Veto();
			return;
		}
		if ((m_isDirty || m_colorPreference.IsDirty()) && !RevertSettings())
		{
			if (event.CanVeto()) event.Veto();
			return;
		}
		event.Skip();
	}
}
