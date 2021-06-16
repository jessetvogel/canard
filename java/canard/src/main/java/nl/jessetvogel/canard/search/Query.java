package nl.jessetvogel.canard.search;

import nl.jessetvogel.canard.core.Function;
import nl.jessetvogel.canard.core.Matcher;

import java.util.*;
import java.util.stream.Collectors;

public class Query {

    private final Query parent;
    final List<Function> indeterminates;
    private final Map<Function, Function> solutions; // Maps the indeterminates of parent to stuff w.r.t. this query
    private final List<Integer> depths; // An integer per indeterminate, indicating how deep that determinate is used for

    private Set<Function> locals;
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

    Query abstraction() {
        // Get last indeterminate
        Function h = indeterminates.get(indeterminates.size() - 1);

        // If h has no dependencies, nothing to do here
        if (h.getDependencies().isEmpty())
            return null;

        // Create a subQuery where h is replaced by a dependency-less version of h
        // This introduces new locals: the current dependencies of h
        List<Function> newIndeterminates = new ArrayList<>(indeterminates);
        newIndeterminates.remove(h);
        Function hNew = new Function(h.getType(), Collections.emptyList());
        newIndeterminates.add(hNew);
        Map<Function, Function> newSolutions = new HashMap<>();
        newSolutions.put(h, hNew);

        Query subQuery = new Query(this, newSolutions, newIndeterminates, depths);

        // Add the dependencies of h to the locals of subQuery
        // Then hNew may depend on these new locals
        Set<Function> hNewAllowed = allowedLocals.containsKey(h) ? allowedLocals.get(h) : new HashSet<>();
        for (Function.Dependency dependency : h.getDependencies()) {
            subQuery.locals.add(dependency.function);
            hNewAllowed.add(dependency.function);
        }
        subQuery.allowedLocals.put(hNew, hNewAllowed);

        return subQuery;
    }

