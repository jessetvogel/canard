package nl.jessetvogel.canard.core;

import java.util.List;

public class Function {

    private String label = "?";

    private final Function type;
    private final List<Function> dependencies;

    Function(Function type, List<Function> dependencies) {
        this.type = (type != null ? type : this);
        this.dependencies = dependencies;
    }

    public Function getType() {
        return type;
    }

    public Function getBase() {
        return this;
    }

    public List<Function> getDependencies() {
        return dependencies;
    }

    public List<Function> getArguments() {
        return dependencies;
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

        List<Function> thisDependencies = getDependencies(), otherDependencies = other.getDependencies();
        int n = thisDependencies.size();
        if (n != otherDependencies.size())
            return false;

        // If there are no dependencies, check equality on the arguments
        if (n == 0) {
            List<Function> thisArguments = getArguments(), otherArguments = other.getArguments();
            int m = thisArguments.size();
            for (int i = 0; i < m; ++i)
                if (!thisArguments.get(i).equals(otherArguments.get(i)))
                    return false;
            return true;
        }

        // If there are dependencies, we need a Matcher
        Matcher matcher = new Matcher(thisDependencies);
        for(int i = 0;i < n; i++) {
            if(!matcher.matches(thisDependencies.get(i), otherDependencies.get(i)))
                return false;
        }

        // Now just let the matcher do its work
        return matcher.matches(this, other);
    }

}
