#include "pch.h"
#include "MainFrame.hpp"
#include "WrappingLayout.hpp"
#include "UiControls.hpp"
#include "RegistryValueResolver.hpp"
#include "DwmSettings.hpp"
#include "Elevation.hpp"
#include "resource.h"
#include <wx/iconbndl.h>

namespace OpenGlass
{
	namespace
	{
		class MoreButton final : public wxButton
		{
		public:
			using wxButton::wxButton;

		private:
			bool DoPopupMenu(wxMenu* menu, int x, int y) override
			{
				const auto position = ClientToScreen(wxPoint(x, y));
				// Keep wx's popup wrapper and status-bar help, changing only alignment.
				const auto selected = TrackPopupMenuEx(reinterpret_cast<HMENU>(menu->GetHMenu()),
					TPM_RIGHTBUTTON | TPM_RECURSE | TPM_BOTTOMALIGN | TPM_RETURNCMD,
					position.x, position.y, reinterpret_cast<HWND>(GetHandle()), nullptr);
				if (selected) menu->MSWCommand(0, static_cast<WXWORD>(selected));
				return true;
			}
		};
	}

	std::wstring MainFrame::ResolveAccountName(const std::wstring& sidText, bool includeDomain)
	{
		PSID sid{};
		if (!ConvertStringSidToSidW(sidText.c_str(), &sid)) return {};
		wil::unique_hlocal sidStorage{ sid };
		DWORD nameLength{}, domainLength{};
		SID_NAME_USE use{};
		LookupAccountSidW(nullptr, sid, nullptr, &nameLength, nullptr, &domainLength, &use);
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return {};
		std::wstring name(nameLength, L'\0');
		std::wstring domain(domainLength, L'\0');
		if (!LookupAccountSidW(nullptr, sid, name.data(), &nameLength, domain.data(), &domainLength, &use)) return {};
		if (!name.empty() && name.back() == L'\0') name.pop_back();
		if (!domain.empty() && domain.back() == L'\0') domain.pop_back();
		return !includeDomain || domain.empty() ? name : domain + L"\\" + name;
	}

	MainFrame::MainFrame(std::wstring userSid, Settings::Scope scope, bool systemTransparency)
		: wxFrame(nullptr, wxID_ANY, L"Aero Glass for Win10+", wxDefaultPosition, wxSize(900, 750))
	{
		SetIcons(wxIconBundle(wxString::Format(L"#%d", IDI_OPENGLASS), reinterpret_cast<WXHINSTANCE>(GetModuleHandleW(nullptr))));
		m_isAdmin = Elevation::IsProcessElevated();
		m_editScope = scope;
		m_systemTransparency = systemTransparency;
		if (scope == Settings::Scope::User)
		{
			const auto accountName = ResolveAccountName(userSid, true);
			SetTitle(GetTitle() + L" (HKCU: " + (accountName.empty() ? userSid : accountName) + L")");
		}
		else
		{
			SetTitle(GetTitle() + L" (HKLM)");
		}
		m_baseTitle = GetTitle();
		m_userConfig = std::make_unique<RegistryConfig>(RegistryConfig::Mode::User, userSid);
		m_resources.Initialize(PresetPackages::GetPresetRoot().parent_path() / L"Configuration", userSid);
		m_systemConfig = std::make_unique<RegistryConfig>(RegistryConfig::Mode::Machine, userSid);
		m_config = GetConfigForScope(scope);
		m_targetUserSid = userSid;

		FollowSystemColours(this, wxSYS_COLOUR_FRAMEBK);

		wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
		SetSizer(mainSizer);

		CreateControls();
		CreateBottomControls(mainSizer);
		BindEvents();
		LoadSettings();
		SetDirty(false);

		Centre();
		// Validation may run a modal loop; all controls and event bindings must exist.
		RefreshPresetPackages();
	}

	bool MainFrame::Destroy()
	{
		if (m_configurationOperationActive) return false;
		if (m_presetWorkActive)
		{
			m_presetWorkStop.request_stop();
			return false; // The modal operation must unwind before its owner is destroyed.
		}
		return wxFrame::Destroy();
	}

