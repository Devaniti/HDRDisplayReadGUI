#include "precompiled_header.h"

#include "subprocess.h"

Subprocess::~Subprocess()
{
    if (m_Process == NULL)
    {
        return;
    }

    Terminate();
    Release();
}

void Subprocess::Start(std::wstring& commandLine,
                       const std::vector<std::pair<std::string, std::string>>& environment)
{
    HDRGUI_ASSERT(!m_IsRunning);
    InitializePipes();
    LaunchProcess(commandLine, environment);
    SetupAutoTerminate();
}

bool Subprocess::IsRunning()
{
    if (!m_IsRunning)
    {
        return false;
    }

    DWORD res = ::WaitForSingleObject(m_Process, 0);
    if (res != WAIT_TIMEOUT)
    {
        m_IsRunning = false;
    }

    return m_IsRunning;
}

void Subprocess::Release()
{
    HDRGUI_ASSERT(!m_IsRunning);
    ::CloseHandle(m_TerminateJob);
    ::CloseHandle(m_Process);
    ::CloseHandle(m_StdinRead);
    ::CloseHandle(m_StdinWrite);
    ::CloseHandle(m_StdoutRead);
    ::CloseHandle(m_StdoutWrite);

    m_Process = NULL;
    m_StdinRead = NULL;
    m_StdinWrite = NULL;
    m_StdoutRead = NULL;
    m_StdoutWrite = NULL;
    m_TerminateJob = NULL;
}

int Subprocess::Read(char* buffer, DWORD size)
{
    DWORD availableBytes = 0;
    BOOL res = PeekNamedPipe(m_StdoutRead, 0, 0, 0, &availableBytes, 0);

    if (!res || availableBytes == 0)
    {
        return 0;
    }

    DWORD bytesRead = 0;
    res = ::ReadFile(m_StdoutRead, buffer, size, &bytesRead, NULL);
    if (!res || bytesRead == 0)
    {
        return 0;
    }

    return bytesRead;
}

bool Subprocess::Join(DWORD timeouit /*= INFINITE*/)
{
    HDRGUI_ASSERT(m_IsRunning);
    DWORD res = ::WaitForSingleObject(m_Process, timeouit);

    if (res == WAIT_OBJECT_0)
    {
        m_IsRunning = false;
        return true;
    }

    return false;
}

void Subprocess::Terminate(UINT exitCode /*= -1*/)
{
    HDRGUI_ASSERT(m_IsRunning);
    ::TerminateProcess(m_Process, exitCode);
    m_IsRunning = false;
}

int Subprocess::GetExitCode()
{
    HDRGUI_ASSERT(!m_IsRunning);
    DWORD exit_code;
    ::GetExitCodeProcess(m_Process, &exit_code);
    return exit_code;
}

void Subprocess::Write(const char* buffer, DWORD size)
{
    HDRGUI_ASSERT(m_IsRunning);
    DWORD written = 0;
    bool res = ::WriteFile(m_StdinWrite, buffer, size, &written, nullptr);
}

void Subprocess::InitializePipes()
{
    SECURITY_ATTRIBUTES securityAttributes{};

    securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityAttributes.bInheritHandle = TRUE;
    securityAttributes.lpSecurityDescriptor = NULL;

    BOOL res = ::CreatePipe(&m_StdoutRead, &m_StdoutWrite, &securityAttributes, 0);
    CHECK_RESULT(res, NULL, L"CreatePipe call failed");

    res = ::CreatePipe(&m_StdinRead, &m_StdinWrite, &securityAttributes, 0);
    CHECK_RESULT(res, NULL, L"CreatePipe call failed");

    res = ::SetHandleInformation(m_StdoutRead, HANDLE_FLAG_INHERIT, 0);
    CHECK_RESULT(res, NULL, L"SetHandleInformation call failed");

    res = ::SetHandleInformation(m_StdinWrite, HANDLE_FLAG_INHERIT, 0);
    CHECK_RESULT(res, NULL, L"SetHandleInformation call failed");
}

void Subprocess::LaunchProcess(std::wstring& commandLine,
                               const std::vector<std::pair<std::string, std::string>>& environment)
{
    PROCESS_INFORMATION procInfo{};
    STARTUPINFOW startupInfo{};

    startupInfo.cb = sizeof(STARTUPINFO);
    startupInfo.hStdInput = m_StdinRead;
    startupInfo.hStdError = m_StdoutWrite;
    startupInfo.hStdOutput = m_StdoutWrite;
    startupInfo.dwFlags |= STARTF_USESTDHANDLES;

    std::vector<char> envBlock;

    for (const auto& [key, value] : environment)
    {
        envBlock.insert(envBlock.end(), key.begin(), key.end());
        envBlock.insert(envBlock.end(), '=');
        envBlock.insert(envBlock.end(), value.begin(), value.end());
        envBlock.insert(envBlock.end(), '\0');
    }
    envBlock.insert(envBlock.end(), '\0');

    BOOL res = ::CreateProcessW(NULL, commandLine.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW,
                                (void*)envBlock.data(), NULL, &startupInfo, &procInfo);
    CHECK_RESULT(res, NULL, L"CreateProcessW call failed");

    m_IsRunning = true;
    m_Process = procInfo.hProcess;
    ::CloseHandle(procInfo.hThread);
}

void Subprocess::SetupAutoTerminate()
{
    m_TerminateJob = ::CreateJobObject(NULL, NULL);
    CHECK_RESULT(m_TerminateJob != NULL, NULL, L"CreateJobObject call failed");

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = {};

    // Configure all child processes associated with the job to terminate if this process crashes
    // or is terminated
    jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

    BOOL res = SetInformationJobObject(m_TerminateJob, JobObjectExtendedLimitInformation, &jobInfo,
                                       sizeof(jobInfo));
    CHECK_RESULT(res, NULL, L"SetInformationJobObject call failed");

    res = AssignProcessToJobObject(m_TerminateJob, m_Process);
    CHECK_RESULT(res, NULL, L"AssignProcessToJobObject call failed");
}
