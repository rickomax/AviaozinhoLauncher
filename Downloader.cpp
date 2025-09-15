#include "Downloader.h"

#include <thread>
#include <chrono>
#include <algorithm>
#include <cstdio>

static const char* FileTypeToString(EWorkshopFileType t) {
    switch (t) {
    case k_EWorkshopFileTypeCommunity: return "Community";
    case k_EWorkshopFileTypeMicrotransaction: return "Microtransaction";
    case k_EWorkshopFileTypeCollection: return "Collection";
    case k_EWorkshopFileTypeArt: return "Art";
    case k_EWorkshopFileTypeVideo: return "Video";
    case k_EWorkshopFileTypeScreenshot: return "Screenshot";
    case k_EWorkshopFileTypeGame: return "Game";
    case k_EWorkshopFileTypeSoftware: return "Software";
    case k_EWorkshopFileTypeConcept: return "Concept";
    case k_EWorkshopFileTypeWebGuide: return "WebGuide";
    case k_EWorkshopFileTypeIntegratedGuide: return "IntegratedGuide";
    case k_EWorkshopFileTypeMerch: return "Merch";
    case k_EWorkshopFileTypeControllerBinding: return "ControllerBinding";
    case k_EWorkshopFileTypeSteamworksAccessInvite: return "AccessInvite";
    case k_EWorkshopFileTypeSteamVideo: return "SteamVideo";
    case k_EWorkshopFileTypeGameManagedItem: return "GameManagedItem";
    default: return "Unknown";
    }
}

Downloader::Downloader(AppId_t appID)
    : m_appID(appID)
    , m_itemInstalled(this, &Downloader::OnItemInstalled)
{
}

bool Downloader::Init() {
    if (m_inited) return true;
    if (!SteamAPI_Init()) {
        std::fprintf(stderr, "SteamAPI_Init failed (is Steam running? steam_appid.txt set?)\n");
        return false;
    }
    m_inited = true;
    return true;
}

void Downloader::Shutdown() {
    if (m_inited) {
        SteamAPI_Shutdown();
        m_inited = false;
    }
}

UGCQueryHandle_t Downloader::BuildAllItemsQuery(
    uint32 page,
    EUGCQuery sort,
    EUGCMatchingUGCType mtype,
    const std::vector<std::string>& requiredTags,
    const std::vector<std::vector<std::string>>& requiredTagGroups)
{
    UGCQueryHandle_t qh = SteamUGC()->CreateQueryAllUGCRequest(
        sort, mtype, m_appID, m_appID, page);

    SteamUGC()->SetReturnLongDescription(qh, true);
    SteamUGC()->SetReturnKeyValueTags(qh, true);
    SteamUGC()->SetReturnMetadata(qh, true);
    SteamUGC()->SetReturnChildren(qh, false);
    SteamUGC()->SetReturnAdditionalPreviews(qh, true);
    SteamUGC()->SetAllowCachedResponse(qh, 0);

    for (const auto& tag : requiredTags) {
        if (!tag.empty()) SteamUGC()->AddRequiredTag(qh, tag.c_str());
    }

    for (const auto& group : requiredTagGroups) {
        if (group.empty()) continue;
        std::vector<const char*> ctags; ctags.reserve(group.size());
        for (const auto& t : group) if (!t.empty()) ctags.push_back(t.c_str());
        if (!ctags.empty()) {
            SteamParamStringArray_t arr{ ctags.data(), (int)ctags.size() };
            SteamUGC()->AddRequiredTagGroup(qh, &arr);
        }
    }

    return qh;
}

void Downloader::RequestPage(UGCQueryHandle_t qh) {
    m_queryInFlight = true;
    m_queryFailed = false;
    SteamAPICall_t call = SteamUGC()->SendQueryUGCRequest(qh);
    m_onQueryCompleted.Set(call, this, &Downloader::OnQueryCompleted);
}

void Downloader::OnQueryCompleted(SteamUGCQueryCompleted_t* p, bool ioFailure) {
    m_queryInFlight = false;

    if (ioFailure || p->m_eResult != k_EResultOK) {
        m_queryFailed = true;
        SteamUGC()->ReleaseQueryUGCRequest(p->m_handle);
        return;
    }

    m_totalMatched = p->m_unTotalMatchingResults;
    ExtractResults(p->m_handle, p->m_unNumResultsReturned);

    uint32 fetchedSoFar = static_cast<uint32>(m_items.size());
    m_hasMorePages = (fetchedSoFar < m_totalMatched);

    SteamUGC()->ReleaseQueryUGCRequest(p->m_handle);
}

