package nl.jessetvogel.canard.core;

import java.util.*;
import java.util.stream.Collectors;

public class Matcher {

    final Matcher parent;
    final Collection<Function> indeterminates;
    final Map<Function, Function> solutions;

    private Set<Function> locals = null;
    private Map<Function, Set<Function>> allowedLocals = null;

    public Matcher(Collection<Function> indeterminates) {
        this.parent = null;
        this.indeterminates = indeterminates;
        this.solutions = new HashMap<>();
    }
    
    public Matcher(Matcher parent, Collection<Function> indeterminates) {
        this.parent = parent;
        this.indeterminates = indeterminates;
        this.solutions = new HashMap<>();
    }

    public void setLocalVariables(Set<Function> set, Map<Function, Set<Function>> map) {
        locals = set;
        allowedLocals = map;
    }

    private boolean putSolution(Function x, Function y) {
        // Case x = y: we are not going to map x -> x, but we will return true nonetheless
        if (x == y)
            return true;

        // Case y -> w: we map x -> w as well
        // noinspection SuspiciousNameCombination
        Function w = getSolution(y);
        if (w != null)
            return putSolution(x, w);

        // If x -> z already, do some checks
        // TODO: we must prevent loops "x -> y -> x"
        Function z = solutions.get(x); // NOTE: it is important this is solutions.get and NOT mapSolution! Because a subMatcher may overwrite a mapping
        if (z != null) {
            if (z.equals(y))
                return true;

            // If z is also an indeterminate (possibly of a parent!), then we are satisfied with mapping y -> z
            if (isIndeterminate(z))
                // noinspection SuspiciousNameCombination
                return putSolution(y, z);

            // Finally, if y is an indeterminate, it was apparently not mapped before. Hence we will map y -> x instead
            if (isIndeterminate(y))
                // noinspection SuspiciousNameCombination
                return putSolution(y, x);

//             System.out.println("Nope, " + x + " will not map to " + y + ". It is already mapped to " + z + ", which is NOT an indeterminate!");
            return false;
        }

        // If we are working with local variables, then it may be that y may not depend on some values.
        // Check this.
        if (locals != null && !locals.isEmpty()) {
            Set<Function> allowed = allowedLocals.get(x);
            Set<Function> disallowed = (allowed == null) ? locals : locals.stream().filter(f -> !allowed.contains(f)).collect(Collectors.toUnmodifiableSet());
            if (y.dependsOn(disallowed)) {
//                    System.out.println("Oops, wanted to map " + x + " to " + y + " but it allows on some disallowed value!");
                return false;
            }
        }
        // If all is well, put x -> y.
        solutions.put(x, y);
        return true;
    }

    private boolean isIndeterminate(Function z) {
        for (Matcher m = this; m != null; m = m.parent)
            if (m.indeterminates.contains(z))
                return true;
        return false;
    }

    public Function getSolution(Function x) {
        Function y = solutions.get(x);
        if (y == null)
            return (parent == null) ? null : parent.getSolution(x);

        // Case x -> y -> z
        // noinspection SuspiciousNameCombination
        Function z = getSolution(y);
        return (z == null) ? y : z;
    }

    // TODO: at this point, it is quite more general
    // Checks if g can be applied to f. So 'g is the argument, f the dependency'
    // Actually it checks if a specialization of g can be applied to f, where thmDependencies denote
    // the arguments of the specialization
    public boolean matches(Function f, Function g) {
        // If f equals g, there is obviously a match, and nothing more to do
        if (f == g)
            return true;

        // TODO: optimize matching by first checking with get_solution ?

        // Number of dependencies should agree
        List<Function.Dependency> fDependencies = f.getDependencies(), gDependencies = g.getDependencies();
        int n = fDependencies.size();
        if (n != gDependencies.size()) {
//            System.out.println("Matter of dependencies");
            return false;
        }

        // Dependencies themselves should match

        // TODO: .getDependenciesAsFunctions() is the bottleneck here! Can we change that?

        Matcher subMatcher = (n > 0) ? new Matcher(this, f.getDependenciesAsFunctions()) : this; // Note: there is no need for a subMatcher if f has no dependencies
        for (int i = 0; i < n; ++i) {
            Function.Dependency fDep = fDependencies.get(i);
            Function.Dependency gDep = gDependencies.get(i);

            if (fDep.explicit != gDep.explicit) // explicitness must match
                return false;

            if (!subMatcher.matches(fDep.function, gDep.function))
                return false;
        }

        // Types should match (note: up to the matches already made by subMatcher)
        if (!subMatcher.matches(f.getType(), g.getType()))
            return false;

        // At this point, f and g agree up to their signature, i.e. their dependencies and types match

        // If f (resp. g) is an indeterminate, map f -> g (resp. g -> f).
        // Also treat the case where f or g is an indeterminate of some parent
        for (Matcher m = this; m != null; m = m.parent) {
            if (m.indeterminates.contains(f))
                return m.putSolution(f, g);
            if (m.indeterminates.contains(g))
                return m.putSolution(g, f);
        }

        // If the bases match, simply match the arguments
        // Note that the bases themselves can be indeterminates, so we have to account for that first.
        // Of course it can also be possible that fBase == gBase (this can be checked as Java Objects, since no base function
        // is represented by different Java Objects)
        Function fBase = f.getBase(), gBase = g.getBase();

        // TODO: what if fBase/gBase is an indeterminate, but also it has dependencies ?
        //  At some points problems appear, because there will be multiple solutions..

        if (((indeterminates.contains(fBase) || indeterminates.contains(gBase)) && matches(fBase, gBase)) || fBase == gBase) {
            List<Function> fArguments = f.getArguments(), gArguments = g.getArguments();
            int l = fArguments.size(); // Since the bases already agree, we know the number of arguments must agree as well
            for (int i = 0; i < l; ++i) {
                if (!matches(fArguments.get(i), gArguments.get(i)))
                    return false;
            }
            return true;
        }

//        System.out.println("Oops, " + f + " does not match " + g);
//        System.out.println("... while " + f + "");
        return false;
    }

