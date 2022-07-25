#include "AutoRecruitTask.h"

#include "Resource.h"
#include "OcrImageAnalyzer.h"
#include "Controller.h"
#include "ProcessTask.h"
#include "RecruitImageAnalyzer.h"
#include "RecruitConfiger.h"
#include "Logger.hpp"

namespace asst::recruit_calc
{
    // all combinations and their operator list, excluding empty set and 6-star operators while there is no senior tag
    auto get_all_combs(const std::vector<std::string>& tags, const std::vector<RecruitOperInfo>& all_ops = Resrc.recruit().get_all_opers())
    {
        std::vector<RecruitCombs> rcs_with_single_tag;

        {
            rcs_with_single_tag.reserve(tags.size());
            std::transform(tags.cbegin(), tags.cend(), std::back_inserter(rcs_with_single_tag), [](const std::string& t)
            {
                RecruitCombs result;
                result.tags = { t };
                result.min_level = 6;
                result.max_level = 0;
                result.avg_level = 0;
                return result;
            });

            static constexpr std::string_view SeniorOper = "高级资深干员";

            for (const auto& op : all_ops) {
                for (auto& rc : rcs_with_single_tag) {
                    if (!op.has_tag(rc.tags.front())) continue;
                    if (op.level == 6 && rc.tags.front() != SeniorOper) continue;
                    rc.opers.push_back(op);
                    rc.min_level = (std::min)(rc.min_level, op.level);
                    rc.max_level = (std::max)(rc.max_level, op.level);
                    rc.avg_level += op.level;
                }
            }

            for (auto& rc : rcs_with_single_tag) {
                rc.avg_level /= double(rc.opers.size());
                // intersection and union are based on sorted container
                std::sort(rc.tags.begin(), rc.tags.end());
                std::sort(rc.opers.begin(), rc.opers.end());
            }
        }

        std::vector<RecruitCombs> result;

        // select one tag first
        for (size_t i = 0; i < tags.size(); ++i) {
            RecruitCombs temp1 = rcs_with_single_tag[i];
            if (temp1.opers.empty()) continue; // this is not possible
            result.push_back(temp1); // that is it

            // but what if another tag is also selected
            for (size_t j = i + 1; j < tags.size(); ++j) {
                RecruitCombs temp2 = temp1 * rcs_with_single_tag[j];
                if (temp2.opers.empty()) continue;
                if (!temp2.opers.empty()) result.push_back(temp2); // two tags only

                // select a third one
                for (size_t k = j + 1; k < tags.size(); ++k) {
                    RecruitCombs temp3 = temp2 * rcs_with_single_tag[k];
                    if (temp3.opers.empty()) continue;
                    result.push_back(temp2 * rcs_with_single_tag[k]);
                }
            }
        }

        return result;
    }
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_select_level(std::vector<int> select_level) noexcept
{
    m_select_level = std::move(select_level);
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_confirm_level(std::vector<int> confirm_level) noexcept
{
    m_confirm_level = std::move(confirm_level);
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_need_refresh(bool need_refresh) noexcept
{
    m_need_refresh = need_refresh;
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_max_times(int max_times) noexcept
{
    m_max_times = max_times;
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_use_expedited(bool use_or_not) noexcept
{
    m_use_expedited = use_or_not;
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_skip_robot(bool skip_robot) noexcept
{
    m_skip_robot = skip_robot;
    return *this;
}

asst::AutoRecruitTask& asst::AutoRecruitTask::set_set_time(bool set_time) noexcept
{
    m_set_time = set_time;
    return *this;
}

bool asst::AutoRecruitTask::_run()
{
    if (is_calc_only_task()) {
        int tags_selected = 0;
        bool force_skip = false;
        return recruit_calc_task(force_skip, tags_selected);
    }

    if (!recruit_begin()) return false;

    if (!check_recruit_home_page()) {
        return false;
    }

    if (!m_use_expedited)
        analyze_start_buttons(); // analyze once only

    static constexpr size_t slot_retry_limit = 3;

    size_t slot_fail = 0;
    size_t recruit_times = 0; // how many times has the confirm button been pressed, NOT expedited plan used
    while ((m_use_expedited || !m_pending_recruit_slot.empty()) && recruit_times != m_max_times) {
        if (slot_fail >= slot_retry_limit) { return false; }
        if (m_use_expedited) {
            Log.info("ready to use expedited");
            if (need_exit()) return false;
            recruit_now();
            if (!check_recruit_home_page()) return false;
            analyze_start_buttons();
        }
        if (need_exit()) return false;
        if (!recruit_one())
            ++slot_fail;
        else
            ++recruit_times;
    }
    return true;
}

bool asst::AutoRecruitTask::analyze_start_buttons()
{
    OcrImageAnalyzer start_analyzer;
    start_analyzer.set_task_info("StartRecruit");

    auto image = m_ctrler->get_image();
    start_analyzer.set_image(image);
    m_pending_recruit_slot.clear();
    if (!start_analyzer.analyze()) {
        Log.info("There is no start button");
        return false;
    }
    start_analyzer.sort_result_horizontal();
    m_start_buttons = start_analyzer.get_result();
    for (size_t i = 0; i < m_start_buttons.size(); ++i) {
        m_pending_recruit_slot.emplace_back(i);
    }
    Log.info("Recruit start button size", m_start_buttons.size());
    return true;
}

/// open a pending recruit slot, set timer and tags then confirm, or leave the slot doing nothing
/// return false if:
/// - recognition failed
/// - timer or tags corrupted
/// - failed to confirm
bool asst::AutoRecruitTask::recruit_one()
{
    LogTraceFunction;

    int delay = Resrc.cfg().get_options().task_delay;

    if (m_pending_recruit_slot.empty()) return false;
    size_t index = m_pending_recruit_slot.front();
    if (index > m_start_buttons.size()) {
        Log.info("index", index, "out of range.");
        m_pending_recruit_slot.pop_front();
        return false;
    }
    Log.info("recruit_index", index);
    Rect button = m_start_buttons.at(index).rect;
    m_ctrler->click(button);
    sleep(delay);

    bool force_skip = true;
    int selected = 0;
    bool calc_recognized = recruit_calc_task(force_skip, selected);
    sleep(delay);

    m_pending_recruit_slot.pop_front();

    if (!calc_recognized) {
        // recognition failed, perhaps open the slot again would not help
        {
            json::value info = basic_info();
            info["what"] = "RecruitError";
            info["why"] = "识别错误";
            callback(AsstMsg::SubTaskError, info);
        }
        click_return_button();
        return false;
    }

    if (force_skip) {
        // mark the slot as completed and return
        click_return_button();
        return true;
    }

    if (need_exit()) return false;

    if (!check_time_reduced()) {
        // timer was not set to 09:00:00 properly, likely the tag selection was also corrupted
        // see https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/300#issuecomment-1073287984
        // return and try later
        Log.info("Timer of this slot has not been reduced as expected, will retry later.");
        m_pending_recruit_slot.push_back(index);
        click_return_button();
        return false;
    }

    // TODO: count blue pixels and compare with number of selected tags desired

    if (need_exit()) return false;

    if (!confirm()) {
        Log.info("Failed to confirm current recruit config.");
        m_pending_recruit_slot.push_back(index);
        click_return_button();
        return false;
    }

    return true;
}

// set recruit timer and tags only
bool asst::AutoRecruitTask::recruit_calc_task(bool& out_force_skip, int& out_selected)
{
    LogTraceFunction;

    static constexpr size_t refresh_limit = 3;
    static constexpr size_t analyze_limit = 5;

    size_t refresh_count = 0;
    for (size_t image_analyzer_retry = 0; image_analyzer_retry < analyze_limit;) {
        ++image_analyzer_retry;

        RecruitImageAnalyzer image_analyzer(m_ctrler->get_image());
        if (!image_analyzer.analyze()) continue;
        if (image_analyzer.get_tags_result().size() != RecruitConfiger::CorrectNumberOfTags) continue;

        const std::vector<TextRect> &tags = image_analyzer.get_tags_result();
        bool has_refresh = !image_analyzer.get_refresh_rect().empty();

        std::vector<std::string> tag_names;
        std::transform(tags.begin(), tags.end(), std::back_inserter(tag_names), std::mem_fn(&TextRect::text));

        bool has_special_tag = false;
        bool has_robot_tag = false;

        // tags result
        {
            json::value info = basic_info();
            std::vector<json::value> tag_json_vector;
            std::transform(tags.begin(), tags.end(), std::back_inserter(tag_json_vector), std::mem_fn(&TextRect::text));

            info["what"] = "RecruitTagsDetected";
            info["details"] = json::object{{ "tags", json::array(tag_json_vector) }};
            callback(AsstMsg::SubTaskExtraInfo, info);
        }

        // special tags
        const std::vector<std::string> SpecialTags = { "高级资深干员", "资深干员" };
        auto special_iter = std::find_first_of(SpecialTags.cbegin(), SpecialTags.cend(), tag_names.cbegin(), tag_names.cend());
        if (special_iter != SpecialTags.cend()) {
            json::value info = basic_info();
            info["what"] = "RecruitSpecialTag";
            info["details"] = json::object{{ "tag", *special_iter }};
            callback(AsstMsg::SubTaskExtraInfo, info);
            has_special_tag = true;
        }

        // robot tags
        const std::vector<std::string> RobotTags = { "支援机械" };
        auto robot_iter = std::find_first_of(RobotTags.cbegin(), RobotTags.cend(), tag_names.cbegin(), tag_names.cend());
        if (robot_iter != RobotTags.cend()) {
            json::value info = basic_info();
            info["what"] = "RecruitSpecialTag";
            info["details"] = json::object{{ "tag", *robot_iter }};
            callback(AsstMsg::SubTaskExtraInfo, info);
            has_robot_tag = true;
        }


        std::vector<RecruitCombs> result_vec = recruit_calc::get_all_combs(tag_names);

        // assuming timer would be set to 09:00:00
        for (RecruitCombs& rc: result_vec) {
            rc.min_level = (std::max)(rc.min_level, 3);
        }

        std::sort(
                result_vec.begin(), result_vec.end(),
                [](const RecruitCombs& lhs, const RecruitCombs& rhs) -> bool
                {
                    if (lhs.min_level != rhs.min_level)
                        return lhs.min_level > rhs.min_level; // 最小等级大的，排前面
                    else if (lhs.max_level != rhs.max_level)
                        return lhs.max_level > rhs.max_level; // 最大等级大的，排前面
                    else if (std::fabs(lhs.avg_level - rhs.avg_level) > DoubleDiff)
                        return lhs.avg_level > rhs.avg_level; // 平均等级高的，排前面
                    else
                        return lhs.tags.size() < rhs.tags.size(); // Tag数量少的，排前面
                });


        if (result_vec.empty()) continue;

        const auto& final_combination = result_vec.front();

        {
            json::value info = basic_info();

            json::value results_json;
            results_json["result"] = json::array();
            results_json["level"] = final_combination.min_level;
            results_json["robot"] = m_skip_robot && has_robot_tag;
            std::vector<json::value> result_json_vector;
            for (const auto& comb : result_vec) {
                json::value comb_json;

                std::vector<json::value> tags_json_vector;
                for (const std::string& tag : comb.tags) {
                    tags_json_vector.emplace_back(tag);
                }
                comb_json["tags"] = json::array(std::move(tags_json_vector));

                std::vector<json::value> opers_json_vector;
                for (const RecruitOperInfo& oper_info : comb.opers) {
                    json::value oper_json;
                    oper_json["name"] = oper_info.name;
                    oper_json["level"] = oper_info.level;
                    opers_json_vector.emplace_back(std::move(oper_json));
                }
                comb_json["opers"] = json::array(std::move(opers_json_vector));
                comb_json["level"] = comb.min_level;
                results_json["result"].as_array().emplace_back(std::move(comb_json));
            }
            info["what"] = "RecruitResult";
            info["details"] = results_json;
            callback(AsstMsg::SubTaskExtraInfo, info);
        }

        if (need_exit()) return false;

        // refresh
        if (m_need_refresh && has_refresh
            && !has_special_tag
            && final_combination.min_level == 3
            && !(m_skip_robot && has_robot_tag)
                ) {

            if (refresh_count > refresh_limit) { // unlikely
                json::value info = basic_info();
                info["what"] = "RecruitError";
                info["why"] = "刷新次数达到上限";
                info["details"] = json::object{
                        { "refresh_limit", refresh_limit }
                };
                callback(AsstMsg::SubTaskError, info);
                return false;
            }

            refresh();

            {
                json::value info = basic_info();
                info["what"] = "RecruitTagsRefreshed";
                info["details"] = json::object{
                        { "count", refresh_count },
                        { "refresh_limit", refresh_limit }
                };
                callback(AsstMsg::SubTaskExtraInfo, info);
                Log.trace("recruit tags refreshed", std::to_string(refresh_count), " times, rerunning recruit task");
            }

            ++refresh_count;
            // desired retry, not an error
            --image_analyzer_retry;
            continue;
        }

        if (need_exit()) return false;

        if (!is_calc_only_task()) {
            // do not confirm, force skip
            if (std::find(m_confirm_level.cbegin(), m_confirm_level.cend(), final_combination.min_level) == m_confirm_level.cend()) {
                out_force_skip = true;
                out_selected = 0;
                return true;
            }

            if (m_skip_robot && has_robot_tag) {
                out_force_skip = true;
                out_selected = 0;
                return true;
            }
        }

        // try to set the timer to 09:00:00
        if (m_set_time) {
            for (const Rect& rect : image_analyzer.get_set_time_rect()) {
                m_ctrler->click(rect);
            }
        }

        // nothing to select, leave the selection empty
        if (std::find(m_select_level.cbegin(), m_select_level.cend(), final_combination.min_level) == m_select_level.cend()) {
            out_force_skip = false;
            out_selected = 0;
            return true;
        }

        // select tags
        for (const std::string& final_tag_name : final_combination.tags) {
            auto tag_rect_iter =
                    std::find_if(tags.cbegin(), tags.cend(), [&](const TextRect& r) { return r.text == final_tag_name; });
            if (tag_rect_iter != tags.cend()) {
                m_ctrler->click(tag_rect_iter->rect);
            }
        }

        {
            json::value info = basic_info();
            info["what"] = "RecruitTagsSelected";
            info["details"] = json::object{
                    { "tags", json::array(final_combination.tags) }
            };
            callback(AsstMsg::SubTaskExtraInfo, info);
        }

        out_selected = int(final_combination.tags.size());
        out_force_skip = false;
        return true;
    }
    return false;
}

bool asst::AutoRecruitTask::recruit_begin()
{
    ProcessTask task(*this, { "RecruitBegin" });
    return task.run();
}

bool asst::AutoRecruitTask::check_time_unreduced()
{
    ProcessTask task(*this, { "RecruitCheckTimeUnreduced" });
    task.set_retry_times(1);
    return task.run();
}

bool asst::AutoRecruitTask::check_time_reduced()
{
    ProcessTask task(*this, { "RecruitCheckTimeReduced" });
    task.set_retry_times(2);
    return task.run();
}

bool asst::AutoRecruitTask::check_recruit_home_page()
{
    ProcessTask task(*this, { "RecruitFlag" });
    task.set_retry_times(2);
    return task.run();
}

bool asst::AutoRecruitTask::recruit_now()
{
    ProcessTask task(*this, { "RecruitNow" });
    return task.run();
}

bool asst::AutoRecruitTask::confirm()
{
    // TODO: use RecruitImageAnalyzer::m_confirm_rect or remove it
    // TODO: https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/902
    ProcessTask confirm_task(*this, { "RecruitConfirm" });
    return confirm_task.set_retry_times(5).run();
}

bool asst::AutoRecruitTask::refresh()
{
    ProcessTask refresh_task(*this, { "RecruitRefresh" });
    return refresh_task.run();
}