    Query reduce(Function thm) {
        // Get last indeterminate
        Function h = indeterminates.get(indeterminates.size() - 1);

        // At this point, h is asserted to have no dependencies!
        assert (h.getDependencies().isEmpty());

        // Create matcher
        List<Function> allIndeterminates = new ArrayList<>(indeterminates);
        List<Function> thmDependencies = thm.getDependenciesAsFunctions();
        allIndeterminates.addAll(thmDependencies);
        Matcher matcher = new Matcher(allIndeterminates);

        // Thm arguments have same allowances as h (if any)
        Set<Function> hAllowed = allowedLocals.get(h);
        if (hAllowed != null && !hAllowed.isEmpty()) {
            Map<Function, Set<Function>> allowedLocalsCopy = new HashMap<>(allowedLocals);
            for (Function thmDep : thmDependencies)
                allowedLocalsCopy.put(thmDep, hAllowed);
            matcher.setLocalVariables(locals, allowedLocalsCopy);
        }

//        System.out.println("Will '" + thm.toFullString() + "' satisfy '" + h.toFullString() + "' ?");
//        System.out.println("(indeterminates are " + indeterminates.get(0) + " and more ...)");

        // Note that we do not match (h, thm) since they might not match!
        // (E.g. when thm has dependencies that require arguments, while h has not)
        // // (Also, e.g. when h has dependencies, but we are constructing lambda-style)
        // Therefore we just match the types of h and thm
        if (!matcher.matches(h.getType(), thm.getType()))
            return null;

//        System.out.println("Valid reduction in " + this);
//        System.out.println("--> solve for " + h.toFullString() + " with " + thm);

        // Create the new subQuery, and keep track of the solutions (for indeterminates) and the arguments (for thm)
        // and the new indeterminates (for the subQuery)
        List<Function> newIndeterminates = new ArrayList<>();
        Map<Function, Function> solutions = new HashMap<>();
        Map<Function, Function> arguments = new HashMap<>();
        List<Integer> newDepths = new ArrayList<>();
        Query subQuery = new Query(this, solutions, newIndeterminates, newDepths);

        int hDepth = depths.get(indeterminates.indexOf(h));

        allIndeterminates = new ArrayList<>(allIndeterminates); // (TODO: this is just to be sure)
        allIndeterminates.remove(h); // Note that h will be solved for, so will no longer be an indeterminate
        Matcher queryToSubQuery = new Matcher(allIndeterminates);

        List<Function> unmapped = new ArrayList<>(allIndeterminates);
        boolean changes = true;
        while (changes && !unmapped.isEmpty()) {
            changes = false;
            for (ListIterator<Function> it = unmapped.listIterator(); it.hasNext(); ) {
                Function f = it.next();
                // When f is an indeterminate of this query
                if (indeterminates.contains(f)) {
                    Function solution = matcher.mapSolution(f);
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
                        solution.setLabel(f.label); // + "'"); // Not really needed, but is pretty
                        newIndeterminates.add(solution);
                        newDepths.add(depths.get(indeterminates.indexOf(f))); // The indeterminate has not changed, so its depth remains the same
//                        System.out.println("No solution yet for " + f);

                        if (allowedLocals.containsKey(f)) // Pass on the allowed locals (if f has any)
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
                    Function argument = matcher.mapSolution(f);
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
                        argument.setLabel(f.label); // + "'"); // Not really needed, but is pretty
                        newIndeterminates.add(argument);
                        newDepths.add(hDepth + 1); // This indeterminate was introduced because of h, so its depth is one more than that of h
//                        System.out.println("No argument yet for " + f + ", so we added " + argument + " as a new indeterminate");

                        // This new indeterminate may depend on the local variables that h was allowed to depend on
                        if (hAllowed != null && !hAllowed.isEmpty())
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

        // Convert local variables if needed (TODO: probably this must be done in some order as well.. and result must be stored)
        Map<Function, Function> mapLocals = new HashMap<>();
        for (Function l : locals) {
            Function copyL = queryToSubQuery.duplicateDependency(l);
            mapLocals.put(l, copyL);
            solutions.put(l, copyL);
        }
        subQuery.locals = subQuery.locals.stream().map(mapLocals::get).collect(Collectors.toSet());
        for (Map.Entry<Function, Set<Function>> entry : subQuery.allowedLocals.entrySet())
            entry.setValue(entry.getValue().stream().map(mapLocals::get).collect(Collectors.toSet()));

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
        Matcher matcher = new Matcher(Collections.emptyList());
        for (Query q : chain) {
//            System.out.println(" -- start new chain layer ---");
//            System.out.println("Current Matcher: " + matcher);

            assert (q.parent != null); // (Just to be sure)
            Matcher nextMatcher = new Matcher(matcher, q.solutions.keySet());
            Function h = q.parent.indeterminates.get(q.parent.indeterminates.size() - 1);
//            for (Function f : q.parent.indeterminates) {
            for (Function f : q.solutions.keySet()) { // TODO: reason is that locals can also be mapped to solutions, and potentially not all indeterminates need to have solutions at each step ?
                Function g = q.solutions.get(f);
                assert (g != null);
                // First convert the solution along the previous matcher
                g = matcher.convert(g);

                // Case f = h, may need to convert the dependencies
                if (f == h) {
                    if (!h.getDependencies().isEmpty()) {
                        try {
//                            System.out.println("Converted hDependencies = ");
//                            for (Function.Dependency d : h.getDependencies())
//                                System.out.print("(" + d.function + "), ");
//                            System.out.println(" to ... ");
                            Matcher m = matcher;
                            List<Function.Dependency> convertedHDependencies = h.getDependencies().stream().map(d -> new Function.Dependency(m.convert(d.function), d.explicit)).collect(Collectors.toUnmodifiableList());
//                            for (Function.Dependency d : convertedHDependencies)
//                                System.out.print("(" + d.function + "), ");
//                            System.out.println("!");

                            g = g.specialize(Collections.emptyList(), convertedHDependencies);
                        } catch (Function.SpecializationException e) {
                            e.printStackTrace();
                            assert (false);
                            return null;
                        }
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
