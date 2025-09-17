#include "Uploader.h"
#include <algorithm>
#include <sstream>
#include <thread>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <cctype>
#include <fstream>

namespace fs = std::filesystem;

static std::string ToLower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return s;
}
static std::string AnsiToUtf8(const std::string& s) { return s; } // assume UTF-8 already

ERemoteStoragePublishedFileVisibility ParseVisibility(const std::string& s) {
	auto v = ToLower(s);
	if (v == "friends") return k_ERemoteStoragePublishedFileVisibilityFriendsOnly;
	if (v == "private") return k_ERemoteStoragePublishedFileVisibilityPrivate;
	return k_ERemoteStoragePublishedFileVisibilityPublic;
}
std::vector<std::string> SplitTags(const std::string& tags) {
	std::vector<std::string> out;
	std::stringstream ss(tags);
	std::string item;
	while (std::getline(ss, item, ';')) if (!item.empty()) out.push_back(item);
	return out;
}

static bool IsForbiddenExt(const std::string& extLower)
{
	// Windows executable/script-ish types we want to block
	// (extend as needed)
	static const char* kBad[] = {
		".exe", ".dll", ".ocx", ".sys", ".scr", ".com",
		".msi", ".msp",
		".bat", ".cmd", ".vbs", ".vbe", ".js", ".jse",
		".wsf", ".wsh", ".ps1", ".psm1", ".psd1", ".ps1xml",
		".msix", ".appx"
	};
	for (auto* e : kBad) if (extLower == e) return true;
	return false;
}

static bool FindForbiddenFiles(const fs::path& root,
	std::vector<fs::path>& outOffenders)
{
	std::error_code ec;
	fs::directory_options opts = fs::directory_options::skip_permission_denied;

	// Quick “has files” check and recursive walk
	for (fs::recursive_directory_iterator it(root, opts, ec), end; !ec && it != end; it.increment(ec))
	{
		if (ec) break;
		if (!it->is_regular_file(ec)) continue;

		const fs::path& p = it->path();
		auto extLower = ToLower(p.extension().string());

		bool bad = false;
		if (IsForbiddenExt(extLower)) {
			bad = true;
		}

		if (bad) outOffenders.push_back(p);
		if (outOffenders.size() >= 32) break; // avoid giant lists
	}

	return !outOffenders.empty();
}

Uploader::Uploader(AppId_t appID,
	std::string contentDir,
	std::string previewPath,
	std::string title,
	std::string desc,
	std::vector<std::string> tags,
	ERemoteStoragePublishedFileVisibility vis,
	PublishedFileId_t knownId)
	: m_appID(appID)
	, m_contentDir(std::move(contentDir))
	, m_preview(std::move(previewPath))
	, m_title(std::move(title))
	, m_desc(std::move(desc))
	, m_tags(std::move(tags))
	, m_visibility(vis)
	, m_knownId(knownId)
{
}

const char* Uploader::EResultToString(EResult r) {
	switch (r) {
	case k_EResultOK: return "OK";
	case k_EResultFail: return "Fail (generic)";
	case k_EResultNoConnection: return "NoConnection";
	case k_EResultInvalidParam: return "InvalidParam";
	case k_EResultAccessDenied: return "AccessDenied";
	case k_EResultLimitExceeded: return "LimitExceeded";
	case k_EResultFileNotFound: return "FileNotFound";
	case k_EResultBusy: return "Busy";
	case k_EResultInvalidState: return "InvalidState";
	case k_EResultServiceUnavailable: return "ServiceUnavailable";
	default: return "Unknown";
	}
}

bool Uploader::Preflight() {
	// Convert everything to UTF-8 for Steam
	m_contentDirUtf8 = AnsiToUtf8(m_contentDir);
	m_previewUtf8 = AnsiToUtf8(m_preview);
	m_titleUtf8 = AnsiToUtf8(m_title);
	m_descUtf8 = AnsiToUtf8(m_desc);

	// Validate content directory exists and is non-empty
	fs::path contentPath = fs::u8path(m_contentDirUtf8);
	std::error_code ec;
	if (!fs::exists(contentPath, ec) || !fs::is_directory(contentPath, ec)) {
		m_error = "Content folder does not exist or is not a directory.";
		return false;
	}
	bool hasAny = false;
	for (auto it = fs::directory_iterator(contentPath, ec); !ec && it != fs::directory_iterator(); ++it) {
		hasAny = true; break;
	}
	if (!hasAny) {
		m_error = "Content folder is empty.";
		return false;
	}

	std::vector<fs::path> offenders;
	if (FindForbiddenFiles(contentPath, offenders)) {
		std::ostringstream oss;
		oss << "Upload blocked: executable/script files detected in content folder.\n";
		oss << "Remove these files and try again:\n";
		m_error = oss.str();
		return false;
	}

	// Validate preview file exists and looks like .png/.jpg
	fs::path prevPath = fs::u8path(m_previewUtf8);
	if (!fs::exists(prevPath, ec) || !fs::is_regular_file(prevPath, ec)) {
		m_error = "Preview image not found.";
		return false;
	}
	auto ext = ToLower(prevPath.extension().string());
	if (!(ext == ".png" || ext == ".jpg" || ext == ".jpeg")) {
		m_error = "Preview must be .png/.jpg.";
		return false;
	}

	// Minimal title/desc checks
	if (m_titleUtf8.empty() || m_titleUtf8.size() > 128) {
		m_error = "Title required (<=128 chars).";
		return false;
	}
	if (m_descUtf8.size() > 8000) { // Steam allows long descriptions; keep generous
		m_error = "Description too long.";
		return false;
	}
	return true;
}

