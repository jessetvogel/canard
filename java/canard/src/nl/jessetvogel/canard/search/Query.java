package nl.jessetvogel.canard.search;

import nl.jessetvogel.canard.core.Function;
import nl.jessetvogel.canard.core.Matcher;
import nl.jessetvogel.canard.core.Session;

import java.util.*;
import java.util.stream.Collectors;

public class Query {

    private final Session session;
    private final Query parent;
    private final Map<Function, Function> solutions;
    private final List<Integer> depths;
    final List<Function> indeterminates;

    public Query(Session session, List<Function> indeterminates) {
        this.parent = null;
        this.solutions = null;
        this.session = session;
        this.indeterminates = indeterminates;
        this.depths = indeterminates.stream().map(f -> 0).collect(Collectors.toUnmodifiableList());
    }

    private Query(Query parent, Map<Function, Function> solutions, List<Function> indeterminates, List<Integer> depths) {
        this.session = parent.session;
        this.parent = parent;
        this.solutions = solutions;
        this.indeterminates = indeterminates;
        this.depths = depths;
    }

    Query reduce(Function h, Function thm) {
        // Alternatively: automatically obtain h as it will be the last indeterminate in the list..
        // Get last indeterminate
//        Function h = indeterminates.get(indeterminates.size() - 1);

        // Create matcher
        Matcher matcher = new Matcher(indeterminates, thm.getDependenciesAsFunctions());

        // Note that we do not match (h, thm) since they might not match!
        // (E.g. when thm has dependencies that require arguments, while h has not)
        // Therefore we just match the types of h and thm
        if (!matcher.matches(h.getType(), thm.getType()))
            return null;

//        System.out.println("Valid reduction: solve for " + h + " with " + thm);

        // Create the new subQuery, and keep track of the solutions (for indeterminates) and the arguments (for thm)
        // and the new indeterminates (for the subQuery)
        List<Function> newIndeterminates = new ArrayList<>();
        Map<Function, Function> solutions = new HashMap<>();
        Map<Function, Function> arguments = new HashMap<>();
        List<Integer> newDepths = new ArrayList<>();
        int hDepth = depths.get(indeterminates.indexOf(h));

        List<Function> allIndeterminates = new ArrayList<>(indeterminates);
        allIndeterminates.remove(h); // Note that h will be solved for, so will no longer be an indeterminate
        allIndeterminates.addAll(thm.getDependenciesAsFunctions()); // All dependencies of thm will be indeterminates (unless they were given arguments of course)
        Matcher queryToSubQuery = new Matcher(allIndeterminates, Collections.emptyList());

        List<Function> unmapped = new ArrayList<>(allIndeterminates);
        boolean changes = true;
        while (changes && !unmapped.isEmpty()) {
            changes = false;
            for (ListIterator<Function> it = unmapped.listIterator(); it.hasNext(); ) {
                Function f = it.next();
                // When f is an indeterminate of this query
                if (indeterminates.contains(f)) {
                    Function solution = matcher.mapSolution(f, false);
                    if (solution != null) {
                        // If f has a solution, the solution must not depend on any unmapped indeterminate
                        // And, in this case, we simply convert along the new matcher
                        if (solution.dependsOn(unmapped))
                            continue;
                        solution = queryToSubQuery.convert(solution);
//                        System.out.println("... so it was converted to " + solution);
                    } else {
                        // If f has no solution, this indeterminate remains an indeterminate.
                        // Hence we duplicate it to the new query
                        if (f.signatureDependsOn(unmapped))
                            continue;
                        solution = queryToSubQuery.duplicateDependency(f);
                        solution.setLabel(f + "'"); // Not really needed, but is pretty
                        newIndeterminates.add(solution);
                        newDepths.add(depths.get(indeterminates.indexOf(f))); // The indeterminate has not changed, so its depth remains the same
//                        System.out.println("No solution yet for " + f);
                    }
                    // Store the solution,  un-mark as unmapped, match, and indicate that changes are made
                    solutions.put(f, solution);
                    it.remove();
                    assert (queryToSubQuery.matches(f, solution));
                    changes = true;
                    break;
                }
                // Otherwise it must be a dependency of thm
                else {
                    Function argument = matcher.mapSolution(f, true);
                    if (argument != null) {
                        // If the argument is set, the argument must not depend on unmapped stuff
                        // And, in that case, convert along the new matcher
                        if (argument.dependsOn(unmapped))
                            continue;
                        argument = queryToSubQuery.convert(argument);
                    } else {
                        // If no argument is set, it becomes a new indeterminate
                        // Hence we duplicate it to the new query
                        if (f.signatureDependsOn(unmapped))
                            continue;
                        argument = queryToSubQuery.duplicateDependency(f);
                        argument.setLabel(f + "'"); // Not really needed, but is pretty
                        newIndeterminates.add(argument);
                        newDepths.add(hDepth + 1); // This indeterminate was introduced because of h, so its depth is one more than that of h
//                        System.out.println("No argument yet for " + f + ", so we added " + argument + " as a new indeterminate");
                    }
                    // Store the argument, un-mark as unmapped, match, and indicate that changes are made
                    arguments.put(f, argument);
                    it.remove();
                    assert (queryToSubQuery.matches(f, argument));
                    changes = true;
                    break;
                }
            }
        }

        // If there are still unmapped indeterminates, there must be some dependency loop, so we concede and say
        // we cannot reduce to a subQuery
        if (!unmapped.isEmpty())
            return null;

        // Finally, set solution of h, as specialization of thm
        List<Function> thmArguments = thm.getExplicitDependencies().stream().map(arguments::get).collect(Collectors.toUnmodifiableList());
        try {
            solutions.put(h, session.specialize(thm, thmArguments, Collections.emptyList()));
        } catch (Session.SpecializationException e) {
            System.err.println("Ai ai ai! Not what was supposed to happen!");
            e.printStackTrace();
            return null;
        }

        // Create the reduced query based on the solutions and new indeterminates
        return new Query(this, solutions, newIndeterminates, newDepths);
    }

    List<Function> getUltimateSolutions(List<Function> list) {
        // Make a copy of the list, so that we do not alter the original list!
        list = new ArrayList<>(list);

        // Get chain of subQueries with solutions
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
            for (Map.Entry<Function, Function> entry : q.solutions.entrySet()) {
//                System.out.println("Want to map " + entry.getKey() + " to " + entry.getValue());
                assert (matcher.matches(entry.getKey(), entry.getValue()));
            }

            // Convert each Function in the given list
            for (int i = 0; i < n; ++i) {
                Function original = list.get(i);
                Function converted = matcher.convert(original, false);
                list.set(i, converted);
//                System.out.println("Converted " + original + " to " + converted);
            }
        }

        // Return the final list
        return list;
    }

    public int getDepth() {
        return Collections.max(depths);
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
