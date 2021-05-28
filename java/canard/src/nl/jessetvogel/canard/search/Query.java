package nl.jessetvogel.canard.search;

import nl.jessetvogel.canard.core.Function;
import nl.jessetvogel.canard.core.Matcher;
import nl.jessetvogel.canard.core.Session;

import java.util.*;

public class Query {

    private final Session session;
    private final Query parent;
    private final Map<Function, Function> solutions;
    final List<Function> indeterminates;

    public Query(Session session, List<Function> indeterminates) {
        this.parent = null;
        this.solutions = null;
        this.session = session;
        this.indeterminates = indeterminates;
    }

    Query(Query parent, Map<Function, Function> solutions, List<Function> indeterminates) {
        this.session = parent.session;
        this.parent = parent;
        this.solutions = solutions;
        this.indeterminates = indeterminates;
    }

    Query reduce(Function h, Function thm) {
        // Alternatively: automatically obtain h as it will be the last indeterminate in the list..
        // Get last indeterminate
//        Function h = indeterminates.get(indeterminates.size() - 1);

        // Create matcher
        List<Function> thmAllDependencies = thm.getDependenciesAsFunctions();
        Matcher matcher = new Matcher(indeterminates, thmAllDependencies);

        // Note that we do not match (h, thm) since they might not match!
        // (E.g. when thm has parameters that require arguments, while h has not)
        // Therefore we just match the types of h and thm
        if (!matcher.matches(h.getType(), thm.getType()))
            return null;

//        System.out.println("Valid reduction: solve for " + h + " with " + thm);

        // Create the new subQuery, and keep track of the solution
        List<Function> newIndeterminates = new ArrayList<>(), newIndeterminatesFromThm = new ArrayList<>();
        Map<Function, Function> solutions = new HashMap<>();

        // Go through the dependencies of thm (both implicit/explicit). If an argument is set by the matcher, use that argument.
        // Otherwise, create a new argument that looks like the old argument
        // h will be given the specialization of thm. Let's determine with which arguments
        Matcher thmArgumentMatcher = new Matcher(thm.getDependenciesAsFunctions(), Collections.emptyList());
        List<Function> thmArguments = new ArrayList<>();
        for (Function.Dependency d : thm.getDependencies()) {
            Function argument = matcher.mapGSolution(d.function);
            if (argument == null) {
                argument = session.createFunction(matcher.convertGtoF(d.function.getType()), Collections.emptyList());
                argument.setLabel(d.function + "'");
//                argument = d.function; // TODO: this may break at some point, maybe we should duplicate the argument ?
                newIndeterminatesFromThm.add(argument);
            }

            assert (thmArgumentMatcher.matches(d.function, argument));

            // Only the explicit argument need to be provided to the thm
            if (d.explicit)
                thmArguments.add(argument);
        }

        // Go through the old indeterminates, store the solutions, and all that have no solution are new indeterminates,
        // apart from h, that one will get a solution a bit later
        // However, not that new indeterminates should be recreated, as their type can be altered/more specified!
        for (Function f : indeterminates) {
            if (f == h)
                continue;

            Function g = matcher.mapFSolution(f);
            if (g != null) {
                g = thmArgumentMatcher.convertFtoG(g); // Convert again since theorem may have gotten new arguments
                solutions.put(f, g);
            }
            else {
                Function convertedFType = thmArgumentMatcher.convertFtoG(matcher.convertFtoG(f.getType()));
                if(convertedFType == f.getType()) { // Re-use arguments if allowed for (i.e. type did not change at all)
                    newIndeterminates.add(f);
                }
                else {
                    Function newF = session.createFunction(convertedFType, Collections.emptyList());
                    newIndeterminates.add(newF);
                    solutions.put(f, newF);
                }
            }
        }

        // Set solution of h
        try {
            solutions.put(h, session.specialize(thm, thmArguments, Collections.emptyList())); // Note that this need not go through thmArgumentMatcher again, since we already give it the new thmArguments
        } catch (Session.SpecializationException e) {
            System.err.println("Ai ai ai! Not what was supposed to happen!");
            e.printStackTrace();
        }

        // Add the new indeterminates coming from the theorem arguments at the END of the list of new indeterminates
        newIndeterminates.addAll(newIndeterminatesFromThm);

        // Create the reduced query based on the solutions and new indeterminates
        return new Query(this, solutions, newIndeterminates);

        // (Print solutions)
//        for (Function f : solutions.keySet())
//            System.out.println("Solution: " + f + " --> " + solutions.get(f));
    }

    List<Function> getUltimateSolutions(List<Function> list) {
        // Make a copy of the list, so that we do not alter the original list!
        list = new ArrayList<>(list);

        // Get chain of solutions
        List<Query> chain = new ArrayList<>();
        for (Query q = this; q.parent != null; q = q.parent)
            chain.add(q);

        // Reverse the chain, so that we start at the top
        Collections.reverse(chain);
//        System.out.println("Chain length = " + chain.size());

        // Step by step convert each expression in the list using the solutions
        int n = list.size();
        for (Query q : chain) {
            // Recreate a Matcher, matching (some of) the indeterminates of q.parent to the solutions that q knows
            assert (q.parent != null);
            Matcher matcher = new Matcher(q.parent.indeterminates, Collections.emptyList());
            assert (q.solutions != null);
            for (Map.Entry<Function, Function> entry : q.solutions.entrySet())
                assert (matcher.matches(entry.getKey(), entry.getValue()));

            // Convert each Function in the given list
            for (int i = 0; i < n; ++i) {
                Function original = list.get(i);
                Function converted = matcher.convertFtoG(original);
                list.set(i, converted);
//                System.out.println("Converted " + original + " to " + converted);
            }
        }

        // Return the final list
        return list;
    }

    @Override
    public String toString() {
        StringJoiner sj = new StringJoiner(" ", "Query ", "");
        for (Function f : indeterminates) {
            sj.add("(" + session.toFullString(f) + ")");
        }
        return sj.toString();
    }

}
