#include "precompiled_header.h"

#include "file_dialog.h"

std::filesystem::path OpenFileDialog(HWND parentWindow, FileDialogOperation operation,
                                     FileDialogTarget target, const wchar_t* fileType /*= nullptr*/,
                                     const wchar_t* fileExtension /*= nullptr*/)
{
    ComPtr<IFileDialog> fileDialog;

    // Can't save folders
    HDRGUI_ASSERT(operation == FileDialogOperation::Load || target == FileDialogTarget::File);

    HRESULT hr;

    switch (operation)
    {
    case FileDialogOperation::Load:
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(&fileDialog));
        CHECK_HR(hr, parentWindow, L"Failed to create FileOpenDialog");

        break;
    case FileDialogOperation::Save:
        hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(&fileDialog));
        CHECK_HR(hr, parentWindow, L"Failed to create FileSaveDialog");
        break;
    default:
        HDRGUI_ASSERT(0);
        return {};
    }

    DWORD dialogOptions;
    hr = fileDialog->GetOptions(&dialogOptions);
    CHECK_HR(hr, parentWindow, L"IFileDialog::GetOptions call failed");

    if (target == FileDialogTarget::Folder)
    {
        hr = fileDialog->SetOptions(dialogOptions | FOS_PICKFOLDERS);
        CHECK_HR(hr, parentWindow, L"IFileDialog::SetOptions call failed");
    }

    if (target == FileDialogTarget::File)
    {
        HDRGUI_ASSERT(fileType != nullptr);
        HDRGUI_ASSERT(fileExtension != nullptr);
        COMDLG_FILTERSPEC rgSpec[] = {{fileType, fileExtension}};
        hr = fileDialog->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
        CHECK_HR(hr, parentWindow, L"IFileDialog::SetFileTypes call failed");
        hr = fileDialog->SetDefaultExtension(fileExtension);
        CHECK_HR(hr, parentWindow, L"IFileDialog::SetDefaultExtension call failed");
        hr = fileDialog->SetOptions(dialogOptions | FOS_OVERWRITEPROMPT | FOS_STRICTFILETYPES);
        CHECK_HR(hr, parentWindow, L"IFileDialog::SetOptions call failed");
    }

    hr = fileDialog->Show(parentWindow);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        return {};
    }
    CHECK_HR(hr, parentWindow, L"IFileDialog::Show call failed");

    ComPtr<IShellItem> dialogItem;
    fileDialog->GetResult(&dialogItem);
    CHECK_HR(hr, parentWindow, L"IFileDialog::GetResult call failed");

    wchar_t* selectedPath;
    hr = dialogItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &selectedPath);
    CHECK_HR(hr, parentWindow, L"IShellItem::GetDisplayName call failed");

    std::filesystem::path result;

    try
    {
        result = selectedPath;
    }
    catch (const std::filesystem::filesystem_error&)
    {
        MessageBoxW(parentWindow, L"Failed parsing selected path", L"Error",
                    MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        ExitProcess(1);
    }

    CoTaskMemFree(selectedPath);
    return result;
}
