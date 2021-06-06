package nl.jessetvogel.canard.core;

import java.util.*;

public class Session {

    public final Namespace globalNamespace;
    public final Function TYPE, PROP;

    public Session() {
        // Create the global namespace
        globalNamespace = new Namespace(this);

        // Create an instance of Type and Prop
        TYPE = new Function(null, Collections.emptyList());
        PROP = new Function(TYPE, Collections.emptyList());
        globalNamespace.context.putFunction("Type", TYPE);
        globalNamespace.context.putFunction("Prop", PROP);
    }

    public Function createFunction(Function type, List<Function.Dependency> dependencies) {
        // TODO: if we ever need to deallocate, we can store them in some private list in Session
        return new Function(type, dependencies);
    }

}
