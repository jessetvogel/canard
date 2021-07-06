package nl.jessetvogel.canard.searcher;

import nl.jessetvogel.canard.core.Function;
import nl.jessetvogel.canard.core.Namespace;

import java.util.*;

public class Searcher {

    private final Queue<Query> queue;
    private final List<Collection<Function>> searchLists;
    private final HashMap<Function, List<Function>> index;
    private final int maxDepth;

    public Searcher(Collection<Namespace> searchSpace, int maxDepth) {
        this.maxDepth = maxDepth;

        // Create a queue as class field, so that we can call .next()
        queue = new LinkedList<>();

        // Create index
        index = new HashMap<>();
        index.put(null, new ArrayList<>());

        // Make a list of lists of all functions that can be used during the search
        // We do this in advance so that we don't constantly create new arraylists
        searchLists = new ArrayList<>();
        for (Namespace space : searchSpace) {
            List<Function> theorems = space.context.getFunctions();
            searchLists.add(theorems);

            for (Function thm : theorems) {
                Function thmTypeBase = thm.getType().getBase();
                // If the thmTypeBase is a dependency of the theorem, then store in the 'general' category, indexed by null
                if(thm.getDependenciesAsFunctions().contains(thmTypeBase))
                    thmTypeBase = null;

                List<Function> list = index.get(thmTypeBase);
                if (list != null) {
                    list.add(thm);
                    continue;
                }
                list = new ArrayList<>();
                list.add(thm);
                index.put(thmTypeBase, list);
            }
        }
    }

    public List<Function> search(Query query) {
        // Clear the queue of queries, and add the query we want to solve for
        queue.clear();
        queue.add(query);

        while (!queue.isEmpty()) {
            // Take the first query from the queue, and try to reduce it
            // Note that we peek, and not poll, because we might need it for the next .next()
            Query q = queue.poll();

//            System.err.println("current query [" + q + "]");

            // Prepare the query before reducing
            Query subQ = q.abstraction();
            if (subQ != null)
                q = subQ;

            // Check for redundancies
            boolean shouldSkip = false;
            for (Query r = q.getParent(); r != null; r = r.getParent()) {
                // If some parent injects into subQuery, then it does not make sense to even consider solving for subQuery: it only complicates things
                if (r.injectsInto(q)) {
                    shouldSkip = true;
                    break;
                }
            }
            if(shouldSkip)
                continue;

            // If subQuery injects into the parent r, then we can safely delete all other branches coming from that parent r
            // since any solution of the parent will yield a solution for q
            for (Query r = q.getParent(); r != null; r = r.getParent()) {
                if (q.injectsInto(r)) {
                    List<Query> toRemove = new ArrayList<>();
                    for (Query s : queue) {
                        for (Query t = s.getParent(); t != null; t = t.getParent()) {
                            if (t == r) {
                                toRemove.add(s);
                                break;
                            }
                        }
                    }
                    queue.removeAll(toRemove);
                }
            }

            // First list to search through is the list of local variables of q
            for(Function thm : q.getLocals()) {
                List<Function> result = searchHelper(q, thm);
                if(result != null)
                    return result;
            }

            // TODO: clean this up
            // If there are indexed theorems, try to use those
            Function hTypeBase = q.getLastIndeterminate().getType().getBase();
            List<Function> indexedTheorems = index.get(hTypeBase);
            if(indexedTheorems != null) {
                for(Function thm : indexedTheorems) {
                    List<Function> result = searchHelper(q, thm);
                    if (result != null)
                        return result;
                }

                // Also try the general theorems
                for(Function thm : index.get(null)) {
                    List<Function> result = searchHelper(q, thm);
                    if (result != null)
                        return result;
                }
            }
            // If the type base was unknown to the index, just try everything!
            else {
                for (Collection<Function> list : searchLists) {
                    for (Function thm : list) {
                        List<Function> result = searchHelper(q, thm);
                        if (result != null)
                            return result;
                    }
                }
            }
        }

        return null;
    }

    private List<Function> searchHelper(Query q, Function thm) {
        // Try applying thm to reduce the query to a subQuery
        Query subQuery = q.reduce(thm);
        if (subQuery == null)
            return null;

//        System.err.println("reduced query [" + subQuery + "]");

        // If the subQuery is completely solved (i.e. no more indeterminates) we are done!
        if (subQuery.solved())
            return subQuery.getFinalSolutions();

        // If the maximum depth was reached, omit this subQuery
        if (subQuery.getDepth() >= maxDepth)
            return null;

//        boolean stopSearching = false;
//
//
//        // Check for duplicate/parallel queries (based on hashCodes)
//        boolean duplicateQuery = false;
//        int matchesByHashCode = 0;
//        for (Query r : queue) {
//            if (r.hashCode() == subQuery.hashCode()) {
//                matchesByHashCode++;
//                if (subQuery.injectsInto(r) && r.injectsInto(subQuery)) {
//                    duplicateQuery = true;
//                    break;
//                }
//            }
//        }

//                    if(matchesByHashCode > 0 && queue.size() > 0)
//                        System.err.println("Matches by hashCode = " + matchesByHashCode + ", i.e. " + (100 * matchesByHashCode / queue.size()) + " % (duplicateQuery = " + duplicateQuery + ")");

//        if (!duplicateQuery)
        queue.add(subQuery);

        return null;
//        if (stopSearching)
//            continue loopQueue;
    }

}
