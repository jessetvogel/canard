package nl.jessetvogel.canard.search;

import nl.jessetvogel.canard.core.Function;
import nl.jessetvogel.canard.core.Session;

import java.util.LinkedList;
import java.util.List;
import java.util.Queue;

public class Searcher {

    private final Session session;

    public Searcher(Session session) {
        this.session = session;
    }

    public List<Function> search(Query query) {
        // Create a queue of queries, and add the query we want to solve for
        Queue<Query> queue = new LinkedList<>();
        queue.add(query);

        while(!queue.isEmpty()) {
            // Take the first query of the queue, and try to reduce it
            Query q = queue.poll();

            // We want to solve for the last indeterminate
            Function h = q.indeterminates.get(q.indeterminates.size() - 1);

            // Search for a theorem/definition to fill it
            for(Function thm : session.mainContext.getFunctions()) {
                Query subQuery = q.reduce(h, thm);
                if(subQuery != null) {
//                    System.out.println("Reduced query: " + subQuery);

                    // If the subQuery is completely solved (i.e. no more indeterminates) we are done!
                    if(subQuery.indeterminates.isEmpty())
                        return subQuery.getUltimateSolutions(query.indeterminates);;

                    // Add all reductions to the end of the queue
                    queue.add(subQuery);
                }
            }
        }

        return null;
    }


}
