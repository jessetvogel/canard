package nl.jessetvogel.canard.search;

import nl.jessetvogel.canard.core.Function;
import nl.jessetvogel.canard.core.Namespace;

import java.util.*;

public class Searcher {

    private final Queue<Query> queue;
    private final List<Collection<Function>> searchLists;
    private final int maxDepth;

    public Searcher(Collection<Namespace> searchSpace, int maxDepth) {
        this.maxDepth = maxDepth;

        // Create a queue as class field, so that we can call .next()
        queue = new LinkedList<>();

        // Make a list of lists of all functions that can be used during the search
        // We do this in advance so that we don't constantly create new arraylists
        searchLists = new ArrayList<>();
        searchLists.add(null); // Use the first entry for local variables for any current query
        for (Namespace space : searchSpace)
            searchLists.add(space.context.getFunctions());
    }

    private Function skipTilThm = null;

    public List<Function> search(Query query) {
        // Clear the queue of queries, and add the query we want to solve for
        queue.clear();
        queue.add(query);
        skipTilThm = null;

        // Now simply find the next solution
        return next();
    }

    public List<Function> next() {
        while (!queue.isEmpty()) {
            // Take the first query from the queue, and try to reduce it
            // Note that we peek, and not poll, because we might need it for the next .next()
            Query q = queue.peek();

//            System.out.println("Current query: " + q);

            // Prepare the query before reducing
            Query subQ = q.abstraction();
            if(subQ != null)
                q = subQ;

            // First list to search through is the list of local variables of q
            searchLists.set(0, q.getLocals());

            // Search for a theorem/definition to fill it
            for (Collection<Function> list : searchLists) {
                for (Function thm : list) {
                    // Wait for last thm
                    if(skipTilThm != null) {
                        if(skipTilThm == thm)
                            skipTilThm = null;
                        continue;
                    }

                    Query subQuery = q.reduce(thm);
                    if (subQuery == null)
                        continue;

//                    System.out.println("Reduced query: " + subQuery);

                    // If the subQuery is completely solved (i.e. no more indeterminates) we are done!
                    if (subQuery.solved()) {
                        skipTilThm = thm; // For next result, know that we should skip until we encounter this this thm again
                        return subQuery.getFinalSolutions();
                    }

                    // Add all reductions to the end of the queue
                    // but only if it does not exceed the search depth!
                    if (subQuery.getDepth() < maxDepth)
                        queue.add(subQuery);
                }
            }
            queue.remove();
        }

        return null;
    }

}
