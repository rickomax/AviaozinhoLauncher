#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "steam/steam_api.h"

ERemoteStoragePublishedFileVisibility ParseVisibility(const std::string& s);
std::vector<std::string> SplitTags(const std::string& tags);

class Uploader {
public:
    Uploader(AppId_t appID,
        std::string contentDir,
        std::string previewPath,
        std::string title,
        std::string desc,
        std::vector<std::string> tags,
        ERemoteStoragePublishedFileVisibility vis,
        PublishedFileId_t knownId = k_PublishedFileIdInvalid);

    bool Run();

    bool Start(bool manageSteamLifecycle);
    void PumpOnce();
    bool IsDone() const { return m_done; }
    bool IsSuccess() const { return m_success; }
    PublishedFileId_t PublishedId() const { return m_lastPublished; }
    bool NeedsLegalAgreement() const { return m_needsAgreement; }
    const std::string& LastError() const { return m_error; }

    bool GetProgress(uint64_t& bytesProcessed, uint64_t& bytesTotal) const;

private:
    bool Preflight();
    void BeginUpdate(PublishedFileId_t id);
    static const char* EResultToString(EResult r);

    CCallResult<Uploader, CreateItemResult_t> m_callCreateItem;
    void OnCreateItem(CreateItemResult_t* p, bool ioFailure);
    CCallResult<Uploader, SubmitItemUpdateResult_t> m_callSubmit;
    void OnSubmit(SubmitItemUpdateResult_t* p, bool ioFailure);

private:
    AppId_t m_appID;
    std::string m_contentDir, m_preview, m_title, m_desc;
    std::vector<std::string> m_tags;
    ERemoteStoragePublishedFileVisibility m_visibility;
    PublishedFileId_t m_knownId;

    std::string m_contentDirUtf8, m_previewUtf8, m_titleUtf8, m_descUtf8;

    bool m_done = false, m_success = false;
    bool m_started = false;
    bool m_manageSteam = true;
    bool m_ownsSteamInit = false;
    bool m_needsAgreement = false;
    PublishedFileId_t m_lastPublished = k_PublishedFileIdInvalid;
    std::string m_error;

    UGCUpdateHandle_t m_updHandle = k_UGCUpdateHandleInvalid;
};
