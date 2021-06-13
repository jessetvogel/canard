package nl.jessetvogel.canard.search;

import nl.jessetvogel.canard.core.Function;
import nl.jessetvogel.canard.core.Matcher;

import java.util.*;
import java.util.stream.Collectors;

public class Query {

    private final Query parent;
    private final List<Function> indeterminates;
    private final Map<Function, Function> solutions; // Maps the indeterminates of parent to stuff w.r.t. this query
    private final List<Integer> depths; // An integer per indeterminate, indicating how deep that determinate is used for

    private final Set<Function> locals;
    private final Map<Function, Set<Function>> allowedLocals;

    public Query(List<Function> indeterminates) {
        this.parent = null;
        this.indeterminates = indeterminates;
        this.solutions = null;
        this.depths = indeterminates.stream().map(f -> 0).collect(Collectors.toUnmodifiableList());
        this.locals = new HashSet<>();
        this.allowedLocals = new HashMap<>();
    }

    private Query(Query parent, Map<Function, Function> solutions, List<Function> indeterminates, List<Integer> depths) {
        this.parent = parent;
        this.indeterminates = indeterminates;
        this.solutions = solutions;
        this.depths = depths;
        this.locals = new HashSet<>(parent.locals);
        this.allowedLocals = new HashMap<>();
    }

    void prepare() {
        // Can only prepare once!
//        assert (hType == null);

        // Get last indeterminate
        Function h = indeterminates.get(indeterminates.size() - 1);

        // Set the type and dependencies
//        hType = h.getType();
//        hDependencies = h.getDependencies();

        // Add the dependencies of h to the local variables
        for (Function.Dependency dependency : h.getDependencies())
            locals.add(dependency.function);
    }

    Query reduce(Function thm) {
        // Must be prepared!
//        assert (hType != null);

        // Get last indeterminate
        Function h = indeterminates.get(indeterminates.size() - 1);

        // Create matcher
        List<Function> thmDependencies = thm.getDependenciesAsFunctions();
        Matcher matcher = new Matcher(indeterminates, thmDependencies);

        // Thm arguments have same allowances as h, together with the dependencies of h
        Set<Function> hAllowed = allowedLocals.containsKey(h) ? allowedLocals.get(h) : new HashSet<>();
        for (Function.Dependency d : h.getDependencies())
            hAllowed.add(d.function);
        if (!locals.isEmpty()) { // only do something in non-trivial cases: when there *are* local variables
            Map<Function, Set<Function>> allowedLocalsCopy = new HashMap<>(allowedLocals);
            for (Function thmDep : thmDependencies)
                allowedLocalsCopy.put(thmDep, hAllowed);
            matcher.setLocalVariables(locals, allowedLocalsCopy);
        }

        // Note that we do not match (h, thm) since they might not match!
        // (E.g. when thm has dependencies that require arguments, while h has not)
        // (Also, e.g. when h has dependencies, but we are constructing lambda-style)
        // Therefore we just match the types of h and thm
        if (!matcher.matches(h.getType(), thm.getType()))
            return null;

//        System.out.println("Valid reduction in " + this);
//        System.out.println("  Solve for " + h.getType() + " with " + thm);

        // Create the new subQuery, and keep track of the solutions (for indeterminates) and the arguments (for thm)
        // and the new indeterminates (for the subQuery)
        List<Function> newIndeterminates = new ArrayList<>();
        Map<Function, Function> solutions = new HashMap<>();
        Map<Function, Function> arguments = new HashMap<>();
        List<Integer> newDepths = new ArrayList<>();
        Query subQuery = new Query(this, solutions, newIndeterminates, newDepths);

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
                        solution.setLabel(f.label + "'"); // Not really needed, but is pretty
                        newIndeterminates.add(solution);
                        newDepths.add(depths.get(indeterminates.indexOf(f))); // The indeterminate has not changed, so its depth remains the same
//                        System.out.println("No solution yet for " + f);

                        if(allowedLocals.containsKey(f)) // Pass on the allowed locals (if f has any)
                            subQuery.allowedLocals.put(solution, allowedLocals.get(f));
                    }
                    // Store the solution,  un-mark as unmapped, match, and indicate that changes are made
                    solutions.put(f, solution);
                    it.remove();
                    queryToSubQuery.assertMatch(f, solution);
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
                        argument.setLabel(f.label + "'"); // Not really needed, but is pretty
                        newIndeterminates.add(argument);
                        newDepths.add(hDepth + 1); // This indeterminate was introduced because of h, so its depth is one more than that of h
//                        System.out.println("No argument yet for " + f + ", so we added " + argument + " as a new indeterminate");

