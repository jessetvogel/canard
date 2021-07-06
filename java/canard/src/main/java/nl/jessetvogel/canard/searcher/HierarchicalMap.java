package nl.jessetvogel.canard.searcher;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

public class HierarchicalMap<K, V> implements Map<K, V> {
    private final Map<? extends K, ? extends V> parent;
    private final HashMap<K, V> map;

    public HierarchicalMap(Map<? extends K, ? extends V> parent) {
        this.parent = parent;
        this.map = new HashMap<K, V>();
    }
    
    public boolean containsKey(Object key) {
        if (map.containsKey(key))
            return true;
        return parent.containsKey(key);
    }

    public V get(Object key) {
        V v = map.get(key);
        if (v != null)
            return v;
        return parent.get(key);
    }

    public V put(K key, V value) {
        return map.put(key, value);
    }

    public void putAll(Map<? extends K, ? extends V> m) {
        map.putAll(m);
    }
    
    public void clear() {
        map.clear();
    }
    
    public boolean containsValue(Object val) {
        if(map.containsValue(val))
            return true;
        return parent.containsValue(val);
    }

    public Set<Entry<K, V>> entrySet() {
        return map.entrySet();
    }

    public boolean isEmpty() {
        if(!map.isEmpty())
            return false;
        return parent.isEmpty();
    }

    public Set<K> keySet() {
        return map.keySet();
    }

    public Collection<V> values() {
        return map.values();
    }

    public V remove(Object key) {
        return map.remove(key);
    }

    public int size() {
        return map.size() + parent.size();
    }

}