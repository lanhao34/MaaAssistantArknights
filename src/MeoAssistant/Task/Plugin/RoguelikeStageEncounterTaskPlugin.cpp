#include "RoguelikeStageEncounterTaskPlugin.h"

#include "Utils/AsstRanges.hpp"

#include "../Sub/ProcessTask.h"
#include "Controller.h"
#include "TaskData.h"
#include "Utils/Logger.hpp"

bool asst::RoguelikeStageEncounterTaskPlugin::verify(AsstMsg msg, const json::value& details) const
{
    if (msg != AsstMsg::SubTaskCompleted || details.get("subtask", std::string()) != "ProcessTask") {
        return false;
    }

    if (details.at("details").at("task").as_string() == "Roguelike1StageEncounterEnter") {
        return true;
    }
    else {
        return false;
    }
}

bool asst::RoguelikeStageEncounterTaskPlugin::_run()
{
    return true;
}
