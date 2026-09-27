#include "pch.h"
#include "OpenGlassGUI.hpp"
#include "MainFrame.hpp"
#include "Elevation.hpp"
#include "EditorScope.hpp"
#include <wx/cmdline.h>

// IMPLEMENT_APP must be in global scope
IMPLEMENT_APP(OpenGlass::OpenGlassApp)

namespace OpenGlass
{
	void OpenGlassApp::OnInitCmdLine(wxCmdLineParser& parser)
	{
		wxApp::OnInitCmdLine(parser);
		parser.AddLongOption(L"scope", L"Configuration target: hkcu or hklm", wxCMD_LINE_VAL_STRING);
		parser.AddLongOption(
			L"elevated-pipe",
			L"internal elevation handshake pipe",
			wxCMD_LINE_VAL_STRING,
			wxCMD_LINE_HIDDEN
		);
	}

	bool OpenGlassApp::OnInit()
	{
		MSWEnableDarkMode(wxApp::DarkMode_Auto);
		if (!wxApp::OnInit())
			return false;

		std::vector<std::wstring> argumentStorage;
		for (int index = 1; index < argc; ++index) argumentStorage.emplace_back(argv[index]);
		std::vector<std::wstring_view> arguments(argumentStorage.begin(), argumentStorage.end());
		auto scope = Settings::ParseEditorScope(arguments);
		if (!scope)
		{
			wxMessageBox(L"Use --scope=hkcu or --scope=hklm. Conflicting scopes are not allowed.", L"Invalid scope", wxOK | wxICON_ERROR);
			return false;
		}
		if (std::ranges::none_of(arguments, [](std::wstring_view arg) {
			return arg == L"--scope" || arg.starts_with(L"--scope=");
		}))
		{
			wxArrayString scopes;
			scopes.Add(L"Current user (HKCU)");
			scopes.Add(L"Local machine (HKLM)");
			wxSingleChoiceDialog scopeDialog(nullptr, L"Choose the DWM registry hive to edit.", L"Configuration scope", scopes);
			if (scopeDialog.ShowModal() != wxID_OK) return false;
			scope = scopeDialog.GetSelection() == 1 ? Settings::Scope::Machine : Settings::Scope::User;
		}
		const auto startup = Elevation::PrepareElevatedStartup(*scope);
		if (!startup.continueStartup)
		{
			return false;
		}

		// Accent color previews are shared by both editor scopes in this session.
		if (!m_singleInstanceChecker.Create(L"OpenGlassGUI.SingleInstance"))
		{
			wxMessageBox(L"Unable to establish the session editor lock. Close the other editor or retry.", L"OpenGlass", wxOK | wxICON_ERROR);
			return false;
		}
		if (m_singleInstanceChecker.IsAnotherRunning())
			return false;

		MainFrame* frame = new MainFrame(startup.userSid, *scope);
		frame->Show(true);
		return true;
	}
}
