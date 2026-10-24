import QtQuick
import QtQuick.Layouts
import Logos.Theme
import Logos.Controls

// Quest Editor -- the view. Authoring UI over quests_core: the list on the
// left mirrors the core (any module may have changed it), the form on the
// right edits one quest's definition as JSON and can run it through its
// lifecycle for testing. Peer-supplied strings render as plain text.
Item {
    id: root

    // Typed replica of the C++ backend (see src/quest_editor_ui.rep).
    readonly property var backend: logos.module("quest_editor_ui")
    property bool ready: false
    readonly property string status: backend ? backend.status : ""
    readonly property int questCount: backend ? backend.questCount : 0

    // [{id, quest, progress}] from quests_core.listQuests(), parsed here.
    property var quests: []
    property string selectedId: ""

    readonly property var selected: {
        for (var i = 0; i < root.quests.length; ++i)
            if (root.quests[i].id === root.selectedId)
                return root.quests[i]
        return null
    }

    function refresh() {
        if (!root.ready || !root.backend)
            return
        logos.watch(root.backend.listQuests(), function (json) {
            try {
                root.quests = JSON.parse(String(json))
            } catch (e) {
                root.quests = []
            }
        }, function (error) {
            root.quests = []
        })
    }

    function select(id) {
        root.selectedId = id
        var q = root.selected
        idField.text = id
        draftArea.text = q ? JSON.stringify(q.quest, null, 2) : ""
        draftError.text = ""
    }

    function newQuest() {
        root.selectedId = ""
        idField.text = ""
        draftArea.text = JSON.stringify({
            title: "",
            description: "",
            objectives: [ { id: "obj-1", title: "" } ],
            rewards: []
        }, null, 2)
        draftError.text = ""
    }

    function saveDraft() {
        var id = idField.text.trim()
        if (id.length === 0) {
            draftError.text = qsTr("A quest needs an id.")
            return
        }
        var parsed
        try {
            parsed = JSON.parse(draftArea.text)
        } catch (e) {
            draftError.text = qsTr("Not valid JSON: ") + e.message
            return
        }
        if (parsed === null || typeof parsed !== "object" || Array.isArray(parsed)) {
            draftError.text = qsTr("A quest definition must be a JSON object.")
            return
        }
        draftError.text = ""
        logos.watch(root.backend.saveQuest(id, JSON.stringify(parsed)), function (ok) {
            if (ok)
                root.selectedId = id
            else
                draftError.text = qsTr("quests_core refused the quest.")
        }, function (error) {
            draftError.text = qsTr("Error: ") + String(error)
        })
    }

    function lifecycle(call) {
        if (root.selectedId.length === 0)
            return
        logos.watch(call(root.selectedId), function (ok) {}, function (e) {})
    }

    Connections {
        target: logos
        function onViewModuleReadyChanged(moduleName, isReady) {
            if (moduleName === "quest_editor_ui") {
                root.ready = isReady && root.backend !== null
                root.refresh()
            }
        }
    }
    Connections {
        target: root.backend
        ignoreUnknownSignals: true
        function onQuestsChanged() { root.refresh() }
    }
    Component.onCompleted: {
        root.ready = root.backend !== null && logos.isViewModuleReady("quest_editor_ui")
        root.refresh()
        root.newQuest()
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.palette.background
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing.xlarge
        spacing: Theme.spacing.large

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing.medium

            LogosText {
                objectName: "quest_editor_ui.title"
                text: qsTr("Quest Editor")
                textFormat: Text.PlainText
                color: Theme.palette.text
                font.pixelSize: Theme.typography.panelTitleText
                font.weight: Theme.typography.weightBold
            }
            Item { Layout.fillWidth: true }
            LogosText {
                objectName: "quest_editor_ui.connection"
                text: root.ready ? qsTr("%n quest(s)", "", root.questCount) : qsTr("Connecting to backend...")
                textFormat: Text.PlainText
                color: root.ready ? Theme.palette.textSecondary : Theme.palette.warning
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacing.large

            // ── Quest list ──────────────────────────────────────────────
            LogosFrame {
                Layout.preferredWidth: 280
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacing.medium
                    spacing: Theme.spacing.small

                    LogosButton {
                        objectName: "quest_editor_ui.newQuest"
                        Layout.fillWidth: true
                        text: qsTr("New quest")
                        enabled: root.ready
                        onClicked: root.newQuest()
                    }

                    LogosText {
                        visible: root.quests.length === 0
                        text: root.ready ? qsTr("No quests yet.") : ""
                        textFormat: Text.PlainText
                        color: Theme.palette.textSecondary
                    }

                    LogosListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: root.quests
                        delegate: LogosListRow {
                            required property var modelData
                            width: ListView.view ? ListView.view.width : implicitWidth
                            objectName: "quest_editor_ui.quest." + modelData.id
                            title: modelData.quest && modelData.quest.title ? String(modelData.quest.title) : String(modelData.id)
                            subtitle: String(modelData.id)
                            value: modelData.progress ? String(modelData.progress.state) : ""
                            showChevron: modelData.id === root.selectedId
                            onClicked: root.select(modelData.id)
                        }
                    }
                }
            }

            // ── Editor ──────────────────────────────────────────────────
            LogosFrame {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacing.medium
                    spacing: Theme.spacing.small

                    LogosTextField {
                        id: idField
                        objectName: "quest_editor_ui.questId"
                        Layout.fillWidth: true
                        placeholderText: qsTr("quest id, e.g. find-the-well")
                        readOnly: root.selectedId.length > 0
                    }

                    LogosTextArea {
                        id: draftArea
                        objectName: "quest_editor_ui.draft"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        font.family: Theme.typography.mono
                        wrapMode: TextEdit.NoWrap
                    }

                    LogosText {
                        id: draftError
                        objectName: "quest_editor_ui.draftError"
                        visible: text.length > 0
                        textFormat: Text.PlainText
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        color: Theme.palette.error
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacing.small

                        LogosButton {
                            objectName: "quest_editor_ui.save"
                            text: qsTr("Save")
                            variant: LogosButton.Variant.Primary
                            enabled: root.ready
                            onClicked: root.saveDraft()
                        }
                        LogosButton {
                            objectName: "quest_editor_ui.delete"
                            text: qsTr("Delete")
                            enabled: root.ready && root.selectedId.length > 0
                            onClicked: {
                                var id = root.selectedId
                                logos.watch(root.backend.deleteQuest(id), function (ok) { if (ok) root.newQuest() }, function (e) {})
                            }
                        }
                        Item { Layout.fillWidth: true }

                        // Try the quest out: the same lifecycle any module drives.
                        LogosText {
                            visible: root.selected !== null
                            text: root.selected && root.selected.progress ? String(root.selected.progress.state) : ""
                            textFormat: Text.PlainText
                            color: Theme.palette.textSecondary
                        }
                        LogosButton {
                            objectName: "quest_editor_ui.start"
                            text: qsTr("Start")
                            compact: true
                            enabled: root.selected !== null && root.selected.progress.state === "available"
                            onClicked: root.lifecycle(root.backend.startQuest)
                        }
                        LogosButton {
                            objectName: "quest_editor_ui.complete"
                            text: qsTr("Complete")
                            compact: true
                            enabled: root.selected !== null && root.selected.progress.state === "active"
                            onClicked: root.lifecycle(root.backend.completeQuest)
                        }
                        LogosButton {
                            objectName: "quest_editor_ui.reset"
                            text: qsTr("Reset")
                            compact: true
                            enabled: root.selected !== null && root.selected.progress.state !== "available"
                            onClicked: root.lifecycle(root.backend.resetQuest)
                        }
                    }
                }
            }
        }

        LogosText {
            objectName: "quest_editor_ui.status"
            text: qsTr("quests_core: ") + root.status
            textFormat: Text.PlainText
            color: Theme.palette.textSecondary
        }
    }
}
