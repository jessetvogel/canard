package nl.jessetvogel.canard.core;

import java.util.*;

public class Matcher {

    final Matcher parent;
    final List<Function> fIndeterminates, gIndeterminates;
    final Map<Function, Function> fSolutions, gSolutions;

    public Matcher(List<Function> fIndeterminates, List<Function> gIndeterminates) {
        parent = null;
        this.fIndeterminates = fIndeterminates;
        this.gIndeterminates = gIndeterminates;
        fSolutions = new HashMap<>();
        gSolutions = new HashMap<>();
    }

    public Matcher(Matcher parent, List<Function> fIndeterminates, List<Function> gIndeterminates) {
        this.parent = parent;
        this.fIndeterminates = fIndeterminates;
        this.gIndeterminates = gIndeterminates;
        fSolutions = new HashMap<>();
        gSolutions = new HashMap<>();
    }

    Matcher(Matcher parent, List<Function> fIndeterminates, List<Function> gIndeterminates, boolean reverse) {
        this.parent = parent;
        this.fIndeterminates = reverse ? gIndeterminates : fIndeterminates;
        this.gIndeterminates = reverse ? fIndeterminates : gIndeterminates;
        fSolutions = new HashMap<>();
        gSolutions = new HashMap<>();
    }

    private boolean putSolution(Function x, Function y, boolean reverse) {
        Function z = (reverse ? gSolutions : fSolutions).get(x);
        if (z == null) {
            (reverse ? gSolutions : fSolutions).put(x, y);
            return true;
        }
        if (z.equals(y))
            return true;

        // In case x is mapped to z and z is also an indeterminate, then we could have also mapped
        // z to x instead. Hence we are satisfied with mapping y to x
        if ((reverse ? fIndeterminates : gIndeterminates).contains(z))
            //noinspection SuspiciousNameCombination
            return putSolution(y, x, !reverse);

//        System.out.println("Nope, " + x + " will not map to " + y);
        return false;
    }

    public Function mapSolution(Function x, boolean reverse) {
        Function y = (reverse ? gSolutions : fSolutions).get(x);
        if (y != null)
            return y;
        if (parent != null)
            return parent.mapSolution(x, reverse);
        return null;
    }

    // Checks if g can be applied to f. So 'g is the argument, f the dependency'
    // Actually it checks if a specialization of g can be applied to f, where thmDependencies denote
    // the arguments of the specialization
    public boolean matches(Function f, Function g) {
        // If f equals g, there is obviously a match, and nothing more to do
        if (f == g)
            return true;

        // Number of dependencies should agree
        List<Function.Dependency> fDependencies = f.getDependencies(), gDependencies = g.getDependencies();
        int n = fDependencies.size();
        if (n != gDependencies.size())
            return false;

        // Dependencies themselves should match
        Matcher subMatcher = (n > 0) ? new Matcher(
                this,
                f.getDependenciesAsFunctions(),
                Collections.emptyList() // One way matching is good enough here! TODO: don't know about this one ?? Pattern also appears in other places!
        ) : this; // Note: there is no need for a subMatcher if f has no dependencies
        for (int i = 0; i < n; ++i) {
            Function.Dependency fDep = fDependencies.get(i);
            Function.Dependency gDep = gDependencies.get(i);

            if (fDep.explicit != gDep.explicit) // Explicitness must match
                return false;

            if (!subMatcher.matches(fDep.function, gDep.function))
                return false;
        }

        // Types should match (note: up to the matches already made by subMatcher)
        if (!subMatcher.matches(f.getType(), g.getType()))
            return false;

        // At this point, f and g agree up to their signature, i.e. their dependencies and types match

        // TODO: what do we do if BOTH f and g are indeterminates ??

        // If f is a parameter of thm, fill in g as argument
        if (gIndeterminates.contains(g))
            return putSolution(g, f, true);

        // If g is an indeterminate, assign f to g
        if (fIndeterminates.contains(f))
            return putSolution(f, g, false);

        // If the bases match, simply match the arguments
        // Note that the bases themselves can be indeterminates, so we have to account for that first.
        // Of course it can also be possible that fBase == gBase (this can be checked as Java Objects, since no base function
        // is represented by different Java Objects)
        Function fBase = f.getBase(), gBase = g.getBase();
        if ((gIndeterminates.contains(gBase) && putSolution(gBase, fBase, true))
                || (fIndeterminates.contains(fBase) && putSolution(fBase, gBase, false))
                || fBase == gBase) {
            List<Function> fArguments = f.getArguments(), gArguments = g.getArguments();
            int l = fArguments.size(); // Since the bases already agree, we know the number of arguments must agree as well
            for (int i = 0; i < l; ++i) {
                if (!matches(fArguments.get(i), gArguments.get(i)))
                    return false;
            }
            return true;
        }

        // Last chance: the parent might let them match, because f might be an indeterminate of some (grand)parent instead!
        if (parent != null)
            return parent.matches(f, g); // TODO: this is necessary, but can it be simplified ?? If it only applies when f is an indeterminate, can check for that instead?
        //  otherwise we must check all the above once again (e.g. types) which is totally unnecessary!

//        System.out.println("Oops, " + f + " does not match " + g);
        return false;
    }

