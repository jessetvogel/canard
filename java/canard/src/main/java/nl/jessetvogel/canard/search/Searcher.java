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

    public List<Function> search(Query query) {
        // Clear the queue of queries, and add the query we want to solve for
        queue.clear();
        queue.add(query);

        // Now simply find the next solution
        return next();
    }

    public List<Function> next() {
        loopQueue:
        while (!queue.isEmpty()) {
            // Take the first query from the queue, and try to reduce it
            // Note that we peek, and not poll, because we might need it for the next .next()
            Query q = queue.poll();

//            System.out.println("Current query: " + q);

            // Prepare the query before reducing
            Query subQ = q.abstraction();
            if (subQ != null)
                q = subQ;

            // Check for redundancies:
            for (Query r = q.getParent(); r != null; r = r.getParent()) {
                // If some parent injects into q, then it does not make sense to even consider solving for q: (via q is clearly not the way to go)
                if (r.injectsInto(q)) {
//                    System.err.println("[" + q + "] was neglected because one of its parents is [" + r + "]");
                    continue loopQueue;
                }

                // If q injects into the parent r, then we can safely delete all other branches coming from that parent r
                // since any solution of the parent will yield a solution for q
                if(q.injectsInto(r)) {
                    Query root = r;
//                    Query qq = q;
                    queue.removeIf(s -> {
                        for(Query t = s.getParent() ; t != null; t = t.getParent())
                            if(t == root) {
//                                System.err.println("[" + s + "] was removed (unnecessary) from the queue since [" + root + "] was already partially solved by [" + qq + "]");
                                return true;
                            }
                        return false;
                    });
                }
            }

            // TODO: check for parallel equalities (hash-based) (a.k.a. the diamond problem)

            // Now the actual search begins

            // First list to search through is the list of local variables of q
            searchLists.set(0, q.getLocals());

            // Search for a theorem/definition to fill it
            for (Collection<Function> list : searchLists) {
                for (Function thm : list) {
                    // Try applying thm to reduce the query to a subQuery
                    Query subQuery = q.reduce(thm);
                    if (subQuery == null)
                        continue;

//                    System.out.println("Reduced query: " + subQuery);

                    // If the subQuery is completely solved (i.e. no more indeterminates) we are done!
                    if (subQuery.solved())
                        return subQuery.getFinalSolutions();

                    // Add all reductions to the end of the queue
                    // but only if it does not exceed the search depth!
                    if (subQuery.getDepth() < maxDepth)
                        queue.add(subQuery);
                }
            }
        }

        return null;
    }

}
