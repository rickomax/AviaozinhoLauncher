#pragma once
#include <steam\isteamnetworkingsockets.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

#include "SteamCallbacks.h"
#include "NetBackend.h"

struct ServerEntry
{
    uint64_t lobbyId;

    std::string address;
    std::string hostname;
    std::string map;

    int clients;
    int maxClients;

    std::string parameters;
    int port;

    bool gotData;
};

extern HWND mainHwnd;

#if false
extern uint8_t netPayload[NETPIPE_BUFFER_SIZE];
#endif

extern std::vector<std::pair<std::string, std::string>> languageMap;
extern std::vector<std::pair<std::string, std::string>> settings;

extern SteamCallbacks steamCallbacks;

extern std::string commandLine;
extern uint64_t lobbyId;

extern std::unordered_map<uint64_t, ServerEntry> serverMap;
extern int expectedLobbies;
extern int pendingLobbies;
extern bool serverListInProgress;

extern int serverListRequestId;
extern int activeServerListRequestId;

static void ShowError(const char* msg)
{
    MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
}

static std::string JsonEscape(const std::string& in)
{
    std::string out;
    out.reserve(in.size() + 8);

    for (unsigned char c : in)
    {
        switch (c)
        {
        case '\"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b";  break;
        case '\f': out += "\\f";  break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            if (c < 0x20)
            {
                char buf[7];
                std::snprintf(buf, sizeof(buf), "\\u%04x", (unsigned)c);
                out += buf;
            }
            else
            {
                out.push_back(static_cast<char>(c));
            }
            break;
        }
    }

    return out;
}

static bool ShellCopyFile(const std::filesystem::path& fromRel, const std::filesystem::path& toRel)
{
    namespace fs = std::filesystem;

    fs::path fromAbs = fs::absolute(fromRel);
    fs::path toAbs = fs::absolute(toRel);

    if (!fs::is_directory(fromAbs)) {
        std::wstring fromStr = fromAbs.c_str();
        std::vector<wchar_t> fromBuf(fromStr.size() + 2, L'\0');
        std::copy(fromStr.begin(), fromStr.end(), fromBuf.begin());

        std::wstring toStr = toAbs.c_str();

        SHFILEOPSTRUCTW op{};
        op.wFunc = FO_COPY;
        op.pFrom = fromBuf.data();
        op.pTo = toStr.c_str();
        op.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

        return SHFileOperationW(&op) == 0 && !op.fAnyOperationsAborted;
    }

    fs::path starPattern = fromAbs / L"*";
    std::wstring fromStar = starPattern.c_str();

    std::vector<wchar_t> fromBuf(fromStar.size() + 2, L'\0');
    std::copy(fromStar.begin(), fromStar.end(), fromBuf.begin());

    std::wstring toStr = toAbs.c_str();

    SHFILEOPSTRUCTW op{};
    op.wFunc = FO_COPY;
    op.pFrom = fromBuf.data();
    op.pTo = toStr.c_str();
    op.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

    return SHFileOperationW(&op) == 0 && !op.fAnyOperationsAborted;
}



template <typename IntT>
static std::optional<IntT> TryParseInt(const std::string& s, int base = 10) {
    try {
        size_t idx = 0;
        long long v = std::stoll(s, &idx, base);
        if (idx == s.size()) return static_cast<IntT>(v);
    }
    catch (...) {}
    return std::nullopt;
}

static std::string ToIso8601UTC(std::time_t t) {
    std::tm gmt{};
#ifdef _WIN32
    gmtime_s(&gmt, &t);
#else
    gmt = *std::gmtime(&t);
#endif
    std::ostringstream oss;
    oss << std::put_time(&gmt, "%Y-%m-%d");
    return oss.str();
}