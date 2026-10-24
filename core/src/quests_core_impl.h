#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include "logos_module_context.h"

/**
 * quests_core -- the headless quest system.
 *
 * Runs in its own logos_host process, under Basecamp or logosctl alike.
 * Every public method here IS the module API: the quest editor's backend
 * authors quests through it, any other module (a game, Maps, ...) starts and
 * advances them through it, and all of them follow its events. The Qt plugin
 * glue is generated from this header by logos-module-builder; this class
 * stays Qt-free.
 *
 * Two kinds of state, both keyed by `questId`:
 *   - the definition: a JSON object the editor supplies (opaque to the core
 *     so the schema can evolve in the editor). Suggested keys: "title",
 *     "description", "objectives": [{"id", "title"}], "rewards".
 *   - the progress: owned by the core -- state "available" | "active" |
 *     "completed" plus the set of completed objective ids.
 *
 * Only `bool`, `int64_t`, `std::string` and std containers of them cross the
 * module boundary (LIDL-typed); structured data travels as JSON text.
 */
class QuestsCoreImpl : public LogosModuleContext
{
public:
    // ── Definitions (the editor's side) ──────────────────────────────────

    /// Create a quest, or replace its definition if `questId` exists.
    /// `questJson` must be a JSON object. Returns false (and changes
    /// nothing) if the id is empty or the payload is not an object.
    bool saveQuest(const std::string& questId, const std::string& questJson);

    /// Delete a quest and its progress. Returns false if unknown.
    bool deleteQuest(const std::string& questId);

    /// The definition as given to saveQuest(), or "" if unknown.
    std::string getQuest(const std::string& questId);

    /// Every quest as one JSON array of {"id", "quest": {...}, "progress":
    /// {...}} objects, in id order.
    std::string listQuests();

    /// Number of defined quests.
    int64_t questCount();

    // ── Lifecycle (any module's side) ────────────────────────────────────

    /// available -> active. Returns false if unknown or not available.
    bool startQuest(const std::string& questId);

    /// Mark one objective of an active quest done. Returns false if the
    /// quest is unknown or not active. Does not complete the quest by
    /// itself: the caller decides when all its objectives are met.
    bool completeObjective(const std::string& questId, const std::string& objectiveId);

    /// active -> completed. Returns false if unknown or not active.
    bool completeQuest(const std::string& questId);

    /// Back to available with no completed objectives. Returns false if unknown.
    bool resetQuest(const std::string& questId);

    /// {"state": "...", "completedObjectives": [...]} or "" if unknown.
    std::string getProgress(const std::string& questId);

logos_events:
    /// A quest definition was created or replaced.
    void questSaved(const std::string& questId);
    /// A quest was deleted.
    void questDeleted(const std::string& questId);
    /// A quest became active.
    void questStarted(const std::string& questId);
    /// An objective of an active quest was completed.
    void objectiveCompleted(const std::string& questId, const std::string& objectiveId);
    /// A quest was completed.
    void questCompleted(const std::string& questId);
    /// A quest was reset to available.
    void questReset(const std::string& questId);

private:
    enum class State { Available, Active, Completed };
    struct Quest {
        std::string definition;              // JSON object text
        State state = State::Available;
        std::set<std::string> completedObjectives;
    };
    std::map<std::string, Quest> m_quests;

    static const char* stateName(State s);
    std::string progressJson(const Quest& q) const;
};
