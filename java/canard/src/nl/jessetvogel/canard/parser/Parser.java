package nl.jessetvogel.canard.parser;

import nl.jessetvogel.canard.core.Context;
import nl.jessetvogel.canard.core.Function;
import nl.jessetvogel.canard.core.Session;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class Parser {
    private final OutputStream out;
    private final Session session;

    private final Lexer lexer;
    private String filename, directory;
    private Token currentToken;

    public Parser(InputStream in, OutputStream out, Session session) {
        this.out = out;
        this.session = session;

        lexer = new Lexer(new Scanner(in));
        directory = "";
        filename = "";
        currentToken = null;
    }

    public void setLocation(String directory, String filename) {
        this.directory = directory;
        this.filename = filename;
    }

    // ---- Token methods ----

    private void nextToken() throws IOException, Lexer.LexerException {
        currentToken = lexer.getToken();
        if (currentToken.type == Token.Type.NEWLINE) // Skip newlines
            nextToken();
    }

    private boolean found(Token.Type type) throws IOException, Lexer.LexerException {
        return found(type, null);
    }

    private boolean found(Token.Type type, String data) throws IOException, Lexer.LexerException {
        if (currentToken == null)
            nextToken();
        return type == null || (currentToken.type == type && (data == null || data.equals(currentToken.data)));
    }

    private Token consume() throws IOException, Lexer.LexerException, ParserException {
        return consume(null, null);
    }

    private Token consume(Token.Type type) throws IOException, Lexer.LexerException, ParserException {
        return consume(type, null);
    }

    private Token consume(Token.Type type, String data) throws IOException, Lexer.LexerException, ParserException {
        if (found(type, data)) {
            Token token = currentToken;
            currentToken = null;
            return token;
        } else {
            throw new ParserException(currentToken, String.format("Expected %s (%s) but found %s (%s)", data != null ? data : "\b", type, currentToken.data != null ? currentToken.data : "\b", currentToken.type));
        }
    }

    // ---- Parsing methods ----

    public boolean parse() {
        try {
            while (!found(Token.Type.EOF))
                parseStatement();
            consume(Token.Type.EOF);
        } catch (ParserException e) {
            output("Parsing error: " + e.getMessage());
        } catch (Lexer.LexerException e) {
            output("Lexing error: " + (filename.equals("") ? "" : filename + ":") + e.getMessage());
        } catch (IOException e) {
            output("IOException: " + e.getMessage());
        }

        return true;
    }

    private void parseStatement()  throws ParserException, IOException, Lexer.LexerException {
        /* STATEMENT =
            ; | def | exit | check
         */

        if(found(Token.Type.SEPARATOR, ";")) {
            consume();
            return;
        }

        if(found(Token.Type.KEYWORD, "def")) {
            parseDefinition();
            return;
        }

        if(found(Token.Type.KEYWORD, "exit")) {
            consume();
            System.exit(0);
            return;
        }

        if(found(Token.Type.KEYWORD, "check")) {
            parseCheck();
            return;
        }
    }

    private void parseCheck() throws ParserException, IOException, Lexer.LexerException {
        /*
            check EXPRESSION
        */

        consume(Token.Type.KEYWORD, "check");
        Function f = parseExpression(session.mainContext);
        output(session.toFullString(f));
    }

    private void parseDefinition() throws ParserException, IOException, Lexer.LexerException {
        /*
            def IDENTIFIER : DEPENDENCIES -> FUNCTION
         */

        consume(Token.Type.KEYWORD, "def");
        parseFunctions(session.mainContext);
    }

    private List<Function> parseFunctions(Context context) throws IOException, Lexer.LexerException, ParserException {
        /* FUNCTIONS =
            LIST_OF_IDENTIFIERS ARGUMENTS : OUTPUT_TYPE
         */

        List<String> identifiers = parseListOfIdentifiers();

        Context subContext;
        List<Function> parameters;

        if (found(Token.Type.SEPARATOR, "(") || found(Token.Type.SEPARATOR, "{")) {
            subContext = new Context(context);
            parameters = new ArrayList<>();

            boolean explicit;
            while ((explicit = found(Token.Type.SEPARATOR, "(")) || found(Token.Type.SEPARATOR, "{")) {
                consume();
                parameters.addAll(parseFunctions(subContext));
                consume(Token.Type.SEPARATOR, explicit ? ")" : "}");
            }
        } else {
            subContext = context; // It is unnecessary to define subcontext without its own functions
            parameters = Collections.emptyList();
        }

        consume(Token.Type.SEPARATOR, ":");
        Function type = parseExpression(subContext);
        if(type.getType() != session.TYPE)
            throw new ParserException(currentToken, "expected a type");

        List<Function> output = new ArrayList<>();
        for (String identifier : identifiers) {
            Function f = session.createFunction(type, parameters);
            context.putFunction(identifier, f);
            output.add(f);
        }
        return output;
    }

    private List<String> parseListOfIdentifiers() throws ParserException, IOException, Lexer.LexerException {
        /*  LIST_OF_IDENTIFIERS =
                IDENTIFIER+
         */

        List<String> identifiers = new ArrayList<>();
        do identifiers.add(consume(Token.Type.IDENTIFIER).data);
        while (found(Token.Type.IDENTIFIER));
        return identifiers;
    }

    private Function parseExpression(Context context) throws ParserException, IOException, Lexer.LexerException {
        /* EXPRESSION =
            TERM | TERM TERM+
         */

        // Get a list of following terms
        List<Function> terms = new ArrayList<>();
        Function term;
        while((term = parseTerm(context)) != null)
            terms.add(term);

        // If there are no terms
        int n = terms.size();
        if(n == 0)
            throw new ParserException(currentToken, "expected expression but not found");

        // If there is only one term
        if(n == 1)
            return terms.get(0);

        // If there is more than one term, specialize
        Function f = terms.get(0);
        terms.remove(0);

        List<Function> fDependencies = f.getDependencies();
        int m = fDependencies.size();
        if(m != n - 1)
            throw new ParserException(currentToken, "expected " + m + " arguments for " + f + " but received " + (n - 1));

//        // Replace placeholders by their original dependency
//        List<Function> dependencies = new ArrayList<>();
//        for(int i = 0;i < m; ++i) {
//            Function argument = terms.get(i);
//            if(argument.equals(session.PLACEHOLDER)) {
//                Function g = fDependencies.get(i);
//                // TODO: still need to specialize g to the values that have already been set!
//                //  except if we just get rid of placeholders, because I see no use for them..
//                terms.set(i, g);
//                dependencies.add(g);
//            }
//        }
        try {
            return session.specialize(f, terms, Collections.emptyList());
        } catch (Session.SpecializationException e) {
            throw new ParserException(currentToken, e.getMessage());
        }
    }

    private Function parseTerm(Context context) throws ParserException, IOException, Lexer.LexerException {
        /* TERM =
            IDENTIFIER | ( EXPRESSION ) | _
        */

        if(found(Token.Type.IDENTIFIER)) {
            Token token = consume();
            String identifier = token.data;
            Function f = context.getFunction(identifier);
            if(f == null)
                throw new ParserException(token, "unknown identifier " + identifier);
            return f;
        }

        if (found(Token.Type.SEPARATOR, "(")) {
            consume();
            Function expression = parseExpression(context);
            consume(Token.Type.SEPARATOR, ")");
            return expression;
        }

//        if(found(Token.Type.SEPARATOR, "_")) {
//            consume();
//            return session.PLACEHOLDER;
//        }

        return null;
    }

    // ---- Output method ----

    private void output(String message) {
        try {
            out.write(message.getBytes(StandardCharsets.UTF_8));
            out.write('\n');
        } catch (IOException ignored) {
        }
    }

    // ---- Parser Exceptions ----

    private class ParserException extends Exception {

        private final Token token;

        public ParserException(Token token, String message) {
            super(message);
            this.token = token;
        }

        @Override
        public String getMessage() {
            if (!filename.equals(""))
                return filename + ":" + token.line + ":" + token.position + ": " + super.getMessage();
            return super.getMessage();
        }
    }

}