bool Uploader::Run() {
	m_manageSteam = true;
	if (!SteamAPI_Init()) { m_error = "SteamAPI_Init failed."; return false; }
	m_ownsSteamInit = true;

	if (!Preflight()) { m_done = true; m_success = false; PumpOnce(); return false; }

	if (m_knownId == k_PublishedFileIdInvalid) {
		SteamAPICall_t h = SteamUGC()->CreateItem(m_appID, k_EWorkshopFileTypeCommunity);
		m_callCreateItem.Set(h, this, &Uploader::OnCreateItem);
	}
	else {
		BeginUpdate(m_knownId);
	}
	m_started = true;

	while (!m_done) {
		PumpOnce();
		std::this_thread::sleep_for(std::chrono::milliseconds(15));
	}

	if (m_ownsSteamInit) { SteamAPI_Shutdown(); m_ownsSteamInit = false; }
	return m_success;
}

bool Uploader::Start(bool manageSteamLifecycle) {
	m_manageSteam = manageSteamLifecycle;
	if (m_manageSteam) {
		if (!SteamAPI_Init()) { m_error = "SteamAPI_Init failed."; return false; }
		m_ownsSteamInit = true;
	}
	if (!Preflight()) { m_done = true; m_success = false; return false; }

	if (m_knownId == k_PublishedFileIdInvalid) {
		SteamAPICall_t h = SteamUGC()->CreateItem(m_appID, k_EWorkshopFileTypeCommunity);
		m_callCreateItem.Set(h, this, &Uploader::OnCreateItem);
	}
	else {
		BeginUpdate(m_knownId);
	}
	m_started = true;
	return true;
}

void Uploader::PumpOnce() {
	SteamAPI_RunCallbacks();
	if (m_done && m_ownsSteamInit && m_manageSteam) {
		SteamAPI_Shutdown();
		m_ownsSteamInit = false;
	}
}

bool Uploader::GetProgress(uint64_t& bytesProcessed, uint64_t& bytesTotal) const {
	if (m_updHandle == k_UGCUpdateHandleInvalid) return false;
	EItemUpdateStatus st = k_EItemUpdateStatusInvalid;
	st = SteamUGC()->GetItemUpdateProgress(m_updHandle, &bytesProcessed, &bytesTotal);
	return st != k_EItemUpdateStatusInvalid;
}

void Uploader::BeginUpdate(PublishedFileId_t id) {
	m_updHandle = SteamUGC()->StartItemUpdate(m_appID, id);

	// All strings must be UTF-8 for Steam
	SteamUGC()->SetItemContent(m_updHandle, m_contentDirUtf8.c_str());
	SteamUGC()->SetItemTitle(m_updHandle, m_titleUtf8.c_str());
	SteamUGC()->SetItemDescription(m_updHandle, m_descUtf8.c_str());
	SteamUGC()->SetItemPreview(m_updHandle, m_previewUtf8.c_str());
	SteamUGC()->SetItemVisibility(m_updHandle, m_visibility);

	std::vector<const char*> ctags; ctags.reserve(m_tags.size());
	for (auto& t : m_tags) ctags.push_back(t.c_str());
	SteamParamStringArray_t tagArray{ ctags.data(), (int)ctags.size() };
	if (!m_tags.empty()) SteamUGC()->SetItemTags(m_updHandle, &tagArray);

	SteamAPICall_t h = SteamUGC()->SubmitItemUpdate(m_updHandle, "Launcher upload");
	m_callSubmit.Set(h, this, &Uploader::OnSubmit);
}

void Uploader::OnCreateItem(CreateItemResult_t* p, bool ioFailure) {
	if (ioFailure) { m_error = "CreateItem IO failure."; m_done = true; m_success = false; return; }
	if (p->m_eResult != k_EResultOK) {
		m_error = std::string("CreateItem failed: ") + EResultToString(p->m_eResult);
		m_done = true; m_success = false; return;
	}
	BeginUpdate(p->m_nPublishedFileId);
}

void Uploader::OnSubmit(SubmitItemUpdateResult_t* p, bool ioFailure) {
	if (ioFailure) { m_error = "Submit IO failure."; m_done = true; m_success = false; return; }
	if (p->m_eResult != k_EResultOK) {
		m_error = std::string("Submit failed: ") + EResultToString(p->m_eResult)
			+ " (code " + std::to_string((int)p->m_eResult) + ")";
		m_done = true; m_success = false; return;
	}
	m_lastPublished = p->m_nPublishedFileId;
	m_needsAgreement = (p->m_bUserNeedsToAcceptWorkshopLegalAgreement != 0);
	m_done = true; m_success = true;
}
