#pragma once

#include <boost/xpressive/xpressive.hpp>

#include <string>
#include <vector>
#include <unordered_set>
#include <stdexcept>

namespace html_parser {

inline std::vector<std::string> ExtractLinks(const std::string & html_content,
                                             const boost::xpressive::sregex & re,
                                             bool dedup = true,
                                             size_t reserve_hint = 0) {
    std::vector<std::string> links;
    if (reserve_hint > 0) links.reserve(reserve_hint);

    std::unordered_set<std::string> seen;

    boost::xpressive::sregex_iterator it(html_content.begin(), html_content.end(), re);
    boost::xpressive::sregex_iterator end;
    for (; it != end; ++it) {
        auto match = (*it)[0];
        std::string schedule_file_name = match.str();

        if (dedup) {
            if (seen.insert(schedule_file_name).second) {
                links.emplace_back(std::move(schedule_file_name));
            }
        } else {
            links.emplace_back(std::move(schedule_file_name));
        }
    }

    return links;
}

inline std::vector<std::string> ExtractLinks(const std::string & html_content,
                                             const std::string & url_regex,
                                             bool dedup = true,
                                             size_t reserve_hint = 0) {
    try {
        auto re = boost::xpressive::sregex::compile(url_regex);
        return ExtractLinks(html_content, re, dedup, reserve_hint);
    } catch (const std::exception & ex) {
        throw std::invalid_argument(std::string("Invalid regular expression: ") + ex.what());
    }
}

} // namespace html_parser