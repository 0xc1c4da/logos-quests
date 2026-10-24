#pragma once

#include "rep_quests_ui_source.h"
#include "logos_ui_plugin_context.h"

/**
 * The Quests backend. Runs in its own process (logos_host); the QML view in
 * qml/Main.qml talks to it through the Qt Remote Objects replica generated
 * from quests_ui.rep.
 *
 * Derives:
 *   - QuestsUiSimpleSource  -- generated from the .rep; implement its slots and
 *     feed its PROPs (setStatus(), setPingCount()), which sync to the view.
 *   - LogosUiPluginContext -- onContextReady() plus modules(): typed callers
 *     and event subscriptions for any metadata.json "dependencies".
 */
class QuestsUiBackend : public QuestsUiSimpleSource,
                      public LogosUiPluginContext
{
public:
    QString ping(QString message) override;
};
