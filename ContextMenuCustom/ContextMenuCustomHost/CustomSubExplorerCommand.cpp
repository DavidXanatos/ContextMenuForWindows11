#include "pch.h"
#include "CustomSubExplorerCommand.h"

using namespace winrt::Windows::Data::Json;

CustomSubExplorerCommand::CustomSubExplorerCommand() {

}

CustomSubExplorerCommand::CustomSubExplorerCommand(winrt::hstring const& configContent) {
	JsonObject result;
	if (JsonObject::TryParse(configContent, result)) {
		_title = result.GetNamedString(L"title",L"Custom Menu");
		_exe = result.GetNamedString(L"exe", L"");
		_param = result.GetNamedString(L"param", L"");
		_icon = result.GetNamedString(L"icon", L"");
		_accept_directory = result.GetNamedBoolean(L"acceptDirectory", false);
		_accept_more_file = result.GetNamedBoolean(L"acceptMoreFile", false);
		_accept_exts = result.GetNamedString(L"acceptExts", L"");
	}
}

 bool CustomSubExplorerCommand::Accept(bool moreFile, bool isDirectory, std::wstring& ext) {
	if (moreFile) {
		return _accept_more_file;
	}

	if (isDirectory) {
		return _accept_directory;
	}

	if (ext.empty()) {
		return true;
	}
	
	if (_accept_exts.empty()) {
		return true;
	}

	if (_accept_exts.find(L"*") != std::wstring::npos) {
		return true;
	}

	return _accept_exts.find(ext) != std::wstring::npos;
}

IFACEMETHODIMP CustomSubExplorerCommand::GetIcon(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* icon)
{
	*icon = nullptr;
	if (!_icon.empty()) {
		return SHStrDupW(_icon.c_str(), icon);
	}
	else {
		return BaseExplorerCommand::GetIcon(items, icon);
	}
}


IFACEMETHODIMP CustomSubExplorerCommand::GetTitle(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* name)
{
	*name = nullptr;
	if (_title.empty()) {
		return SHStrDupW(L"Menu", name);
	}
	else {
		return SHStrDupW(_title.c_str(), name);
	}
}

const EXPCMDSTATE CustomSubExplorerCommand::State(_In_opt_ IShellItemArray* selection) { return ECS_ENABLED; }


static std::wstring string_replace_all(std::wstring src, std::wstring const& target, std::wstring const& repl) {
	if (src.length() == 0 || target.length() == 0) {
		return src;
	}

	size_t idx = 0;

	for (;;) {
		idx = src.find(target, idx);
		if (idx == std::wstring::npos)  break;

		src.replace(idx, target.length(), repl);
		idx += repl.length();
	}

	return src;
}


IFACEMETHODIMP CustomSubExplorerCommand::Invoke(_In_opt_ IShellItemArray* selection, _In_opt_ IBindCtx*) noexcept try
{
	HWND parent = nullptr;
	if (m_site)
	{
		RETURN_IF_FAILED(IUnknown_GetWindow(m_site.Get(), &parent));
	}

	if (selection)
	{
		DWORD count;
		selection->GetCount(&count);

		if (count > 1) {
			auto path = GetPaths(selection);
			if (!path.empty()) {
				auto param = string_replace_all(_param, L"{path}", path);
				ShellExecute(parent, L"open", _exe.c_str(), param.c_str(), nullptr, SW_SHOWNORMAL);
			}
		}
		else if(count >0) {
			wil::unique_cotaskmem_string path = GetPath(selection);

			if (path.is_valid()) {
				std::filesystem::path file(path.get());
				auto param = string_replace_all(_param, L"{path}", file.wstring());
				param = string_replace_all(param, L"{name}", file.filename().wstring());
				ShellExecute(parent, L"open", _exe.c_str(), param.c_str(), nullptr, SW_SHOWNORMAL);
			}
		}

	}

	

	return S_OK;
}
CATCH_RETURN();