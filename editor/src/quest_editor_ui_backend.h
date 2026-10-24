#pragma once

#include "rep_quest_editor_ui_source.h"
#include "logos_ui_plugin_context.h"

/**
 * The Quest Editor backend. Runs in its own process (ui-host); the QML view
 * in qml/Main.qml talks to it through the Qt Remote Objects replica
 * generated from quest_editor_ui.rep.
 *
 * It owns no quest state: quests_core does (so games and other modules use
 * the same quests this editor authors). The backend forwards each slot to
 * the typed `modules().quests_core` client and relays the core's events to
 * the view as the `questsChanged` signal plus the `questCount` PROP.
 * Editor-side logic that is not the core's business -- validating a draft,
 * templates, import/export -- belongs here.
 *
 * Derives:
 *   - QuestEditorUiSimpleSource -- generated from the .rep; implement its
 *     slots, feed its PROPs (setQuestCount(), setStatus()), emit its signals.
 *   - LogosUiPluginContext      -- onContextReady() plus modules(): Qt-typed
 *     callers and event subscriptions for metadata.json "dependencies".
 */
class QuestEditorUiBackend : public QuestEditorUiSimpleSource,
                             public LogosUiPluginContext
{
public:
    bool saveQuest(QString questId, QString questJson) override;
    bool deleteQuest(QString questId) override;
    QString getQuest(QString questId) override;
    QString listQuests() override;

    bool startQuest(QString questId) override;
    bool completeObjective(QString questId, QString objectiveId) override;
    bool completeQuest(QString questId) override;
    bool resetQuest(QString questId) override;
    QString getProgress(QString questId) override;

    /// Fires when ui-host hands the plugin its LogosAPI: the typed
    /// dependency surface is live, so subscriptions are armed here.
    void onContextReady() override;

private:
    /// Re-read the core's state and tell the view, on a later event-loop
    /// turn. Calling back into a module synchronously from inside one of its
    /// event callbacks (or from onContextReady) blocks the dispatcher until
    /// the 30 s call timeout and then yields a default, so the call is always
    /// deferred: 0 ms from an event callback, ~1 s from onContextReady.
    void scheduleRefresh(const QString& what, int delayMs = 0);
    void refresh(const QString& what);
};
