#include "PenguinUploader.h"

#include <Windows.h>

#include <json.h>

#include "Configer.h"
#include "Version.h"
#include "Logger.hpp"
#include "AsstAux.h"

bool asst::PenguinUploader::upload(const std::string& rec_res)
{
    std::string body = cvt_json(rec_res);
    return request_penguin(body);
}

std::string asst::PenguinUploader::cvt_json(const std::string& rec_res)
{
    auto parse_ret = json::parse(rec_res);
    if (!parse_ret) {
        return std::string();
    }
    json::value rec = std::move(parse_ret.value());

    // Doc: https://developer.penguin-stats.io/public-api/api-v2-instruction/report-api
    json::value body;
    body["server"] = Configer::get_instance().m_options.penguin_server;
    body["stageId"] = rec["stage"]["stageId"];
    body["drops"] = rec["drops"];
    body["source"] = "MeoAssistance";
    body["version"] = Version;

    return body.to_string();
}

bool asst::PenguinUploader::request_penguin(const std::string& body)
{
    std::string curl_cmd = R"(curl -H "Content-Type: application/json" -v -i )";
    curl_cmd += "-d \"" + StringReplaceAll(body, "\"", "\\\"") + "\" \"" + Configer::get_instance().m_options.penguin_api + '"';

    DebugTrace("request_penguin |", curl_cmd);

    SECURITY_ATTRIBUTES pipe_sec_attr = { 0 };
    pipe_sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    pipe_sec_attr.lpSecurityDescriptor = nullptr;
    pipe_sec_attr.bInheritHandle = TRUE;
    HANDLE pipe_read = nullptr;
    HANDLE pipe_child_write = nullptr;
    CreatePipe(&pipe_read, &pipe_child_write, &pipe_sec_attr, 4096);

    STARTUPINFOA si = { 0 };
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = pipe_child_write;
    si.hStdError = pipe_child_write;

    PROCESS_INFORMATION pi = { 0 };

    BOOL p_ret = CreateProcessA(NULL, const_cast<LPSTR>(curl_cmd.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (p_ret) {
        std::string pipe_str;
        DWORD read_num = 0;
        DWORD std_num = 0;
        do {
            while (::PeekNamedPipe(pipe_read, NULL, 0, NULL, &read_num, NULL) && read_num > 0) {
                char* pipe_buffer = new char[read_num];
                BOOL read_ret = ::ReadFile(pipe_read, pipe_buffer, read_num, &std_num, NULL);
                if (read_ret) {
                    pipe_str.append(pipe_buffer, pipe_buffer + std_num);
                }
                if (pipe_buffer != nullptr) {
                    delete[] pipe_buffer;
                    pipe_buffer = nullptr;
                }
            }
        } while (::WaitForSingleObject(pi.hProcess, 0) == WAIT_TIMEOUT);

        DWORD exit_ret = -1;
        ::GetExitCodeProcess(pi.hProcess, &exit_ret);
        DebugTrace("curl say:", pipe_str);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    ::CloseHandle(pipe_read);
    ::CloseHandle(pipe_child_write);
    return true;
}