    public void assertMatch(Function f, Function g) {
//        System.out.println("Must match " + f.toFullString() + " to " + g.toFullString());
        if (!matches(f, g))
            assert false;
    }

    public Function convert(Function x) {
        // If x -> y (we use .mapSolution because it may possibly be mapped by a parent!), return y
        Function y = getSolution(x);
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
                x.getDependenciesAsFunctions()
        ) : this; // If x has no dependencies, no need to construct new dependencies. Duh!
        for (Function.Dependency xDep : xDependencies) {
            Function.Dependency dupDep = new Function.Dependency(subMatcher.clone(xDep.function), xDep.explicit);
            subMatcher.assertMatch(xDep.function, dupDep.function);
            convertedDependencies.add(dupDep);
        }

        // Otherwise, when x is a specialization, we convert each argument of x
        boolean changes = false;
        List<Function> convertedArguments = new ArrayList<>();
        for (Function arg : x.getArguments()) {
            Function convertedArg = subMatcher.convert(arg);
            if (!convertedArg.equals(arg))
                changes = true;
            convertedArguments.add(convertedArg);
        }

        // Note: also possibly the base should be converted!
        Function baseSolution = getSolution(base);
        if (baseSolution != null && baseSolution != base) {
            base = baseSolution; // (silently change the base)
            changes = true;
        }

        // If no actual changes were made to the arguments, we can simply return the original x
        if (!changes)
            return x;

//        System.out.println("Trying to convert " + x.toFullString() + " along " + this);

        // Now we must convert the type of x, this requires another subMatcher!
        // Because the type is determined by the arguments provided to the base Function,
        // since we replace those now, the type should be re-determined
        List<Function> baseExplicitDependencies = base.getExplicitDependencies();
        Matcher subSubMatcher = new Matcher(subMatcher, base.getDependenciesAsFunctions());
        int m = baseExplicitDependencies.size();
        for (int i = 0; i < m; ++i) {
//            System.out.println("ASSERT TO MATCH " + baseExplicitDependencies.get(i).toFullString() + " to " + convertedArguments.get(i).toFullString());
            subSubMatcher.assertMatch(baseExplicitDependencies.get(i), convertedArguments.get(i));
        }
        Function convertedType = subSubMatcher.convert(x.getType());
        return new Specialization(base, convertedArguments, convertedType, convertedDependencies);
    }

    public Function.Dependency convert(Function.Dependency d) {
        Function g = convert(d.function);
        if (g == d.function)
            return d;
        return new Function.Dependency(g, d.explicit);
    }

    public Function clone(Function x) {
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
                x.getDependenciesAsFunctions()
        ) : this;
        for (Function.Dependency xDep : xDependencies) {
            Function.Dependency dupDep = new Function.Dependency(subMatcher.clone(xDep.function), xDep.explicit);
            subMatcher.assertMatch(xDep.function, dupDep.function);
            convertedDependencies.add(dupDep);
        }

        // Convert the type (according to the subMatcher!)
        Function convertedType = subMatcher.convert(x.getType());

        // Create a new Function
        Function y = new Function(convertedType, convertedDependencies);
        y.setLabel(x.label);
        return y;
    }

    public Function cheapClone(Function x) {
        // Should return x itself when x does not depend on any indeterminate (and neither on an indeterminate of some parent)
        Matcher m;
        for (m = this; m != null; m = m.parent) {
            if (x.signatureDependsOn(m.indeterminates))
                break;
        }
        if (m == null)
            return x;
        return clone(x);
    }

    @Override
    public String toString() {
        StringJoiner sj = new StringJoiner(", ", "{", "}");
        for (Map.Entry<Function, Function> entry : solutions.entrySet())
            sj.add(entry.getKey() + " -> " + entry.getValue());
        return sj.toString();
    }

}
