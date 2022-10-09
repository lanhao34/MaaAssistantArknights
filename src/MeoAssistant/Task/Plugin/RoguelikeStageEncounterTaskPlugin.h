#pragma once
#include "AbstractTaskPlugin.h"

namespace asst
{
    // 集成战略模式不期而遇任务
    class RoguelikeStageEncounterTaskPlugin : public AbstractTaskPlugin
    {
    public:
        using AbstractTaskPlugin::AbstractTaskPlugin;
        virtual ~RoguelikeStageEncounterTaskPlugin() override = default;

        virtual bool verify(AsstMsg msg, const json::value& details) const override;

    protected:
        virtual bool _run() override;
    };
}
