//
// Created by Jesse Vogel on 03/08/2022.
//

#include "Telescope.h"
#include "Function.h"
#include <algorithm>
#include <numeric>

Telescope::Telescope(std::vector<FunctionRef> functions) {
    m_functions = std::move(functions);
}

void Telescope::add(FunctionRef f) {
    m_functions.push_back(std::move(f));
}

void Telescope::add(const std::vector<FunctionRef> &fs) {
    m_functions.insert(m_functions.end(), fs.begin(), fs.end());
}

void Telescope::add(const Telescope &other) {
    add(other.m_functions);
}

bool Telescope::contains(const FunctionRef &f) const {
    return std::find(m_functions.begin(), m_functions.end(), f) != m_functions.end();
}

Telescope Telescope::operator+(const Telescope &other) const {
    std::vector<FunctionRef> new_functions = m_functions;
    new_functions.insert(new_functions.end(), other.m_functions.begin(), other.m_functions.end());
    return Telescope(std::move(new_functions));
}

std::vector<Telescope> Telescope::split() const {
    // Create a vector of group indices
    std::vector<int> groups(m_functions.size());
    std::iota(groups.begin(), groups.end(), 0);
    // Merge groups if they have related functions (i.e. one depends on the other)
    for (int i = 0; i < m_functions.size(); ++i) {
        const auto &f = m_functions[i];
        int &group_f = groups[i];
        for (int j = 0; j < i; ++j) {
            const auto &g = m_functions[j];
            int &group_g = groups[j];
            if (group_g == group_f)
                continue; // nothing effective to do
            bool related = (f->is_base())
                           ? f.signature_depends_on({g})
                           : f.depends_on({g});
            if (related) {
                int group_src = std::max(group_f, group_g);
                int group_dst = std::min(group_f, group_g);
                for (int k = 0; k <= i; ++k) {
                    if (groups[k] == group_src)
                        groups[k] = group_dst;
                }
            }
        }
    }
    // Create vector of telescopes, each corresponding to a group
    int N = *std::max_element(groups.begin(), groups.end()) + 1;
    std::vector<Telescope> telescopes(N);
    for (int i = 0; i < m_functions.size(); ++i)
        telescopes[groups[i]].add(m_functions[i]);
    return telescopes;
}
