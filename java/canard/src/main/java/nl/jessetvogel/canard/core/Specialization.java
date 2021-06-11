package nl.jessetvogel.canard.core;

import java.util.Collection;
import java.util.List;

public class Specialization extends Function {

    private final Function base;
    private final List<Function> arguments;

    public Specialization(Function base, List<Function> arguments, Function type, List<Dependency> dependencies) {
        super(type, dependencies);

        this.base = base;
        this.arguments = arguments;

        assert (base.getBase() == base); // Make sure the contraction of a contraction is simplified!
        assert (arguments.size() == base.getExplicitDependencies().size()); // Number of arguments should equal the number of dependencies of the base function
    }

    @Override
    public Function getBase() {
        return base;
    }

    @Override
    public List<Function> getArguments() {
        return arguments;
    }

    @Override
    public boolean dependsOn(Collection<Function> coll) {
        return arguments.stream().anyMatch(arg -> arg.dependsOn(coll));
    }

    @Override
    public String toString() {
        if (label != null) return label;

        StringBuilder sb = new StringBuilder();
        if (!getDependencies().isEmpty()) {
            sb.append("λ");
            for (Function.Dependency d : getDependencies())
                sb.append(d.explicit ? " (" : " {").append(d.function.toFullString()).append(d.explicit ? ")" : "}");
            sb.append(" := ");
        }

        sb.append(getBase().toString());
        for (Function f : arguments) {
            String strArgument = f.toString();
            if (strArgument.contains(" "))
                strArgument = "(" + strArgument + ")";
            sb.append(" ").append(strArgument);
        }
        return sb.toString();
    }

    @Override
    public String toFullString() {
        return this + " : " + getType();
    }

}
