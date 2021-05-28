package nl.jessetvogel.canard.core;

import java.util.Collections;
import java.util.List;
import java.util.stream.Collectors;

public class Function {

    private String label = "?";

    private final Function type;
    private final List<Dependency> dependencies; // TODO: if empty, can set to null. Just have to change the get-functions

    Function(Function type, List<Dependency> dependencies) {
        this.type = (type != null ? type : this);
        this.dependencies = dependencies;
    }

    public Function getBase() {
        return this;
    }

    public Function getType() {
        return type;
    }

    public List<Dependency> getDependencies() {
        return dependencies;
    }

    public List<Function> getDependenciesAsFunctions() {
        return getDependencies().stream().map(d -> d.function).collect(Collectors.toUnmodifiableList());
    }

    public List<Function> getExplicitDependencies() {
        return dependencies.stream().filter(d -> d.explicit).map(d -> d.function).collect(Collectors.toUnmodifiableList());
    }

    public List<Function> getArguments() {
        return getExplicitDependencies();
    }

    public void setLabel(String label) {
        this.label = label;
    }

    @Override
    public String toString() {
        return label;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == this) return true;
        if (!(obj instanceof Function)) return false;

        Function other = (Function) obj;
        if (other.getBase() != getBase()) return false;

        List<Dependency> thisDependencies = getDependencies(), otherDependencies = other.getDependencies();
        int n = thisDependencies.size();
        if (n != otherDependencies.size())
            return false;

        // If there are no dependencies, check equality on the arguments
        if (n == 0) {
            List<Function> thisArguments = getArguments(), otherArguments = other.getArguments();
            int l = thisArguments.size();
            for (int i = 0; i < l; ++i)
                if (!thisArguments.get(i).equals(otherArguments.get(i)))
                    return false;
            return true;
        }

        // If there are dependencies, we need a Matcher
        Matcher matcher = new Matcher(getDependenciesAsFunctions(), Collections.emptyList());
        for (int i = 0; i < n; i++) {
            Dependency thisDep = thisDependencies.get(i), otherDep = otherDependencies.get(i);
            if (thisDep.explicit != otherDep.explicit) // Explicitness must match
                return false;
            if (!matcher.matches(thisDep.function, thisDep.function))
                return false;
        }

        // Now just let the matcher do its work
        return matcher.matches(this, other);
    }

    public static class Dependency {

        public final Function function;
        public final boolean explicit;

        public Dependency(Function function, boolean explicit) {
            this.function = function;
            this.explicit = explicit;
        }

    }

}
