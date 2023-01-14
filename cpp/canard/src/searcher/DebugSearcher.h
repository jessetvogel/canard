//
// Created by Jesse Vogel on 18/08/2022.
//

#pragma once

#include "Query.h"
#include <queue>
#include <mutex>

class DebugSearcher {

public:

    explicit DebugSearcher(std::shared_ptr<Query>);

    bool apply(const FunctionRef &);

    const Query &query() const { return *m_query; }

private:

    bool m_searching = false;

    std::shared_ptr<Query> m_query;

};


