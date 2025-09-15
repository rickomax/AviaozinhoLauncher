#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "steam/steam_api.h"

struct WorkshopItemInfo {
    PublishedFileId_t id{};
    uint64            owner{};
    std::string       ownerPersonaName;
    std::string       title;
    std::string       description;
    std::string       fileType;
    uint32            votesUp{};
    uint32            votesDown{};
    float             score{};
    bool              banned{};
    bool              tagsTruncated{};
    std::string       previewURL;
    std::string       metadata;    
    uint32            timeCreated{};
    uint32            timeUpdated{};
};

class Downloader {
public:
    explicit Downloader(AppId_t appID);

    bool Init();
    void Shutdown();

    std::vector<WorkshopItemInfo> EnumerateAll(
        EUGCQuery sort = k_EUGCQuery_RankedByPublicationDate,
        EUGCMatchingUGCType mtype = k_EUGCMatchingUGCType_Items,
        uint32 maxPages = 100,
        const std::vector<std::string>& requiredTags = {},
        const std::vector<std::vector<std::string>>& requiredTagGroups = {}
    );

    void StartDownload(PublishedFileId_t id, bool subscribe);
    bool IsItemInstalled(PublishedFileId_t id, std::string* installPathOut = nullptr);
    bool IsItemDownloading(PublishedFileId_t id, uint64* bytesDownloaded = nullptr, uint64* bytesTotal = nullptr);
    void PumpCallbacks();

    const std::vector<WorkshopItemInfo>& Items() const { return m_items; }

private:
    UGCQueryHandle_t BuildAllItemsQuery(
        uint32 page,
        EUGCQuery sort,
        EUGCMatchingUGCType mtype,
        const std::vector<std::string>& requiredTags,
        const std::vector<std::vector<std::string>>& requiredTagGroups
    );

    void RequestPage(UGCQueryHandle_t qh);

    CCallResult<Downloader, SteamUGCQueryCompleted_t> m_onQueryCompleted;
    void OnQueryCompleted(SteamUGCQueryCompleted_t* p, bool ioFailure);

    STEAM_CALLBACK(Downloader, OnItemInstalled, ItemInstalled_t, m_itemInstalled);

    void ExtractResults(UGCQueryHandle_t qh, uint32 numResultsReturned);

    void ResolveOwnerNames(unsigned maxMillis = 1500);


private:
    AppId_t m_appID = 0;
    bool    m_inited = false;

    bool    m_queryInFlight = false;
    bool    m_queryFailed = false;
    bool    m_hasMorePages = true;
    uint32  m_currPage = 1;
    uint32  m_totalMatched = 0;

    std::vector<WorkshopItemInfo>   m_items;
    std::vector<PublishedFileId_t>  m_ids;

    std::unordered_map<PublishedFileId_t, bool> m_installReady;
};
