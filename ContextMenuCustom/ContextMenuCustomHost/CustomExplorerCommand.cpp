#include "pch.h"
#include "CustomExplorerCommand.h"
#include "CustomSubExplorerCommand.h"
#include <winrt/base.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <fstream>
#include <ppltasks.h>
#include <shlwapi.h>
#pragma comment(lib, "windowsapp")
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Data::Json;
using namespace std::filesystem;

const EXPCMDSTATE CustomExplorerCommand::State(_In_opt_ IShellItemArray* selection) { return ECS_ENABLED; };

const EXPCMDFLAGS CustomExplorerCommand::Flags() { return ECF_HASSUBCOMMANDS; }

IFACEMETHODIMP CustomExplorerCommand::GetTitle(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* name)
{
	*name = nullptr;
	winrt::hstring title = winrt::unbox_value_or<winrt::hstring>(winrt::Windows::Storage::ApplicationData::Current().LocalSettings().Values().Lookup(L"Custom_Menu_Name"), L"Open With");
	return SHStrDupW(title.data(), name);
}

const  wchar_t* CustomExplorerCommand::GetIconId()
{
	DWORD value = 0;
	DWORD size = sizeof(value);
	auto result = SHRegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"SystemUsesLightTheme", SRRF_RT_DWORD, NULL, &value, &size);
	if (result== ERROR_SUCCESS && !!value) {
		return L",-103";
	}
	else {
		return L",-101";
	}
}

CustomExplorerCommand::CustomExplorerCommand() {
	auto e = Make<CustomeCommands>();
}

IFACEMETHODIMP CustomExplorerCommand::EnumSubCommands(_COM_Outptr_ IEnumExplorerCommand** enumCommands)
{
	*enumCommands = nullptr;
	auto e = Make<CustomeCommands>();
	e->ReadCommands();
	return e->QueryInterface(IID_PPV_ARGS(enumCommands));
}

CustomeCommands::CustomeCommands() {

}

void CustomeCommands::ReadCommands()
{
	auto localFolder = ApplicationData::Current().LocalFolder().Path();
	path localFolderPath{ localFolder.c_str() };
	localFolderPath /= "custom_commands";

	auto task = concurrency::create_task([&]
		{
			if (exists(localFolderPath) && is_directory(localFolderPath)) {
				auto configsFolder = StorageFolder::GetFolderFromPathAsync(localFolderPath.c_str()).get();
				auto files = configsFolder.GetFilesAsync().get();
				for (auto configFile : files) {
					const auto content = FileIO::ReadTextAsync(configFile).get();
					const auto command = Make<CustomSubExplorerCommand>(content);
					m_commands.push_back(command);
				}
			}
		});
	task.wait();
	m_current = m_commands.cbegin();
}
IFACEMETHODIMP CustomeCommands::Next(ULONG celt, __out_ecount_part(celt, *pceltFetched) IExplorerCommand** apUICommand, __out_opt ULONG* pceltFetched)
{
	ULONG fetched{ 0 };
	wil::assign_to_opt_param(pceltFetched, 0ul);

	for (ULONG i = 0; (i < celt) && (m_current != m_commands.cend()); i++)
	{
		m_current->CopyTo(&apUICommand[0]);
		m_current++;
		fetched++;
	}

	wil::assign_to_opt_param(pceltFetched, fetched);
	return (fetched == celt) ? S_OK : S_FALSE;
}
IFACEMETHODIMP CustomeCommands::Skip(ULONG /*celt*/) { return E_NOTIMPL; }
IFACEMETHODIMP CustomeCommands::Reset()
{
	m_current = m_commands.cbegin();
	return S_OK;
}
IFACEMETHODIMP CustomeCommands::Clone(__deref_out IEnumExplorerCommand** ppenum) { *ppenum = nullptr; return E_NOTIMPL; }

