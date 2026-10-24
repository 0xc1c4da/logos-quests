#include "quests_core_impl.h"

namespace {

// Trim ASCII whitespace; the core stores definitions verbatim otherwise.
std::string trimmed(const std::string& s)
{
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// Cheap shape check. The core does not validate the schema -- the editor
// owns it -- but it refuses payloads that could not be spliced into
// listQuests() output as JSON.
bool looksLikeJsonObject(const std::string& s)
{
    return s.size() >= 2 && s.front() == '{' && s.back() == '}';
}

// JSON-escape a string for embedding as a JSON string value.
std::string quoted(const std::string& s)
{
    std::string out = "\"";
    for (unsigned char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            if (c < 0x20) {
                static const char hex[] = "0123456789abcdef";
                out += "\\u00";
                out += hex[c >> 4];
                out += hex[c & 0xf];
            } else {
                out += static_cast<char>(c);
            }
        }
    }
    out += '"';
    return out;
}

} // namespace

// ── Definitions ──────────────────────────────────────────────────────────

bool QuestsCoreImpl::saveQuest(const std::string& questId, const std::string& questJson)
{
    const std::string def = trimmed(questJson);
    if (questId.empty() || !looksLikeJsonObject(def))
        return false;

    auto it = m_quests.find(questId);
    if (it == m_quests.end())
        m_quests.emplace(questId, Quest{def, State::Available, {}});
    else
        it->second.definition = def;   // progress is kept across edits
    questSaved(questId);
    return true;
}

bool QuestsCoreImpl::deleteQuest(const std::string& questId)
{
    if (m_quests.erase(questId) == 0)
        return false;
    questDeleted(questId);
    return true;
}

std::string QuestsCoreImpl::getQuest(const std::string& questId)
{
    auto it = m_quests.find(questId);
    return it == m_quests.end() ? std::string{} : it->second.definition;
}

std::string QuestsCoreImpl::listQuests()
{
    std::string out = "[";
    bool first = true;
    for (const auto& [id, q] : m_quests) {
        if (!first) out += ',';
        first = false;
        out += "{\"id\":" + quoted(id)
             + ",\"quest\":" + q.definition
             + ",\"progress\":" + progressJson(q) + "}";
    }
    out += ']';
    return out;
}

int64_t QuestsCoreImpl::questCount()
{
    return static_cast<int64_t>(m_quests.size());
}

// ── Lifecycle ────────────────────────────────────────────────────────────

bool QuestsCoreImpl::startQuest(const std::string& questId)
{
    auto it = m_quests.find(questId);
    if (it == m_quests.end() || it->second.state != State::Available)
        return false;
    it->second.state = State::Active;
    questStarted(questId);
    return true;
}

bool QuestsCoreImpl::completeObjective(const std::string& questId, const std::string& objectiveId)
{
    auto it = m_quests.find(questId);
    if (it == m_quests.end() || it->second.state != State::Active || objectiveId.empty())
        return false;
    if (it->second.completedObjectives.insert(objectiveId).second)
        objectiveCompleted(questId, objectiveId);
    return true;
}

bool QuestsCoreImpl::completeQuest(const std::string& questId)
{
    auto it = m_quests.find(questId);
    if (it == m_quests.end() || it->second.state != State::Active)
        return false;
    it->second.state = State::Completed;
    questCompleted(questId);
    return true;
}

bool QuestsCoreImpl::resetQuest(const std::string& questId)
{
    auto it = m_quests.find(questId);
    if (it == m_quests.end())
        return false;
    it->second.state = State::Available;
    it->second.completedObjectives.clear();
    questReset(questId);
    return true;
}

std::string QuestsCoreImpl::getProgress(const std::string& questId)
{
    auto it = m_quests.find(questId);
    return it == m_quests.end() ? std::string{} : progressJson(it->second);
}

// ── Helpers ──────────────────────────────────────────────────────────────

const char* QuestsCoreImpl::stateName(State s)
{
    switch (s) {
    case State::Available: return "available";
    case State::Active:    return "active";
    case State::Completed: return "completed";
    }
    return "available";
}

std::string QuestsCoreImpl::progressJson(const Quest& q) const
{
    std::string out = "{\"state\":\"";
    out += stateName(q.state);
    out += "\",\"completedObjectives\":[";
    bool first = true;
    for (const auto& o : q.completedObjectives) {
        if (!first) out += ',';
        first = false;
        out += quoted(o);
    }
    out += "]}";
    return out;
}
