/*
    HDR Display Read GUI
    Copyright (C) 2026  Dmytro Bulatov

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    My contacts are on https://boolka.dev/
*/

#pragma once

class Subprocess
{
public:
    Subprocess() = default;
    ~Subprocess();

    void Start(std::wstring& commandLine,
               const std::vector<std::pair<std::string, std::string>>& environment);
    bool IsRunning();
    void Release();

    int Read(char* buffer, DWORD size);
    void Write(const char* buffer, DWORD size);

    bool Join(DWORD timeouit = INFINITE);
    void Terminate(UINT exitCode = -1);
    int GetExitCode();

private:
    void InitializePipes();
    void LaunchProcess(std::wstring& commandLine,
                       const std::vector<std::pair<std::string, std::string>>& environment);
    void SetupAutoTerminate();

    bool m_IsRunning = false;
    HANDLE m_Process = NULL;
    HANDLE m_StdinRead = NULL;
    HANDLE m_StdinWrite = NULL;
    HANDLE m_StdoutRead = NULL;
    HANDLE m_StdoutWrite = NULL;
    HANDLE m_TerminateJob = NULL;
};