    public boolean matches(Function f, Function g, boolean reverse) {
        return matches(reverse ? g : f, reverse ? f : g);
    }

    public Function convert(Function x) {
        return convert(x, false);
    }

    public Function convert(Function x, boolean reverse) {
        // If x can directly be mapped, simply map x
        Function y = mapSolution(x, reverse);
        if (y != null)
            return y;

        Function base = x.getBase();
        if (x == base) // Note that this is actually a special case of the next step!
            return x;

        // Convert dependencies if needed
        // E.g. something like "(f : Hom X X) => comp f f" must be converted under "X -> Spec R"
        // to "(f : Hom (Spec R) (Spec R)) => comp f f"
        List<Function.Dependency> convertedDependencies = new ArrayList<>();
        List<Function.Dependency> xDependencies = x.getDependencies();
        int n = xDependencies.size();
        Matcher subMatcher = (n > 0) ? new Matcher(
                this,
                x.getDependenciesAsFunctions(),
                Collections.emptyList(),
                reverse
        ) : this; // If x has no dependencies, no need to construct new dependencies. Duh!
        for (Function.Dependency xDep : xDependencies) {
            Function.Dependency dupDep = new Function.Dependency(subMatcher.duplicateDependency(xDep.function, reverse), xDep.explicit);
            assert (subMatcher.matches(xDep.function, dupDep.function, reverse));
            convertedDependencies.add(dupDep);
        }

        // Otherwise, when x is a specialization, we convert each argument of x
        boolean changes = false;
        List<Function> convertedArguments = new ArrayList<>();
        for (Function arg : x.getArguments()) {
            Function convertedArg = subMatcher.convert(arg, reverse);
            if (!convertedArg.equals(arg))
                changes = true;
            convertedArguments.add(convertedArg);
        }

        // Note: also possibly the base should be converted!
        Function baseSolution = mapSolution(base, reverse);
        if (baseSolution != null && baseSolution != base) {
            base = baseSolution; // (silently change the base)
            changes = true;
        }

        // If no actual changes were made to the arguments, we can simply return the original x
        if (!changes)
            return x;

        // Now we must convert the type of x, this requires another subMatcher!
        // Because the type is determined by the arguments provided to the base Function,
        // since we replace those now, the type should be re-determined
        List<Function> baseExplicitDependencies = base.getExplicitDependencies();
        Matcher subSubMatcher = new Matcher(subMatcher, base.getDependenciesAsFunctions(), Collections.emptyList(), reverse);
        int m = baseExplicitDependencies.size();
        for (int i = 0; i < m; ++i)
            assert (subSubMatcher.matches(baseExplicitDependencies.get(i), convertedArguments.get(i), reverse));
        Function convertedType = subSubMatcher.convert(x.getType(), reverse);
        return new Specialization(base, convertedArguments, convertedType, convertedDependencies);
    }

    public Function duplicateDependency(Function x) {
        return duplicateDependency(x, false);
    }

    public Function duplicateDependency(Function x, boolean reverse) {
        // Since we only use this for duplicating dependencies, and dependencies are always their own base function!
        assert (x == x.getBase());
        // TODO: does it make sense to duplicate a specialization ?
        // TODO: are there any shortcuts we can take so that we can simply immediately return x itself ?

        // Duplicate *its* dependencies
        List<Function.Dependency> convertedDependencies = new ArrayList<>();
        List<Function.Dependency> xDependencies = x.getDependencies();
        int n = xDependencies.size();
        Matcher subMatcher = (n > 0) ? new Matcher(
                this,
                x.getDependenciesAsFunctions(),
                Collections.emptyList(),
                reverse
        ) : this;
        for (Function.Dependency xDep : xDependencies) {
            Function.Dependency dupDep = new Function.Dependency(subMatcher.duplicateDependency(xDep.function, reverse), xDep.explicit);
            assert (subMatcher.matches(xDep.function, dupDep.function, reverse));
            convertedDependencies.add(dupDep);
        }

        // Convert the type (according to the subMatcher!)
        Function convertedType = subMatcher.convert(x.getType(), reverse);

        // Create a new Function
        return new Function(convertedType, convertedDependencies);
    }
}
