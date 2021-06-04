package nl.jessetvogel.canard.search;

import nl.jessetvogel.canard.core.Function;
import nl.jessetvogel.canard.core.Namespace;

import java.util.*;

public class Searcher {

    private final Set<Namespace> searchSpace;

    public Searcher() {
        this.searchSpace = new LinkedHashSet<>(); // so that the iterating order is preserved
    }

    public void addSearchSpace(Namespace space) {
        searchSpace.add(space);
    }

    public List<Function> search(Query query, int maxDepth) {
        // Create a queue of queries, and add the query we want to solve for
        Queue<Query> queue = new LinkedList<>();
        queue.add(query);

        while(!queue.isEmpty()) {
            // Take the first query of the queue, and try to reduce it
            Query q = queue.poll();

            // We want to solve for the last indeterminate
            Function h = q.indeterminates.get(q.indeterminates.size() - 1);

            // Search for a theorem/definition to fill it
            // For this, go through all the provided namespaces
            for(Namespace space : searchSpace) {
                for (Function thm : space.context.getFunctions()) {
                    Query subQuery = q.reduce(h, thm);
                    if (subQuery != null) {
//                        System.out.println("Reduced query: " + subQuery);

                        // If the subQuery is completely solved (i.e. no more indeterminates) we are done!
                        if (subQuery.indeterminates.isEmpty())
                            return subQuery.getUltimateSolutions(query.indeterminates);
                        ;

                        // Add all reductions to the end of the queue
                        // but only if it does not exceed the search depth!
                        if (subQuery.getDepth() < maxDepth)
                            queue.add(subQuery);
                    }
                }
            }
        }

        return null;
    }


}
