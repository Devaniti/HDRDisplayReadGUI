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
