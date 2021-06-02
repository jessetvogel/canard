package nl.jessetvogel.canard.parser;

import java.io.IOException;
import java.util.List;
import java.util.regex.Pattern;

class Lexer {

    private static final List<String> KEYWORDS = List.of("def", "check", "search", "exit");
    private static final List<String> SEPARATORS = List.of(":", "(", ")", "{", "}", "_", ";");
    private static final Pattern PATTERN_NUMBERS = Pattern.compile("^\\d+$");
    private static final Pattern PATTERN_IDENTIFIERS = Pattern.compile("^\\w+$");
    private static final Pattern PATTERN_STRING = Pattern.compile("^\"[^\"]*\"$");
    private static final Pattern PATTERN_NEWLINE = Pattern.compile("^\\r?\\n$");

    private final Scanner scanner;

    private Token currentToken = null;
    private final StringBuilder sb;
    private int line = 0, column = 1;

    Lexer(Scanner scanner) {
        this.scanner = scanner;
        sb = new StringBuilder();
    }

    Token getToken() throws IOException, LexerException {
        // Read characters until a new Token is produced
        while (true) {
            Character c = scanner.get();

            // End of file
            if (c == null) {
                if (sb.length() == 0)
                    return new Token(Token.Type.EOF, null, line, column);

                if (!tokenize(sb.toString()))
                    throw new LexerException("Unexpected end of file");

                return makeToken();
            }

            // Whitespace: always marks the end of a token (if there currently is one)
            if (c.equals(' ') || c.equals('\t')) {
                if (sb.length() == 0)
                    continue;

                return makeToken();
            }

            // Comments: mark the end of a token (if there currently is one),
            // then continue discarding characters until a newline appears
            if (c.equals('#')) {
                Token token = (sb.length() != 0) ? makeToken() : null;

                do {
                    c = scanner.get();
                } while (c != null && !c.equals('\n'));

                if (c != null)
                    sb.append(c);

                line = scanner.line;
                column = scanner.column;
                tokenize(sb.toString());
                return (token != null) ? token : getToken();
            }

            // If string buffer is empty, set line and column of next token
            if (sb.length() == 0) {
                line = scanner.line;
                column = scanner.column;
            }

            // Enlarge the token if possible
            sb.append(c);

            // If can tokenize, just continue
            if (tokenize(sb.toString()))
                continue;

            // If we also did not tokenize before, hope that it will make sense later
            if (currentToken == null)
                continue;

            // Return the last valid token
            Token token = makeToken();
            sb.append(c);
            line = scanner.line;
            column = scanner.column;
            tokenize(sb.toString());
            return token;
        }
    }

    private boolean tokenize(String str) {
        Token.Type type;
        if (KEYWORDS.contains(str))
            type = Token.Type.KEYWORD;
        else if (SEPARATORS.contains(str))
            type = Token.Type.SEPARATOR;
        else if (PATTERN_NEWLINE.matcher(str).matches())
            type = Token.Type.NEWLINE;
        else if (PATTERN_NUMBERS.matcher(str).matches())
            type = Token.Type.NUMBER;
        else if (PATTERN_IDENTIFIERS.matcher(str).matches())
            type = Token.Type.IDENTIFIER;
        else if (PATTERN_STRING.matcher(str).matches()) {
            type = Token.Type.STRING;
            str = str.substring(1, str.length() - 1);
        } else
            return false;

        // So that there are not constantly made new instances of Token
        if (currentToken != null) {
            currentToken.type = type;
            currentToken.data = str;
        }
        else
            currentToken = new Token(type, str, line, column);
        return true;
    }

    private Token makeToken() throws LexerException {
        if (currentToken == null) {
            String message = "Unknown token '" + sb + "'";
            sb.setLength(0);
            throw new LexerException(message);
        }

        Token token = currentToken;
        currentToken = null;
        sb.setLength(0);
        return token;
    }

    class LexerException extends Exception {

        final int l, c;

        LexerException(String message) {
            super(message);
            l = line;
            c = column;
        }

        public String getMessage() {
            return "" + l + ":" + c + ": " + super.getMessage();
        }

    }
}
