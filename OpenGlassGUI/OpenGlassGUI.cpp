#include "pch.h"
#include "OpenGlassGUI.hpp"
#include "MainFrame.hpp"
#include "Elevation.hpp"
#include <wx/cmdline.h>

// IMPLEMENT_APP must be in global scope
IMPLEMENT_APP(OpenGlass::OpenGlassApp)

namespace OpenGlass
{
	void OpenGlassApp::OnInitCmdLine(wxCmdLineParser& parser)
	{
		wxApp::OnInitCmdLine(parser);
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

		const auto startup = Elevation::PrepareElevatedStartup();
		if (!startup.continueStartup)
		{
			return false;
		}

		// Keep one editor per interactive session so Save/Revert cannot race.
		m_singleInstanceChecker.Create(L"OpenGlassGUI.SingleInstance");
		if (m_singleInstanceChecker.IsAnotherRunning())
			return false;

		MainFrame* frame = new MainFrame(L"Aero Glass for Win10+", startup.userSid);
		if (frame->IsInitializationCanceled())
		{
			frame->Destroy();
			return false;
		}
		frame->Show(true);
		return true;
	}
}