                        // This new indeterminate may depend on the local variables that h was allowed to depend on
                        subQuery.allowedLocals.put(argument, hAllowed);
                    }
                    // Store the argument, un-mark as unmapped, match, and indicate that changes are made
                    arguments.put(f, argument);
                    it.remove();
                    queryToSubQuery.assertMatch(f, argument);
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
            solutions.put(h, thm.specialize(thmArguments, Collections.emptyList()));
        } catch (Function.SpecializationException e) {
            System.err.println("Ai ai ai! Not what was supposed to happen!");
            e.printStackTrace();
            assert (false);
            return null;
        }

        // Create the reduced query based on the solutions and new indeterminates
        return subQuery;
    }

    List<Function> getFinalSolutions() {
        // This can only be done if this query is solved
        if (!solved()) return null;

        // Create chain of subQueries, starting at the bottom, all the way up to the top
        // Note: we include every query which has a parent, so the initial query will not be included
        List<Query> chain = new ArrayList<>();
        for (Query q = this; q.parent != null; q = q.parent)
            chain.add(q);

        /* The idea is as follows: starting at the back, `matcher` will map from Query 4, 3, 2, 1 to the solution,
           at each step using the previous matcher

            Query 1 -> Query 2 -> Query 3 -> Query 4
                \           \          \       |
                 ----->      ----->     -> Solution
        */
        Matcher matcher = new Matcher(Collections.emptyList(), Collections.emptyList());
        for (Query q : chain) {
            assert (q.parent != null); // (Just to be sure)
            Matcher nextMatcher = new Matcher(q.parent.indeterminates, Collections.emptyList());
            Function h = q.parent.indeterminates.get(q.parent.indeterminates.size() - 1);
            for (Function f : q.parent.indeterminates) {
                Function g = q.solutions.get(f);
                assert (g != null);
                // First convert the solution along the previous matcher
                g = matcher.convert(g);
                // Then, if needed, add dependencies to the solution of h (Note: it is important that this is done *after* the conversion by matcher!)
                if (f == h && !h.getDependencies().isEmpty()) {
                    try {
                        g = g.specialize(Collections.emptyList(), h.getDependencies());
                    } catch (Function.SpecializationException e) {
                        e.printStackTrace();
                        assert (false);
                        return null;
                    }
                }
                // Now the next matcher should map f to g
                nextMatcher.assertMatch(f, g);
            }
            // Update the matcher
            matcher = nextMatcher;
        }

        // Finally, we can convert the indeterminates of the initial query
        // Note: this is the parent of the last query in the chain (because the initial query does not have a parent,
        // so it is not in the chain)
        Query initialQuery = chain.get(chain.size() - 1).parent;
        return initialQuery.indeterminates.stream().map(matcher::convert).collect(Collectors.toUnmodifiableList());
    }

    public int getDepth() {
        return Collections.max(depths);
    }

    public Set<Function> getLocals() {
        return locals;
    }

    public boolean solved() {
        return indeterminates.isEmpty();
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder("Query");
        for (Function f : indeterminates)
            sb.append(" (").append(f.toFullString()).append(")");
        return sb.toString();
    }
}
