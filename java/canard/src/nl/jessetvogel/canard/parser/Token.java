package nl.jessetvogel.canard.parser;

public class Token {

    enum Type {
        IDENTIFIER,
        KEYWORD,
        SEPARATOR,
        EOF,
        NEWLINE,
        NUMBER,
        STRING
    }

    public Type type;
    public String data;
    public int line, position;

    Token(Type type, String data, int line, int position) {
        this.type = type;
        this.data = data;
        this.line = line;
        this.position = position;
    }

}