	void MainFrame::CreateControls()
	{
		m_notebook = new wxNotebook(this, wxID_ANY);

		CreateSystemTab();
		CreateThemeTab();
		CreateAppearanceTab();
		CreateGlassColorsTab();
		CreatePresetsTab();
		CreateDiagnosticsTab();

		m_notebook->SetSelection(2);

		GetSizer()->Add(m_notebook, 1, wxEXPAND | wxALL, 5);
		CreateStatusBar();
	}

	void MainFrame::CreateBottomControls(wxSizer* parentSizer)
	{
		wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);

		auto* more = new MoreButton(this, wxID_ANY, L"More \u25B4");
		more->SetToolTip(L"Merge configuration or restore defaults in the editing scope.");
		more->Bind(wxEVT_BUTTON, [this, more](wxCommandEvent&)
		{
			const bool allowed = m_editScope == Settings::Scope::User || m_isAdmin;
			wxMenu menu;
			auto* merge = menu.Append(wxID_ANY,
				m_editScope == Settings::Scope::User ? L"Merge into HKCU..." : L"Merge into HKLM...",
				!allowed ? L"Requires administrator privileges to modify HKLM."
					: m_isDirty ? L"Save or Revert pending changes before merging."
					: L"Merge the saved effective configuration into the editing scope. Revert can undo this preview.");
			merge->Enable(allowed && !m_isDirty);
			auto* restore = menu.Append(wxID_ANY, L"Restore defaults...",
				!allowed ? L"Requires administrator privileges to modify HKLM."
					: L"Remove this scope's custom settings. Values from the other scope may still apply. Revert can undo this preview.");
			restore->Enable(allowed);
			wxMenuItem* switchView{};
			wxMenuItem* sync{};
			if (wxGetKeyState(WXK_SHIFT))
			{
				menu.AppendSeparator();
				sync = menu.Append(wxID_ANY, L"Sync",
					L"Save or Revert pending changes before reloading external changes into the GUI.");
				sync->Enable(CanSyncExternalChanges());
				switchView = menu.Append(wxID_ANY,
					m_editScope == Settings::Scope::User ? L"Switch to HKLM view" : L"Switch to HKCU view",
					L"Switch editing scope after saving or reverting pending changes and finishing symbol downloads.");
				switchView->Enable(CanSwitchEditingScope());
			}
			const auto selected = more->GetPopupMenuSelectionFromUser(menu, wxPoint(0, 0));
			if (selected == merge->GetId()) MergeConfiguration();
			else if (selected == restore->GetId()) RestoreDefaults();
			else if (sync && selected == sync->GetId()) SyncExternalChanges();
			else if (switchView && selected == switchView->GetId()) SwitchEditingScope();
		});

		m_btnSave = new wxButton(this, wxID_ANY, L"Save");
		m_btnRevert = new wxButton(this, wxID_ANY, L"Revert");
		m_btnSave->Enable(false);
		m_btnRevert->Enable(false);
		m_btnSave->SetToolTip(L"Save changes (Ctrl+S)");
		m_btnRevert->SetToolTip(L"Revert pending changes");

		btnSizer->Add(more, 0);
		btnSizer->AddStretchSpacer();
		btnSizer->Add(m_btnSave, 0, wxRIGHT, 5);
		btnSizer->Add(m_btnRevert, 0);

		parentSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 5);
	}

	bool MainFrame::CanSwitchEditingScope() const
	{
		return !m_isDirty && !m_colorPreference.IsDirty() && !m_preview.IsAttemptActive()
			&& !m_presetWorkActive && !m_configurationOperationActive
			&& !m_symbolDownloadRunning && !m_closeWhenSymbolDownloadStops && !IsBeingDeleted();
	}

	void MainFrame::SwitchEditingScope()
	{
		if (!CanSwitchEditingScope()) return;
		// Constructing the replacement may pump the preset-validation dialog.
		// The original frame must remain alive and unchanged until that returns.
		m_configurationOperationActive = true;
		auto operation = wil::scope_exit([this] { m_configurationOperationActive = false; });
		Enable(false);
		auto enableOnFailure = wil::scope_exit([this] { Enable(); });
		MainFrame* replacement{};
		try
		{
			// Keep the verified original user and the app's single-instance lock.
			const auto scope = m_editScope == Settings::Scope::User ? Settings::Scope::Machine : Settings::Scope::User;
			WINDOWPLACEMENT placement{ sizeof(WINDOWPLACEMENT) };
			THROW_IF_WIN32_BOOL_FALSE(GetWindowPlacement(reinterpret_cast<HWND>(GetHandle()), &placement));
			replacement = new MainFrame(m_targetUserSid.ToStdWstring(), scope, m_systemTransparency);
			// Copy the restore rectangle too, while keeping the new window hidden.
			placement.showCmd = SW_HIDE;
			THROW_IF_WIN32_BOOL_FALSE(SetWindowPlacement(reinterpret_cast<HWND>(replacement->GetHandle()), &placement));
			replacement->m_notebook->ChangeSelection(m_notebook->GetSelection());
			if (IsMaximized()) replacement->Maximize();
		}
		catch (...)
		{
			if (replacement) replacement->Destroy();
			wxMessageBox(wxString::Format(L"The other editing view could not be opened (HRESULT 0x%08lX).",
				static_cast<unsigned long>(wil::ResultFromCaughtException())), L"Switch editing scope", wxOK | wxICON_ERROR, this);
			return;
		}
		m_configurationOperationActive = false;
		operation.release();
		wxTheApp->SetTopWindow(replacement);
		if (!CanSwitchEditingScope() || !Close())
		{
			wxTheApp->SetTopWindow(this);
			replacement->Destroy();
			return;
		}
		enableOnFailure.release();
		replacement->Show();
	}

	void MainFrame::NotifySettingsChange(Settings::UpdateType impact)
	{
		DwmSettings::NotifyChange(FindWindowW(L"Dwm", nullptr), impact);
	}

	void MainFrame::SetDirty(bool dirty)
	{
		auto syncButtons = [this]() {
			if (m_btnSave)
			{
				m_btnSave->Enable(m_isDirty);
			}
			if (m_btnRevert)
			{
				m_btnRevert->Enable(m_isDirty);
			}
		};
		if (m_isDirty == dirty)
		{
			syncButtons();
			return;
		}
		m_isDirty = dirty;
		UpdateWindowTitle();
		syncButtons();
	}

	void MainFrame::UpdateWindowTitle()
	{
		if (m_isDirty)
		{
			SetTitle(L"*" + m_baseTitle);
		}
		else
		{
			SetTitle(m_baseTitle);
		}
	}

	void MainFrame::AddProperty(
		wxWindow* parent,
		wxSizer* sizer,
		const wxString& label,
		wxWindow* control,
		Settings::Id setting
	)
	{
		if (wxSlider* slider = dynamic_cast<wxSlider*>(control))
		{
			slider->SetToolTip(wxString::Format(L"%d", slider->GetValue()));

			// Update tooltip on slider release
			slider->Bind(wxEVT_SLIDER, [slider]([[maybe_unused]] wxCommandEvent& e) {
				slider->SetToolTip(wxString::Format(L"%d", slider->GetValue()));
			});

			// Update tooltip while dragging
			slider->Bind(wxEVT_SCROLL_THUMBTRACK, [slider]([[maybe_unused]] wxScrollEvent& e) {
				slider->SetToolTip(wxString::Format(L"%d", slider->GetValue()));
			});
		}

		wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
		wxStaticText* text = new wxStaticText(parent, wxID_ANY, label, wxDefaultPosition, wxSize(300, -1));
		row->Add(text, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		row->Add(control, 1, wxALIGN_CENTER_VERTICAL);
		AddOptionStatus(parent, row, setting);
		sizer->Add(row, 0, wxEXPAND | wxALL, 2);
	}

	void MainFrame::AddOptionStatus(
		wxWindow* parent,
		wxBoxSizer* row,
		Settings::Id setting
	)
	{
		if (!parent || !row)
		{
			return;
		}

		const wxSize iconSize(16, 16);
		// Bundle dimensions are DIPs; wxStaticBitmap selects native pixels for its current DPI.
		const auto infoBmp = wxArtProvider::GetBitmapBundle(
			m_editScope == Settings::Scope::User ? wxART_INFORMATION : wxART_WARNING, wxART_MESSAGE_BOX, iconSize);
		auto* info = new wxStaticBitmap(parent, wxID_ANY, infoBmp);
		info->SetToolTip(m_editScope == Settings::Scope::User
			? L"Not configured in HKCU; HKLM is inherited at runtime. This control shows the local default and edits HKCU only."
			: L"The current user's HKCU setting takes precedence. This control shows and edits HKLM only.");
		info->Hide();
		row->Add(info, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 2);

		m_optionStatus.push_back({ info, setting });
	}

	void MainFrame::UpdateOptionStatusIcons()
	{
		if (!m_config)
		{
			return;
		}

		std::set<wxWindow*> changedParents;
		for (auto& item : m_optionStatus)
		{
			const auto& spec = Settings::Get(item.setting);
			const std::wstring name(spec.name);
			auto available = [&](const RegistryConfig& config)
			{
				DWORD number{}; std::wstring text;
				return spec.type == Settings::ValueType::Dword
					? config.TryGetDword(name, number) : config.TryGetString(name, text);
			};
			const bool hasUserValue = available(*m_userConfig);
			// HKLM notices depend only on HKCU. HKCU needs HKLM only when local data is absent.
			const auto notice = GetEditorRegistryNotice(m_editScope == Settings::Scope::User,
				hasUserValue, m_editScope == Settings::Scope::User && !hasUserValue && available(*m_systemConfig));
			const bool show = notice != EditorRegistryNotice::None;
			if (item.icon->IsShown() != show)
			{
				item.icon->Show(show);
				changedParents.insert(item.icon->GetParent());
			}
		}

		if (!changedParents.empty())
		{
			for (auto* parent : changedParents) parent->Layout();
			if (m_glassColorsPanel)
			{
				LayoutWrappingPage(m_glassColorsPanel);
			}
			Layout();
			Refresh();
		}
	}

	void MainFrame::AddPathWarningIcon(wxWindow* parent, wxBoxSizer* row, wxFilePickerCtrl* picker, wxCheckBox* checkbox, const wxString& title)
	{
		if (!parent || !row || !picker)
		{
			return;
		}

		const wxSize iconSize(16, 16);
		const auto warnBmp = wxArtProvider::GetBitmapBundle(wxART_WARNING, wxART_MESSAGE_BOX, iconSize);
		auto* warn = new wxStaticBitmap(parent, wxID_ANY, warnBmp);
		warn->SetToolTip(title + L" path does not exist. The value will still be saved.");
		warn->Hide();

		row->Add(warn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 2);

		m_pathWarnings.push_back({ picker, checkbox, warn });
	}

	void MainFrame::UpdatePathWarningIcons()
	{
		bool needLayout = false;
		std::vector<wxWindow*> parents;
		auto addParent = [&parents](wxWindow* parent) {
			if (!parent)
			{
				return;
			}
			if (std::find(parents.begin(), parents.end(), parent) == parents.end())
			{
				parents.push_back(parent);
			}
		};
		for (auto& item : m_pathWarnings)
		{
			if (!item.icon || !item.picker)
			{
				continue;
			}
			const bool enabled = !item.checkbox || item.checkbox->IsChecked();
			const wxString path = item.picker->GetPath();
			if (!item.initialized || enabled != item.lastEnabled || path != item.lastPath)
			{
				item.lastEnabled = enabled;
				item.lastPath = path;
				item.lastExists = enabled && !path.empty() && wxFileExists(path);
				item.initialized = true;
			}
			const bool showWarn = enabled && !path.empty() && !item.lastExists;
			if (item.icon->IsShown() != showWarn)
			{
				item.icon->Show(showWarn);
				needLayout = true;
				addParent(item.icon->GetParent());
			}
		}
		if (needLayout)
		{
			for (wxWindow* parent : parents)
			{
				if (!parent)
				{
					continue;
				}
				parent->Layout();
				if (wxScrolledWindow* scrolled = wxDynamicCast(parent, wxScrolledWindow))
				{
					scrolled->FitInside();
				}
			}
			Layout();
			Refresh();
		}
	}


}
