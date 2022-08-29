//
// Created by Jesse Vogel on 26/08/2022.
//

#include "Index.h"
#include <algorithm>

struct TheoremEntry {
    FunctionRef thm;
    int preference;

    bool operator<(const TheoremEntry &other) const {
        return preference > other.preference;
    }
};

std::vector<FunctionRef> sort_and_convert(std::vector<TheoremEntry> &input) {
    std::vector<FunctionRef> output;
    output.reserve(input.size());
    std::sort(input.begin(), input.end());
    for (const auto &entry: input)
        output.push_back(entry.thm);
    return output;
}

Index::Index(const std::unordered_set<Namespace *> &spaces) {
    std::vector<TheoremEntry> all_theorems, generic_theorems;
    std::unordered_map<FunctionRef, std::vector<TheoremEntry>> index;

    // Make a list of lists of all functions that can be used during the search
    // We do this in advance so that we don't constantly create new arraylists
    for (const auto space: spaces) {
        all_theorems.reserve(all_theorems.size() + space->context().map().size());
        for (const auto &entry: space->context().map()) {
            const auto &thm = entry.second;
            const auto &thm_type_base = thm.type().base();
            const int preference = space->get_preference(thm);

            all_theorems.push_back({thm, preference});

            // If thm.type().base() is a parameter of the theorem, then store in the 'general' category
            if (thm->parameters().contains(thm_type_base)) {
                generic_theorems.push_back({thm, preference});
                continue;
            }

            auto it = index.find(thm_type_base);
            if (it != index.end()) {
                it->second.push_back({thm, preference});
                continue;
            }

            // Alternatively, create a new entry in the index
            index.emplace(thm_type_base, std::vector<TheoremEntry>({{thm, preference}}));
        }
    }

    // Sort and copy
    m_all_theorems = sort_and_convert(all_theorems);
    m_generic_theorems = sort_and_convert(generic_theorems);
    for (auto &entry: index)
        m_index.emplace(entry.first, sort_and_convert(entry.second));
}

const std::vector<FunctionRef> *Index::theorems(const FunctionRef &f) const {
    auto it = m_index.find(f);
    return (it != m_index.end()) ? &it->second : nullptr;
}
