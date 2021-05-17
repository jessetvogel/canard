package nl.jessetvogel.canard.core;

import java.util.*;
import java.util.stream.Collectors;

public class Matcher {

    final Matcher parent;
    final List<Function> indeterminates;
    final Map<Function, Function> map;

    Matcher(List<Function> indeterminates) {
        parent = null;
        this.indeterminates = indeterminates;
        map = new HashMap<>();
    }

    Matcher(Matcher parent, List<Function> indeterminates) {
        this.parent = parent;
        this.indeterminates = indeterminates;
        map = new HashMap<>();
    }

    private boolean put(Function f, Function g) {
        Function h = map.get(f);
        if (h == null) {
            map.put(f, g);
            return true;
        }
        return h.equals(g);
    }

    private Function map(Function f) {
        Function g = map.get(f);
        if (g != null)
            return g;
        if (parent != null)
            return parent.map(f);
        return null;
    }

    // What does this function do?
    boolean matches(Function f, Function g) {
        // If f equals g, there is obviously a match, and nothing more to do
        if (f == g)
            return true;

        // Number of dependencies should agree
        List<Function.Dependency> fDependencies = f.getDependencies(), gDependencies = g.getDependencies();
        int n = fDependencies.size();
        if (n != gDependencies.size())
            return false;

        // Dependencies themselves should match
        Matcher subMatcher = (n > 0) ? new Matcher(this, fDependencies.stream().map(d -> d.function).collect(Collectors.toUnmodifiableList())) : this; // Note: there is no need for a subMatcher if f has no dependencies
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
        // Now if f is a dependency, we can simply map f to g.
        if (indeterminates.contains(f))
            return put(f, g);

        // Otherwise, we require a (more) strict match! (E.g. when comparing types)

        // If f was already mapped, it should map to g (up to equality)
        Function h = map(f);
        if (h != null)
            return h.equals(g);

        // Otherwise, the base of f and g should coincide (strict equality of Objects! Because no base function can be represented by two different Objects)
        if (f.getBase() != g.getBase())
            return false;

        List<Function> fArguments = f.getArguments(), gArguments = g.getArguments();
        int l = fArguments.size(); // Since the bases already agree, we know the number of arguments must agree as well
        for (int i = 0; i < l; ++i) {
            Function fArg = fArguments.get(i);
            Function gArg = gArguments.get(i);
            if (!matches(fArg, gArg))
                return false;
        }

        // Note: I think we don't even care about checking anything about dependencies: for one we have already done it,
        // and secondly, what the dependencies are is just a function of the base and its dependencies
        return true;
    }

    public Function convertExpression(Function f) {
        // If f is given as a dependency (possibly to a parent), we can simply map f
        Function g = map(f);
        if (g != null)
            return g;

        Function base = f.getBase();
        if (f == base) // Note that this is actually a special case of the next step!
            return f;

        // Otherwise, when f is a specialization, we convert each argument of f
        boolean changes = false;
        List<Function> convertedArguments = new ArrayList<>();
        for(Function arg : f.getArguments()) {
            Function convertedArg = convertExpression(arg);
            if (!convertedArg.equals(arg))
                changes = true;
            convertedArguments.add(convertedArg);
        }

        // If no actual changes were made to the arguments, we can simply return the original f
        if(!changes)
            return f;

        // Now we must convert the type of f, this requires a subMatcher
        List<Function> baseExplicitDependencies = base.getExplicitDependencies();
        Matcher subMatcher = new Matcher(this, base.getDependencies().stream().map(d -> d.function).collect(Collectors.toUnmodifiableList()));
        int n = baseExplicitDependencies.size();
        for (int i = 0; i < n; ++i)
            assert (subMatcher.matches(baseExplicitDependencies.get(i), convertedArguments.get(i)));

        Function type = subMatcher.convertExpression(f.getType());
        return new Specialization(base, convertedArguments, type, Collections.emptyList());
    }

}
