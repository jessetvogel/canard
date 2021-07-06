package nl.jessetvogel.canard.searcher;

import nl.jessetvogel.canard.core.Function;
import nl.jessetvogel.canard.core.Matcher;

import java.util.*;
import java.util.stream.Collectors;

public class Query {

    private final Query parent;
    final List<Function> indeterminates;
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
        this.locals = new HashSet<>();
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

        Query subQuery = new Query(this, newSolutions, newIndeterminates, depths); // Just use the same depths, doesn't seem fair to charge for an abstraction..

        subQuery.locals.addAll(locals);
        subQuery.allowedLocals.putAll(allowedLocals);

        // Add the dependencies of h to the locals of subQuery
        // Then hNew may depend on these new locals
        Set<Function> hNewAllowed = new HashSet<>();
        if (allowedLocals.containsKey(h)) // Add all that h was allowed to
            hNewAllowed.addAll(allowedLocals.get(h));
        for (Function.Dependency dependency : h.getDependencies()) {
            subQuery.locals.add(dependency.function);
            hNewAllowed.add(dependency.function);
        }
        subQuery.allowedLocals.remove(h);
        subQuery.allowedLocals.put(hNew, hNewAllowed);

        return subQuery;
    }

    Query reduce(Function thm) {
        // Get last indeterminate
        Function h = indeterminates.get(indeterminates.size() - 1);

        // At this point, h is asserted to have no dependencies!
        assert (h.getDependencies().isEmpty());

        // Get allowed locals for h
        Set<Function> hAllowed = allowedLocals.get(h);

        // Check: if thm *itself* is a local: then require that it is allowed for h! This is very important.
        if (locals.contains(thm) && (hAllowed == null || !hAllowed.contains(thm)))
            return null;

        // Create matcher
        List<Function> allIndeterminates = new ArrayList<>(indeterminates);
        List<Function> thmDependencies = thm.getDependenciesAsFunctions();
        allIndeterminates.addAll(thmDependencies);
        Matcher matcher = new Matcher(allIndeterminates);

        // Thm arguments have same allowances as h (if there are any)
        if (hAllowed != null && !hAllowed.isEmpty()) {
            // TODO: apparently it is slow to constantly copy the hashmap. Can we think of an alternative ??
            //  - Maybe only check afterwards if the solution is actually allowed ?
            Map<Function, Set<Function>> allowedLocalsCopy = new HierarchicalMap<>(allowedLocals);
            for (Function thmDep : thmDependencies)
                allowedLocalsCopy.put(thmDep, hAllowed);
            matcher.setLocalVariables(locals, allowedLocalsCopy);
        } else {
            // Note: this is important. Even though the current h does not have any allowed locals,
            // still there can be locals, which are *disallowed* for h, so we must pass locals to the matcher
            matcher.setLocalVariables(locals, allowedLocals);
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
//        System.out.println("This was done using the Matcher " + matcher);
//        System.out.print("Btw, locals = { ");
//        for(Function l : locals)
//            System.out.print(" " + l.toFullString() + " ");
//        System.out.println(" }");

        // Create the new subQuery, and keep track of the solutions (for indeterminates) and the arguments (for thm)
        // and the new indeterminates (for the subQuery)
        List<Function> newIndeterminates = new ArrayList<>();
        Map<Function, Function> solutions = new HashMap<>();
        Map<Function, Function> arguments = new HashMap<>();
        List<Integer> newDepths = new ArrayList<>();
        Query subQuery = new Query(this, solutions, newIndeterminates, newDepths);

        int hDepth = depths.get(indeterminates.indexOf(h));

        List<Function> mappables = new ArrayList<>(allIndeterminates);
        mappables.remove(h); // Note that h will be solved for, so will no longer be an indeterminate
        mappables.addAll(locals); // Locals might need mapping!
        Matcher queryToSubQuery = new Matcher(mappables);

        List<Function> unmapped = new ArrayList<>(mappables);
        boolean changes = true;
        while (changes && !unmapped.isEmpty()) {
            changes = false;
            for (ListIterator<Function> it = unmapped.listIterator(); it.hasNext(); ) {
                Function f = it.next();
                // When f is an indeterminate of this query
                if (indeterminates.contains(f)) {
                    Function solution = matcher.getSolution(f);
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
                        solution = queryToSubQuery.cheapClone(f); // This can be done cheaply, i.e. might preserve f as an indeterminate
                        if (solution != f)
                            solution.setLabel(f.label); // + "'"); // Not really needed, but is pretty
                        newIndeterminates.add(solution);
                        newDepths.add(depths.get(indeterminates.indexOf(f))); // The indeterminate has not changed, so its depth remains the same
//                        System.out.println("No solution yet for " + f);

                        if (allowedLocals.containsKey(f)) // Pass on the allowed locals (if f has any) (Note: the locals will get replaced later on)
                            subQuery.allowedLocals.put(solution, allowedLocals.get(f));
                    }
                    // Store the solution, un-mark as unmapped, match, and indicate that changes are made
                    if (solution != f)
                        solutions.put(f, solution);

                    queryToSubQuery.assertMatch(f, solution);
                    it.remove();
                    changes = true;
                    break;
                }
                // Case it is a dependency of thm:
                else if (thmDependencies.contains(f)) {
                    Function argument = matcher.getSolution(f);
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
                        argument = queryToSubQuery.clone(f); // Note: no cheap clone here! This might cause confusion e.g. when theorems are applied twice
                        if (argument != f)
                            argument.setLabel(f.label); // + "'"); // Not really needed, but is pretty
                        newIndeterminates.add(argument);
                        newDepths.add(hDepth + 1); // This indeterminate was introduced because of h, so its depth is one more than that of h
//                        System.out.println("No argument yet for " + f + ", so we added " + argument + " as a new indeterminate");

                        // This new indeterminate may depend on the local variables that h was allowed to depend on
                        if (hAllowed != null && !hAllowed.isEmpty())
                            subQuery.allowedLocals.put(argument, hAllowed); // (Note: the locals will get replaced later on)
                    }
                    // Store the argument, un-mark as unmapped, match, and indicate that changes are made
                    assert (argument != f);
                    arguments.put(f, argument);
                    it.remove();
                    queryToSubQuery.assertMatch(f, argument);
                    changes = true;
                    break;
                }
                // Case f is a local
                else if (locals.contains(f)) {
                    // If the signature of this local depends on something unmapped, we cannot know yet what to do
                    if (f.signatureDependsOn(unmapped))
                        continue;

                    // We should clone this dependency (cheaply!) and add it as a local of subQuery
                    Function fClone = queryToSubQuery.cheapClone(f);
                    subQuery.locals.add(fClone);
                    if (fClone != f) // if something changed, then add as a solution
                        solutions.put(f, fClone);
                    queryToSubQuery.assertMatch(f, fClone);
                    it.remove();
                    changes = true;
                    break;
                } else assert (false);
            }
        }

        // If there are still unmapped indeterminates, there must be some dependency loop, so we concede and say
        // we cannot reduce to a subQuery
        if (!unmapped.isEmpty())
            return null;

        // Now, subQuery.locals is set.
        // Also subQuery.allowedLocals is set, but with the OLD locals. We simply need to replace those!
        for (Map.Entry<Function, Set<Function>> entry : subQuery.allowedLocals.entrySet())
            entry.setValue(entry.getValue().stream().map(queryToSubQuery::convert).collect(Collectors.toUnmodifiableSet()));

        // Finally, set solution of h, as specialization of thm
        List<Function> thmArguments = thm.getExplicitDependencies().stream().map(arguments::get).collect(Collectors.toUnmodifiableList());
        try {
            // Note that thm itself must also be converted, since it might have changed along the way to subQuery!
            // E.g. for `search (P {A B : Prop} (h (a : A) : B) (x : A) : B)`
            solutions.put(h, queryToSubQuery.convert(thm).specialize(thmArguments, Collections.emptyList()));
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
            assert (q.parent != null); // (Just to be sure)

//            System.out.println(" -- start new chain layer ---");
//            System.out.println("Current Matcher: " + matcher);

            Matcher nextMatcher = new Matcher(matcher, q.solutions.keySet());
            Function h = q.parent.indeterminates.get(q.parent.indeterminates.size() - 1);
//            for (Function f : q.parent.indeterminates) {
            for (Function f : q.solutions.keySet()) { // TODO: reason is that locals can also be mapped to solutions, and potentially not all indeterminates need to have solutions at each step ?
                Function g = q.solutions.get(f);
                assert (g != null);
                // First convert the solution along the previous matcher
                g = matcher.convert(g);

                // Case f = h, may need to convert the dependencies
                if (f == h && !h.getDependencies().isEmpty()) {
                    try {
                        g = g.specialize(Collections.emptyList(), h.getDependencies().stream().map(matcher::convert).collect(Collectors.toUnmodifiableList()));
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

    public boolean injectsInto(Query other) {
        // Goal is to map (injectively) all my indeterminates to other.indeterminates, but all other.locals to my locals
        // That is, we are checking if this query searches for less with more information.

        // Probably start at the end, work way towards the beginning
        // Method should be recursive
        // Can we just use Matchers ?

        // 1. let f = indeterminates[-1]
        // 2. for each g in other.indeterminates:
        //   2.1. Create subMatcher with f -> g. If fail, continue with next g
        //   2.2. If succeeded, see what indeterminates are mapped, and update the lists of which to map and which are still allowed from other
        //   2.3. Continue recursively to the next (not yet mapped (also not induced mapped)) indeterminate (from the end, remember)

        // At any point, we need:
        // - current matcher (possibly null)
        // - indeterminates yet to be mapped
        // - allowed other.indeterminates to map to

//        System.out.println("Does [" + this + "] inject into [" + other + "] ? ");

        // Shortcut: if this is 'bigger' than other, it won't work
        if (indeterminates.size() > other.indeterminates.size())
            return false;

        return injectsIntoHelper(null, new ArrayList<>(indeterminates), new ArrayList<>(other.indeterminates));

        // TODO: actually, we would probably also need to check for the local variables..
    }

    private boolean injectsIntoHelper(Matcher matcher, List<Function> unmapped, List<Function> allowed) {
        // Base case
        int n = unmapped.size();
        if (n == 0)
            return true;

        // Starting at the back, try to map some unmapped Function to some allowed Function
        Function f = unmapped.get(n - 1);

        // If f is contained in allowed as well, we will assume that should be the assignment!
        if (allowed.remove(f)) {
            unmapped.remove(n - 1);
            return injectsIntoHelper(matcher, unmapped, allowed);
        }

        // Again starting at the back, find some allowed Function g to match our f
        loopG:
        for (int j = allowed.size() - 1; j >= 0; j--) {
            Function g = allowed.get(j);
            Matcher subMatcher = (matcher == null) ? new Matcher(unmapped) : new Matcher(matcher, unmapped);
            if (!subMatcher.matches(f, g))
                continue;

            // Create new unmapped and allowed lists, by removing those which are mapped and to which they are mapped
            List<Function> newUnmapped = new ArrayList<>();
            List<Function> newAllowed = new ArrayList<>(allowed);
            for (Function h : unmapped) {
                Function k = subMatcher.getSolution(h);
                if (k == null)
                    // Add those which are not yet mapped
                    newUnmapped.add(h);
                else {
                    // If h -> k, remove k from allowed lists
                    // If list does not contain k, there was some invalid mapping anyway, so we continue
                    if (!newAllowed.remove(k))
                        continue loopG;
                }
            }
            // Now simply recursively find a match for the next unmapped
            if (injectsIntoHelper(subMatcher, newUnmapped, newAllowed))
                return true;
        }
        return false;
    }

    public Query getParent() {
        return parent;
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
            sb.append(" (").append(f.toString(true, false)).append(")");
        return sb.toString();
    }

    private Integer hashCode = null;

    @Override
    public int hashCode() {
        // If hashCode was already computed return
        if (hashCode != null)
            return hashCode;

        // TODO: do something more clever than this ? Although it should also not be too expensive!
        hashCode = indeterminates.size();
        for (Function f : indeterminates)
            hashCode += f.getType().getBase().hashCode();

        return hashCode;
    }

    public Function getLastIndeterminate() {
        return indeterminates.get(indeterminates.size() - 1);
    }
}
