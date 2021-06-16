package nl.jessetvogel.canard.core;

import java.util.*;
import java.util.stream.Collectors;

public class Function {

    public String label = null;
    public Namespace space = null;

    private final Function type;
    private final List<Dependency> dependencies;

    public Function(Function type, List<Dependency> dependencies) {
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
//        List<Function> list = new ArrayList<>();
//        for(Dependency dependency : dependencies)
//            list.add(dependency.function);
//        return list;
    }

    public List<Function> getExplicitDependencies() {
        return dependencies.stream().filter(d -> d.explicit).map(d -> d.function).collect(Collectors.toUnmodifiableList());
//        List<Function> list = new ArrayList<>();
//        for(Dependency dependency : dependencies)
//            if(dependency.explicit)
//                list.add(dependency.function);
//        return list;
    }

    public List<Function> getArguments() {
        return getExplicitDependencies();
    }

    public void setLabel(String label) {
        this.label = label;
    }

    public void setNamespace(Namespace space) {
        this.space = space;
    }

    public boolean dependsOn(Collection<Function> coll) {
        return coll.contains(this);
    }

    public boolean signatureDependsOn(List<Function> list) {
        return dependencies.stream().anyMatch(dep -> dep.function.signatureDependsOn(list)) || type.dependsOn(list);
//        for(Dependency dependency : dependencies)
//            if(dependency.function.signatureDependsOn(list))
//                return true;
//        return type.dependsOn(list);
    }

    public Function specialize(List<Function> arguments, List<Function.Dependency> dependencies) throws SpecializationException {
        // Check that the number of arguments equals the number of explicit dependencies
        int n = arguments.size();
        List<Function> explicitDependencies = getExplicitDependencies();
        if(n != explicitDependencies.size())
            throw new SpecializationException("expected " + explicitDependencies.size() + " arguments but received " + n);

        // Base case: if there are no arguments and no given dependencies, immediately return
        if(n == 0 && dependencies.isEmpty())
            return this;

        // Create a matcher that matches the dependencies to the arguments given
        Matcher matcher = new Matcher(getDependenciesAsFunctions());
        for(int i = 0;i < n; ++i) {
            Function dependency = explicitDependencies.get(i);
            Function argument = arguments.get(i);
            if(!matcher.matches(dependency, argument)) {
                StringJoiner sj = new StringJoiner(", ", " where ", "");
                sj.setEmptyValue("");
                for(int j = 0;j < i; ++j)
                    sj.add(explicitDependencies.get(j) + " = " + arguments.get(j));
                throw new SpecializationException("argument '" + argument + "' does not match '" + dependency.toFullString() + "'" + sj);
            }
        }

        // If this is a specialization itself (that is, the base is not equal to this),
        // we convert the arguments to arguments for the base, and then return a specialization of the base.
        // Note this must be done recursively, since specializing the base requires a different Matcher
        // Note also that base must be converted as well! E.g. when specializing 'def dom {T : Type} {X Y : T} (f : Morphism X Y) := X'
        Function base = getBase();
        if(base != this) {
            List<Function> baseArguments = getArguments().stream().map(matcher::convert).collect(Collectors.toUnmodifiableList());
            return matcher.convert(base).specialize(baseArguments, dependencies);
        }

        // TODO: if the list of arguments equals the list of dependencies, we can just return the underlying base function right ??

        // Now we deal with the case where this is a base function:
        // At this point everything is verified.
        // The last thing to do is to construct the type of the specialization.
        Function convertedType = matcher.convert(type);

        // Now create a new specialization with the right inputs
        return new Specialization(this, arguments, convertedType, dependencies);
    }

    @Override
    public String toString() {
        if (label == null)
            return String.format("%04d", hashCode() % 10000);

        if(space == null)
            return label; // + "[" + String.format("%04d", hashCode() % 10000) + "]";

//        return label;

        String path = space.toString();
        return path.isEmpty() ? label : path + "." + label;
    }

    public String toFullString() {
        StringBuilder sb = new StringBuilder();
        sb.append(this);
        for(Function.Dependency d : dependencies)
            sb.append(d.explicit ? " (" : " {").append(d.function.toFullString()).append(d.explicit ? ")" : "}");
        sb.append(" : ");
        sb.append(type);
        if(label != null && getBase() != this)
            sb.append(" := ").append(getBase().toString());
        return sb.toString();
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
        Matcher matcher = new Matcher(getDependenciesAsFunctions());
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

    public static class SpecializationException extends Exception {

        public SpecializationException(String message) {
            super(message);
        }

    }

}
