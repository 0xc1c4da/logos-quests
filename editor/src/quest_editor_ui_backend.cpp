#include "quest_editor_ui_backend.h"

// Generated umbrella for metadata.json "dependencies": LogosModules behind
// modules(), with the Qt-typed quests_core client and its event accessors.
#include "logos_sdk.h"

bool QuestEditorUiBackend::saveQuest(QString questId, QString questJson)
{
    return modules().quests_core.saveQuest(questId, questJson);
}

bool QuestEditorUiBackend::deleteQuest(QString questId)
{
    return modules().quests_core.deleteQuest(questId);
}

QString QuestEditorUiBackend::getQuest(QString questId)
{
    return modules().quests_core.getQuest(questId);
}

QString QuestEditorUiBackend::listQuests()
{
    return modules().quests_core.listQuests();
}

bool QuestEditorUiBackend::startQuest(QString questId)
{
    return modules().quests_core.startQuest(questId);
}

bool QuestEditorUiBackend::completeObjective(QString questId, QString objectiveId)
{
    return modules().quests_core.completeObjective(questId, objectiveId);
}

bool QuestEditorUiBackend::completeQuest(QString questId)
{
    return modules().quests_core.completeQuest(questId);
}

bool QuestEditorUiBackend::resetQuest(QString questId)
{
    return modules().quests_core.resetQuest(questId);
}

QString QuestEditorUiBackend::getProgress(QString questId)
{
    return modules().quests_core.getProgress(questId);
}

void QuestEditorUiBackend::onContextReady()
{
    // Typed core events -> view. QtRO pushes every PROP change and signal to
    // the QML replica; the view refetches listQuests() on questsChanged.
    auto& core = modules().quests_core;
    core.onQuestSaved([this](const QString& id) { refresh(QStringLiteral("saved %1").arg(id)); });
    core.onQuestDeleted([this](const QString& id) { refresh(QStringLiteral("deleted %1").arg(id)); });
    core.onQuestStarted([this](const QString& id) { refresh(QStringLiteral("started %1").arg(id)); });
    core.onObjectiveCompleted([this](const QString& id, const QString& objective) {
        refresh(QStringLiteral("%1: objective %2 done").arg(id, objective));
    });
    core.onQuestCompleted([this](const QString& id) { refresh(QStringLiteral("completed %1").arg(id)); });
    core.onQuestReset([this](const QString& id) { refresh(QStringLiteral("reset %1").arg(id)); });
    refresh(QStringLiteral("connected to quests_core"));
}

void QuestEditorUiBackend::refresh(const QString& what)
{
    setQuestCount(static_cast<int>(modules().quests_core.questCount()));
    setStatus(what);
    emit questsChanged();
}
