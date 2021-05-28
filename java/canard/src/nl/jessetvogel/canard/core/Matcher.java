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

    Matcher(Matcher parent, List<Function> fIndeterminates, List<Function> gIndeterminates) {
        this.parent = parent;
        this.fIndeterminates = fIndeterminates;
        this.gIndeterminates = gIndeterminates;
        fSolutions = new HashMap<>();
        gSolutions = new HashMap<>();
    }

    private boolean putGSolution(Function g, Function f) {
        Function h = gSolutions.get(g);
        if (h == null) {
            gSolutions.put(g, f);
            return true;
        }
        return h.equals(f);
    }

    private boolean putFSolution(Function f, Function g) {
        Function h = fSolutions.get(f);
        if (h == null) {
            fSolutions.put(f, g);
            return true;
        }

        if(!h.equals(g)) {
            System.err.println("Hmm. Want to map " + f + " to " + g + ", but was already mapped to " + h);
            return false;
        }
        return true;
    }

    public Function mapGSolution(Function g) {
        Function f = gSolutions.get(g);
        if (f != null)
            return f;
        if (parent != null)
            return parent.mapGSolution(g);
        return null;
    }

    public Function mapFSolution(Function f) {
        Function g = fSolutions.get(f);
        if (g != null)
            return g;
        if (parent != null)
            return parent.mapFSolution(f);
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
                Collections.emptyList() // One way matching is good enough here! TODO: don't know about this one ??
        ) : this; // Note: there is no need for a subMatcher if f has no dependencies
        for (int i = 0; i < n; ++i) {
            Function.Dependency fDep = fDependencies.get(i);
            Function.Dependency gDep = gDependencies.get(i);

            if(fDep.explicit != gDep.explicit) // Explicitness must match
                return false;

            if (!subMatcher.matches(fDep.function, gDep.function))
                return false;
        }

        // Types should match (note: up to the matches already made by subMatcher)
        if (!subMatcher.matches(f.getType(), g.getType()))
            return false;

        // At this point, f and g agree up to their signature, i.e. their dependencies and types match

        // If f is a parameter of thm, fill in g as argument
        if(gIndeterminates.contains(g))
            return putGSolution(g, f);

        // If g is an indeterminate, assign f to g
        if(fIndeterminates.contains(f))
            return putFSolution(f, g);

        // If the bases match, simply match the arguments (note: we check for a strict equality of Objects! Because no base function can be represented by two different Objects)
        if(f.getBase() == g.getBase()) {
            List<Function> fArguments = f.getArguments(), gArguments = g.getArguments();
            int l = fArguments.size(); // Since the bases already agree, we know the number of arguments must agree as well
            for (int i = 0; i < l; ++i) {
                Function fArg = fArguments.get(i);
                Function gArg = gArguments.get(i);
                if (!matches(fArg, gArg))
                    return false;
            }
            return true;
        }

        // Last chance: the parent might let them match, because f might be an indeterminate of some (grand)parent instead!
        if(parent != null)
            return parent.matches(f, g); // TODO: this is necessary, but can it be simplified ?? If it only applies when f is an indeterminate, can check for that instead?
        //  otherwise we must check all the above once again (e.g. types) which is totally unnecessary!

//        System.out.println("Oops, " + f + " does not match " + g);
        return false;
    }

    public Function convertGtoF(Function g) {
        // If g is given as an argument (possibly to a parent), we can simply map g
        Function f = mapGSolution(g);
        if (f != null)
            return f;

        Function base = g.getBase();
        if (g == base) // Note that this is actually a special case of the next step!
            return g;

        // Otherwise, when g is a specialization, we convert each argument of g
        boolean changes = false;
        List<Function> convertedArguments = new ArrayList<>();
        for(Function arg : g.getArguments()) {
            Function convertedArg = convertGtoF(arg);
            if (!convertedArg.equals(arg))
                changes = true;
            convertedArguments.add(convertedArg);
        }

        // If no actual changes were made to the arguments, we can simply return the original g
        if(!changes)
            return g;

        // Now we must convert the type of g, this requires a subMatcher
        List<Function> baseExplicitDependencies = base.getExplicitDependencies();
        Matcher subMatcher = new Matcher(this, base.getDependenciesAsFunctions(), Collections.emptyList());
        int n = baseExplicitDependencies.size();
        for (int i = 0; i < n; ++i)
            assert (subMatcher.matches(baseExplicitDependencies.get(i), convertedArguments.get(i)));

        Function type = subMatcher.convertGtoF(g.getType());
        return new Specialization(base, convertedArguments, type, Collections.emptyList());
    }

    public Function convertFtoG(Function f) {
        // If f is given as an argument (possibly to a parent), we can simply map f
        Function g = mapFSolution(f);
        if (g != null)
            return g;

        Function base = f.getBase();
        if (f == base) // Note that this is actually a special case of the next step!
            return f;

        // Otherwise, when g is a specialization, we convert each argument of g
        boolean changes = false;
        List<Function> convertedArguments = new ArrayList<>();
        for(Function arg : f.getArguments()) {
            Function convertedArg = convertFtoG(arg);
            if (!convertedArg.equals(arg))
                changes = true;
            convertedArguments.add(convertedArg);
        }

        // If no actual changes were made to the arguments, we can simply return the original g
        if(!changes)
            return f;

        // Now we must convert the type of g, this requires a subMatcher
        List<Function> baseExplicitDependencies = base.getExplicitDependencies();
        Matcher subMatcher = new Matcher(this, base.getDependenciesAsFunctions(), Collections.emptyList());
        int n = baseExplicitDependencies.size();
        for (int i = 0; i < n; ++i)
            assert (subMatcher.matches(baseExplicitDependencies.get(i), convertedArguments.get(i)));

        Function type = subMatcher.convertFtoG(f.getType());
        return new Specialization(base, convertedArguments, type, Collections.emptyList());
    }
}
