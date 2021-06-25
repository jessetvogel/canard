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

    public static String create(Status status, List<String> keys, List<List<String>> data) {
        int n = keys.size();
        StringJoiner sjList = new StringJoiner(",", "[", "]");
        for(List<String> result : data) {
            StringJoiner sjMap = new StringJoiner(",","{","}");
            for(int i = 0;i < n; ++i)
                sjMap.add("\"" + escape(keys.get(i)) + "\":\"" + escape(result.get(i)) + "\"");
            sjList.add(sjMap.toString());
        }
        return String.format("{\"status\":\"%s\",\"data\":%s}", status.toString().toLowerCase(), sjList);
    }

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
