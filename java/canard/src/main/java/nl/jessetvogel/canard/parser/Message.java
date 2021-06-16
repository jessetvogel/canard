package nl.jessetvogel.canard.parser;

import nl.jessetvogel.canard.core.Function;

import java.util.List;
import java.util.StringJoiner;

public class Message {

    public enum Status { SUCCESS, FAIL, ERROR }

    public static String create(Status status, String data) {
        return String.format("{\"status\":\"%s\",\"data\":\"%s\"}", status.toString().toLowerCase(), escape(data));
    }

    public static String create(Status status, List<String> data) {
        StringJoiner sj = new StringJoiner(",", "[", "]");
        for(String x : data)
            sj.add("\"" + escape(x) + "\"");
        return String.format("{\"status\":\"%s\",\"data\":%s}", status.toString().toLowerCase(), sj);
    }

//    public static String create(Status status, Function data) {
//        return String.format("{\"status\":\"%s\",\"data\":[%s,%s]}", status.toString().toLowerCase(), jsonOfFunction(data), jsonOfFunction(data.getType()));
//    }

    public static String create(Status status, List<Function> keys, List<List<Function>> data) {
        int n = keys.size();
        StringJoiner sjList = new StringJoiner(",", "[", "]");
        for(List<Function> result : data) {
            StringJoiner sjMap = new StringJoiner(",","{","}");
            for(int i = 0;i < n; ++i)
                sjMap.add("\"" + escape(keys.get(i).toString()) + "\":\"" + escape(result.get(i).toFullString()) + "\"");
            sjList.add(sjMap.toString());
        }
        return String.format("{\"status\":\"%s\",\"data\":%s}", status.toString().toLowerCase(), sjList);
    }

//    private static String jsonOfFunction(Function f) {
//        // First construct the body
//        String body;
//        if(f == f.getBase())
//            body = "\"" + escape(f.toString()) + "\"";
//        else {
//            StringJoiner sj = new StringJoiner(",", "[", "]");
//            sj.add("\"" + escape(f.getBase().toString()) + "\"");
//            for (Function arg : f.getArguments())
//                sj.add(jsonOfFunction(arg));
//            body = sj.toString();
//        }
//
//        // If there are no dependencies, just return the body
//        List<Function.Dependency> fDependencies = f.getDependencies();
//        if(fDependencies.isEmpty())
//            return body;
//
//        // Otherwise, [dependencies, body]
//        StringJoiner sj = new StringJoiner(",", "[", "]");
//        for(Function.Dependency dep : f.getDependencies())
//            sj.add(String.format("[%d,%s,%s]", dep.explicit ? 1 : 0, jsonOfFunction(dep.function), jsonOfFunction(dep.function.getType())));
//        return String.format("[%s,%s]", sj, body);
//    }

    private static String escape(String raw) {
        StringBuilder sb = new StringBuilder();
        for (char ch : raw.toCharArray()) {
            if (ch == '\"') sb.append("\\\"");
            else if (ch == '\\') sb.append("\\\\");
            else if (ch == '\b') sb.append("\\b");
            else if (ch == '\f') sb.append("\\f");
            else if (ch == '\n') sb.append("\\n");
            else if (ch == '\r') sb.append("\\r");
            else if (ch == '\t') sb.append("\\t");
            else if (ch >= 0x20 && ch <= 0x7E) sb.append(ch);
            else sb.append(String.format("\\u%04X", (int) ch));
        }
        return sb.toString();
    }

}
