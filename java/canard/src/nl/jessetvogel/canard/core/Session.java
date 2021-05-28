package nl.jessetvogel.canard.core;

import java.util.*;
import java.util.stream.Collectors;

public class Session {

    public final Context mainContext;

    public final Function TYPE;

    public Session() {
        // Create main context
        mainContext = new Context(this);

        // Create an instance of TYPE
        TYPE = new Function(null, Collections.emptyList());
        mainContext.putFunction("Prop", TYPE);
        mainContext.putFunction("Type", TYPE);
    }

    public Function createFunction(Function type, List<Function.Dependency> dependencies) {
        // TODO: if we ever need to deallocate, we can store them in some private list in Session
        return new Function(type, dependencies);
    }

    public Function specialize(Function parent, List<Function> arguments, List<Function.Dependency> dependencies) throws SpecializationException {
        // Make some assertions: arguments must match the dependencies of the base
        int n = arguments.size();
        List<Function> parentExplicitDependencies = parent.getExplicitDependencies();
        assert(n == parentExplicitDependencies.size());

        // Base case: if there are no arguments, immediately return
        if(n == 0)
            return parent;

        // If parent is a specialization itself (that is, its base is not equal to itself),
        // we first specialize the arguments to arguments for the base, and then return
        // a specialization of the base.
        Function base = parent.getBase();
        if(!base.equals(parent)) {
            List<Function> baseArguments = new ArrayList<>();
            for(Function f : parent.getArguments()) {
                // When f is itself a dependency of parent, return the corresponding argument
                int i = parentExplicitDependencies.indexOf(f);
                if(i >= 0) {
                    baseArguments.add(arguments.get(i));
                    continue;
                }

                // Otherwise, when f is *not* a dependency, we must specialize f:
                // - The arguments applied to f is the sublist of 'arguments' corresponding to
                //   the dependencies of f which is (by assertion) a sublist of the dependencies of parent
                List<Function> subArguments = f.getDependencies().stream().map(g -> {
                    int j = parentExplicitDependencies.indexOf(g.function);
                    assert(j >= 0);
                    return arguments.get(j);
                }).collect(Collectors.toUnmodifiableList());

                // - The dependencies of the specialization of f are the sublist of 'dependencies'
                //   that appear in the dependencies of the arguments (TODO: we must be careful here ? there might be a problem)
                List<Function.Dependency> subDependencies = dependencies.stream().filter(g -> {
                    for(Function h : subArguments) {
                        if(h.getDependencies().contains(g))
                            return true;
                    }
                    return false;
                }).collect(Collectors.toUnmodifiableList());

                baseArguments.add(specialize(f, subArguments, subDependencies));
            }
            return specialize(base, baseArguments, dependencies);
        }

        // Now we deal with the case where parent is a base function:
        // Check if the type of the given arguments match the dependencies
        Matcher matcher = new Matcher(parent.getDependenciesAsFunctions(), Collections.emptyList());
        for(int i = 0;i < n; ++i) {
            Function dependency = parentExplicitDependencies.get(i);
            Function argument = arguments.get(i);
            if(!matcher.matches(dependency, argument)) {
                StringJoiner sj = new StringJoiner(", ", " where ", "");
                sj.setEmptyValue("");
                for(int j = 0;j < i; ++j)
                    sj.add(parentExplicitDependencies.get(j) + " = " + arguments.get(j));
                throw new SpecializationException("argument '" + argument + "' does not match '" + toFullString(dependency) + "'" + sj);
            }
        }

        // TODO: if the list of arguments equals the list of dependencies, we can just return the underlying base function right ??

        // At this point everything is verified.
        // The last thing to do is to construct the type of the specialization.
        Function type = matcher.convertFtoG(parent.getType());

        // Now create a new specialization with the right input
        return new Specialization(parent, arguments, type, dependencies);
    }

    public String toFullString(Function f) {
        StringBuilder sb = new StringBuilder();
        sb.append(f);
        for(Function.Dependency d : f.getDependencies())
            sb.append(d.explicit ? " (" : " {").append(toFullString(d.function)).append(d.explicit ? ")" : "}");
        sb.append(" : ");
        sb.append(f.getType());
        return sb.toString();
    }

    public static class SpecializationException extends Exception {

        public SpecializationException(String message) {
            super(message);
        }

    }
}
