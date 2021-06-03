package nl.jessetvogel.canard.core;

import java.util.HashMap;
import java.util.Map;

public class Namespace {
    private final Session session;
    private final Namespace parent;
    private final Map<String, Namespace> children;

    public final String name;
    public final Context context;

    Namespace(Session session) {
        // Creates the global namespace
        this.session = session;
        this.parent = null;
        this.name = "";
        this.context = new Context(session);
        this.children = new HashMap<>();
    }

    public Namespace(Namespace parent, String name) {
        // Creates a sub-namespace
        this.session = parent.session;
        this.parent = parent;
        this.name = name;
        this.context = new Context(session);
        this.children = new HashMap<>();

        // Add me to the list of my parents children
        parent.children.put(name, this);
    }

    public Function getFunction(String path) {
        int i = path.indexOf(".");
        if(i < 0) // if path is a genuine label
            return context.getFunction(path);

        // Otherwise, ask a child
        Namespace subSpace = children.get(path.substring(0, i));
        return (subSpace == null) ? null : subSpace.getFunction(path.substring(i + 1));
    }

    public Namespace getParent() {
        return parent;
    }

    public Namespace getNamespace(String path) {
        if(path.equals("")) // if path is empty, should return this
            return this;

        int i = path.indexOf(".");
        if(i < 0) // if path has no dots, return one of the children
            return children.get(path);

        // Otherwise, ask a child
        Namespace subSpace = children.get(path.substring(0, i));
        return (subSpace == null) ? null : subSpace.getNamespace(path.substring(i + 1));
    }
}
