#pragma once

enum class FileDialogOperation
{
    Save,
    Load
};

enum class FileDialogTarget
{
    Folder,
    File
};

std::filesystem::path OpenFileDialog(HWND parentWindow, FileDialogOperation operation,
                                     FileDialogTarget target, const wchar_t* fileType = nullptr,
                                     const wchar_t* fileExtension = nullptr);
