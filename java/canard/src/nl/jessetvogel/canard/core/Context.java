package nl.jessetvogel.canard.core;

import java.util.*;

public class Context {

    private final Session session;
    private final Context parent;
    private final Map<String, Function> labels;

    public Context(Context parent) {
        session = parent.session;
        this.parent = parent;
        labels = new HashMap<>();
    }

    public Context(Session session) {
        this.session = session;
        parent = null;
        labels = new HashMap<>();
    }

    public void putFunction(String label, Function f) {
        f.setLabel(label);
        labels.put(label, f);
    }

    public Function getFunction(String label) {
        Function f = labels.get(label);
        if(f != null)
            return f;
        if(parent != null)
            return parent.getFunction(label);
        return null;
    }

}
