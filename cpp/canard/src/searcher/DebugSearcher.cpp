//
// Created by Jesse Vogel on 18/08/2022.
//

#include "DebugSearcher.h"

DebugSearcher::DebugSearcher(std::shared_ptr<Query> query) {
    m_query = Query::normalize(query);
}

bool DebugSearcher::apply(const FunctionRef &f) {
    auto sub_query = Query::reduce(m_query, f);
    if (sub_query == nullptr)
        return false;

    m_query = Query::normalize(sub_query);
    return true;
}
