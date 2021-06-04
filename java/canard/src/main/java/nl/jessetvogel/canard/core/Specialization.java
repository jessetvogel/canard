package nl.jessetvogel.canard.core;

import java.util.List;

public class Specialization extends Function {

    private final Function base;
    private final List<Function> arguments;

    public Specialization(Function base, List<Function> arguments, Function type, List<Dependency> dependencies) {
        super(type, dependencies);

        this.base = base;
        this.arguments = arguments;

        assert(base.getBase() == base); // Make sure the contraction of a contraction is simplified!
        assert(arguments.size() == base.getExplicitDependencies().size()); // Number of arguments should equal the number of dependencies of the base function
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
    public boolean dependsOn(List<Function> list) {
        return arguments.stream().anyMatch(arg -> arg.dependsOn(list));
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder(label != null ? label : getBase().toString());
        for(Function f : arguments) {
            String strArgument = f.toString();
            if(strArgument.contains(" "))
                strArgument = "(" + strArgument + ")";
            sb.append(" ").append(strArgument);
        }
        return sb.toString();
    }

}
