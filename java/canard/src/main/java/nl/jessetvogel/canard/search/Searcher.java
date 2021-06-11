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

        // Make a list of lists of all functions that can be used during the search
        // We do this in advance so that we don't constantly create new arraylists
        List<Collection<Function>> searchLists = new ArrayList<>();
        searchLists.add(null); // Use the first entry for local variables
        for (Namespace space : searchSpace)
            searchLists.add(space.context.getFunctions());

        while (!queue.isEmpty()) {
            // Take the first query of the queue, and try to reduce it
            Query q = queue.poll();
//            System.out.println("Current query: " + q);

            // Prepare the query before reducing
            q.prepare();

            // First list to search through is the list of local variables of q
            searchLists.set(0, q.getLocals());

            // Search for a theorem/definition to fill it
            for (Collection<Function> list : searchLists) {
                for (Function thm : list) {
                    Query subQuery = q.reduce(thm);
                    if (subQuery == null)
                        continue;

//                    System.out.println("Found in local variables: " + thm);
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