void Downloader::ExtractResults(UGCQueryHandle_t qh, uint32 numResultsReturned) {
    for (uint32 i = 0; i < numResultsReturned; ++i) {
        SteamUGCDetails_t det{};
        if (!SteamUGC()->GetQueryUGCResult(qh, i, &det)) continue;

        WorkshopItemInfo info;
        info.id = det.m_nPublishedFileId;
        info.owner = det.m_ulSteamIDOwner;
        info.title = det.m_rgchTitle ? det.m_rgchTitle : "";
        info.fileType = FileTypeToString((EWorkshopFileType)det.m_eFileType);
        info.votesUp = det.m_unVotesUp;
        info.votesDown = det.m_unVotesDown;
        info.score = det.m_flScore;
        info.banned = det.m_bBanned;
        info.tagsTruncated = det.m_bTagsTruncated;
        info.timeCreated = det.m_rtimeCreated;
        info.timeUpdated = det.m_rtimeUpdated;

        if (det.m_rgchDescription) {
            info.description = det.m_rgchDescription;
        }

        char url[1024];
        if (SteamUGC()->GetQueryUGCPreviewURL(qh, i, url, sizeof(url))) {
            info.previewURL = url;
        }

        char md[4096];
        if (SteamUGC()->GetQueryUGCMetadata(qh, i, md, sizeof(md))) {
            info.metadata = md;
        }

        const CSteamID ownerId{ det.m_ulSteamIDOwner };
        const char* persona = SteamFriends()->GetFriendPersonaName(ownerId);
        if (persona && *persona && std::strcmp(persona, "[unknown]") != 0) {
            info.ownerPersonaName = persona;
        }
        else {
            SteamFriends()->RequestUserInformation(ownerId, true);
        }

        m_ids.push_back(info.id);
        m_items.push_back(std::move(info));
    }
}

void Downloader::ResolveOwnerNames(unsigned maxMillis) {
    using namespace std::chrono;
    const auto until = steady_clock::now() + milliseconds(maxMillis);

    auto needsName = [&]() -> bool {
        for (auto& it : m_items) {
            if (it.ownerPersonaName.empty()) return true;
        }
        return false;
        };

    while (needsName() && steady_clock::now() < until) {
        PumpCallbacks();
        for (auto& it : m_items) {
            if (!it.ownerPersonaName.empty()) continue;
            const CSteamID sid{ it.owner };
            const char* persona = SteamFriends()->GetFriendPersonaName(sid);
            if (persona && *persona && std::strcmp(persona, "[unknown]") != 0) {
                it.ownerPersonaName = persona;
            }
        }
    }
}

std::vector<WorkshopItemInfo> Downloader::EnumerateAll(
    EUGCQuery sort,
    EUGCMatchingUGCType mtype,
    uint32 maxPages,
    const std::vector<std::string>& requiredTags,
    const std::vector<std::vector<std::string>>& requiredTagGroups)
{
    if (!m_inited && !Init()) return {};

    m_items.clear();
    m_ids.clear();
    m_installReady.clear();
    m_totalMatched = 0;
    m_currPage = 1;
    m_hasMorePages = true;
    m_queryInFlight = false;
    m_queryFailed = false;

    while (m_hasMorePages && m_currPage <= maxPages) {
        UGCQueryHandle_t qh = BuildAllItemsQuery(m_currPage, sort, mtype, requiredTags, requiredTagGroups);
        RequestPage(qh);

        while (m_queryInFlight) PumpCallbacks();
        if (m_queryFailed) break;

        ++m_currPage;
    }

    ResolveOwnerNames(1500);

    return m_items;
}

void Downloader::StartDownload(PublishedFileId_t id, bool subscribe) {
    if (subscribe) {
        SteamUGC()->SubscribeItem(id);
    }
    SteamUGC()->DownloadItem(id, true);
}

bool Downloader::IsItemInstalled(PublishedFileId_t id, std::string* installPathOut) {
    uint64 sizeOnDisk = 0;
    char path[1024] = {};
    uint32 ts = 0;
    if (SteamUGC()->GetItemInstallInfo(id, &sizeOnDisk, path, sizeof(path), &ts)) {
        if (installPathOut) *installPathOut = path;
        return true;
    }
    return false;
}

bool Downloader::IsItemDownloading(PublishedFileId_t id, uint64* bytesDownloaded, uint64* bytesTotal) {
    uint64 a = 0, b = 0;
    bool ok = SteamUGC()->GetItemDownloadInfo(id, &a, &b);
    if (bytesDownloaded) *bytesDownloaded = a;
    if (bytesTotal) *bytesTotal = b;
    return ok;
}

void Downloader::PumpCallbacks() {
    SteamAPI_RunCallbacks();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void Downloader::OnItemInstalled(ItemInstalled_t* p) {
    if (p->m_unAppID == m_appID) {
        m_installReady[p->m_nPublishedFileId] = true;
    }
}
