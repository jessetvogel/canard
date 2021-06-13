package nl.jessetvogel.canard.parser;

import nl.jessetvogel.canard.core.Context;
import nl.jessetvogel.canard.core.Function;
import nl.jessetvogel.canard.core.Namespace;
import nl.jessetvogel.canard.core.Session;
import nl.jessetvogel.canard.search.Query;
import nl.jessetvogel.canard.search.Searcher;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.*;
import java.util.stream.Collectors;

public class Parser {
    private final OutputStream out;
    private final Lexer lexer;
    private String filename, directory;
    private Token currentToken;

    private final Session session;
    private Namespace currentNamespace;
    private final Set<Namespace> openNamespaces;
    private List<String> importedFiles;

    public enum Format { PLAIN, JSON };
    private Format format = Format.PLAIN;

    public Parser(InputStream in, OutputStream out, Session session) {
        this.out = out;
        lexer = new Lexer(new Scanner(in));
        directory = "";
        filename = "";
        currentToken = null;

        this.session = session;
        this.currentNamespace = session.globalNamespace;
        this.openNamespaces = new HashSet<>();
        this.importedFiles = new ArrayList<>();
    }

    public void setFormat(Format format) {
        this.format = format;
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
            throw new ParserException(currentToken, String.format("expected %s (%s) but found %s (%s)", data != null ? data : "\b", type, currentToken.data != null ? currentToken.data : "\b", currentToken.type));
        }
    }

    // ---- Parsing methods ----

    public boolean parse() {
        try {
            while (parseStatement()) ;
            if (!found(Token.Type.EOF))
                throw new ParserException(currentToken, "expected statement, got " + consume().data);
            consume(Token.Type.EOF);
            return true;
        } catch (ParserException e) {
            if (filename.equals(""))
                error("Parsing error: " + e.getMessage());
            else
                error("Parsing error in " + filename + " at " + e.token.line + ":" + e.token.position + ": " + e.getMessage());
            return false;
        } catch (Lexer.LexerException e) {
            if (filename.equals(""))
                error("Lexing error: " + e.getMessage());
            else
                error("Lexing error in " + filename + " at " + e.l + ":" + e.c + ": " + e.getMessage());
            return false;
        } catch (IOException e) {
            error("IOException: " + e.getMessage());
            return false;
        }
    }

    private boolean parseStatement() throws ParserException, IOException, Lexer.LexerException {
        /* STATEMENT =
            ; | def | search | check | exit
         */

        if (found(Token.Type.SEPARATOR, ";")) {
            consume();
            return true;
        }

        if (found(Token.Type.KEYWORD, "let")) {
            parseDeclaration();
            return true;
        }

        if (found(Token.Type.KEYWORD, "def")) {
            parseDefinition();
            return true;
        }

        if (found(Token.Type.KEYWORD, "check")) {
            parseCheck();
            return true;
        }

        if (found(Token.Type.KEYWORD, "search")) {
            parseSearch();
            return true;
        }

        if (found(Token.Type.KEYWORD, "import")) {
            parseImport();
            return true;
        }

        if (found(Token.Type.KEYWORD, "namespace")) {
            parseNamespace();
            return true;
        }

        if (found(Token.Type.KEYWORD, "open")) {
            parseOpen();
            return true;
        }

        if (found(Token.Type.KEYWORD, "close")) {
            parseClose();
            return true;
        }

        if (found(Token.Type.KEYWORD, "exit")) {
            consume();
            System.exit(0);
            return true;
        }

        if (found(Token.Type.KEYWORD, "inspect")) {
            parseInspect();
            return true;
        }

        return false;
    }

    private void parseInspect() throws ParserException, IOException, Lexer.LexerException {
        /*
            inspect NAMESPACE
        */

        Token tInspect = consume(Token.Type.KEYWORD, "inspect");

        // Get namespace
        String path = parsePath();
        Namespace space = session.globalNamespace.getNamespace(path);
        if(space == null)
            throw new ParserException(tInspect, "unknown namespace '" + path + "'");

        if(format == Format.JSON)
            output(Message.create(Message.Status.SUCCESS, space.context.getFunctions().stream().map(Function::toString).collect(Collectors.toUnmodifiableList())));
        else
            for (Function f : space.context.getFunctions())
                output(f.toString());
    }

    private void parseOpen() throws ParserException, IOException, Lexer.LexerException {
        /*
            open NAMESPACE
         */

        Token tOpen = consume(Token.Type.KEYWORD, "open");
        String name = consume(Token.Type.IDENTIFIER).data;
        Namespace space = session.globalNamespace.getNamespace(name);
        if (space == null)
            throw new ParserException(tOpen, "invalid namespace name '" + name + "'");
        openNamespaces.add(space);
    }

    private void parseClose() throws ParserException, IOException, Lexer.LexerException {
        /*
            close NAMESPACE
         */

        Token tClose = consume(Token.Type.KEYWORD, "close");
        String name = consume(Token.Type.IDENTIFIER).data;
        Namespace space = session.globalNamespace.getNamespace(name);
        if (space == null)
            throw new ParserException(tClose, "invalid namespace name '" + name + "'");
        openNamespaces.remove(space);
    }

    private void parseImport() throws ParserException, IOException, Lexer.LexerException {
        /*
            import "PATH"
        */

        Token tImport = consume(Token.Type.KEYWORD, "import");
        String filename = consume(Token.Type.STRING).data;

        // Check if absolute or relative path
        File file = new File(filename);
        if (!file.isAbsolute())
            file = new File(directory + File.separator + filename);
        if (!file.isFile())
            throw new ParserException(tImport, "file '" + filename + "' not found");

        // Check if the file was already imported before, based on normalized absolute path of file
        String normalizedPath = (Path.of(file.getAbsolutePath())).normalize().toString();
        if (importedFiles.contains(normalizedPath))
            return;

        // Add the absolute path to list of imported files
        importedFiles.add(normalizedPath);

        // Create subParser to parse the file
        Parser subParser = new Parser(new FileInputStream(file), out, session);
        subParser.importedFiles = importedFiles; // (please use my list of imported files)
        subParser.setLocation(file.getAbsoluteFile().getParent(), file.getName());
        if (!subParser.parse())
            throw new ParserException(tImport, "error in importing '" + filename + "'");
    }

    private void parseNamespace() throws ParserException, IOException, Lexer.LexerException {
        /*
            namespace IDENTIFIER
                STATEMENT*
            end
        */

        consume(Token.Type.KEYWORD, "namespace");
        String name = consume(Token.Type.IDENTIFIER).data;
        // If namespace already exists, use that one. Otherwise, create a new one
        Namespace space = currentNamespace.getNamespace(name);
        currentNamespace = (space != null) ? space : new Namespace(currentNamespace, name);

        while (parseStatement()) ;

        consume(Token.Type.KEYWORD, "end");
        consume(Token.Type.IDENTIFIER, name);
        currentNamespace = currentNamespace.getParent();
    }

    private void parseSearch() throws ParserException, IOException, Lexer.LexerException {
        /*
            search LIST_OF_PARAMETERS
         */

        consume(Token.Type.KEYWORD, "search");

        // Parse indeterminates
        List<Function> indeterminates = new ArrayList<>();
        Context subContext = new Context(currentNamespace.context);
        while (found(Token.Type.SEPARATOR, "(")) {
            consume();
            indeterminates.addAll(parseFunctions(subContext));
            consume(Token.Type.SEPARATOR, ")");
        }

        // Number of results wanted
        int N = found(Token.Type.NUMBER) ? Integer.parseInt(consume().data) : 1;

        // Create searcher and specify the searching space
        Set<Namespace> searchSpace = new HashSet<>(openNamespaces);
        for (Namespace space = currentNamespace; space != null; space = space.getParent())
            searchSpace.add(space);
        Searcher searcher = new Searcher(searchSpace, 5); // TODO: watch out, its a magic number! 🪄

        // Make a query and do a search
        // Store the results in a list
        List<List<Function>> results = new ArrayList<>();
        Query query = new Query(indeterminates);
        List<Function> solution = searcher.search(query);
        if (solution != null)
            results.add(solution);
        for (int i = 1; i < N && solution != null; ++i) {
            solution = searcher.next();
            if (solution != null)
                results.add(solution);
        }

        // Print the solutions
        if(format == Format.JSON)
            output(Message.create(Message.Status.SUCCESS, indeterminates, results));
        else {
            if (results.isEmpty()) {
                output("\uD83E\uDD7A no solutions found");
                return;
            }

            for (List<Function> result : results) {
                StringJoiner sj = new StringJoiner(", ");
                int n = result.size();
                for (int i = 0; i < n; ++i)
                    sj.add(indeterminates.get(i) + " = " + result.get(i));
                output("\uD83D\uDD0E " + sj);
            }
        }
    }

    private void parseCheck() throws ParserException, IOException, Lexer.LexerException {
        /*
            check EXPRESSION
        */

        consume(Token.Type.KEYWORD, "check");
        Function f = parseExpression(currentNamespace.context);

        if(format == Format.JSON)
            output(Message.create(Message.Status.SUCCESS, f.toFullString()));
        else
            output("\uD83E\uDD86 " + f.toFullString());
    }

    private void parseDeclaration() throws ParserException, IOException, Lexer.LexerException {
        /*
            let IDENTIFIER DEPENDENCIES : TYPE
         */

        consume(Token.Type.KEYWORD, "let");
        parseFunctions(currentNamespace.context);
    }

    private void parseDefinition() throws ParserException, IOException, Lexer.LexerException {
        /*
            def IDENTIFIER DEPENDENCIES := EXPRESSION
         */

        consume(Token.Type.KEYWORD, "def");
        String identifier = consume(Token.Type.IDENTIFIER).data;

        // Parse dependencies
        Context context = currentNamespace.context;
        Context subContext = new Context(context);
        List<Function.Dependency> dependencies = parseDependencies(subContext);
        if (dependencies.isEmpty()) // (if there are no dependencies, don't need a subContext)
            subContext = context;

        consume(Token.Type.SEPARATOR, ":=");

        Function f = parseExpression(subContext, dependencies);
        if (!context.putFunction(identifier, f))
            throw new ParserException(currentToken, "name " + identifier + " already used in this context");
    }

    private List<Function> parseFunctions(Context context) throws IOException, Lexer.LexerException, ParserException {
        /* FUNCTIONS =
            LIST_OF_IDENTIFIERS PARAMETERS : OUTPUT_TYPE
         */

        List<String> identifiers = parseListOfIdentifiers();

        // Parse dependencies
        Context subContext = new Context(context);
        List<Function.Dependency> dependencies = parseDependencies(subContext);
        if (dependencies.isEmpty()) // (if there are no dependencies, don't need a subContext)
            subContext = context;

        // Parse type
        consume(Token.Type.SEPARATOR, ":");
        Token token = currentToken;
        Function type = parseExpression(subContext);
        if (type.getType() != session.TYPE && type.getType() != session.PROP)
            throw new ParserException(token, "expected a Type or Prop");
        if (type.getDependencies().size() > 0)
            throw new ParserException(token, "forgot arguments");

        // Actually create the functions
        List<Function> output = new ArrayList<>();
        for (String identifier : identifiers) {
            Function f = session.createFunction(type, dependencies);
            if (!context.putFunction(identifier, f))
                throw new ParserException(currentToken, "name " + identifier + " already used in this context");
            output.add(f);
        }

        return output;
    }

    private List<Function.Dependency> parseDependencies(Context subContext) throws ParserException, IOException, Lexer.LexerException {
        List<Function.Dependency> dependencies = Collections.emptyList();
        if (found(Token.Type.SEPARATOR, "{") || found(Token.Type.SEPARATOR, "(")) {
            dependencies = new ArrayList<>();
            boolean explicit;
            while ((explicit = found(Token.Type.SEPARATOR, "(")) || found(Token.Type.SEPARATOR, "{")) {
                consume();
                for (Function f : parseFunctions(subContext))
                    dependencies.add(new Function.Dependency(f, explicit));
                consume(Token.Type.SEPARATOR, explicit ? ")" : "}");
            }
        }

        // Check if the implicits were actually used
        for (Function.Dependency d : dependencies) {
            if (!d.explicit && !subContext.isUsed(d.function))
                throw new ParserException(currentToken, "implicit parameter " + d.function + " unused");
        }

        return dependencies;
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
        return parseExpression(context, Collections.emptyList());
    }

    private Function parseExpression(Context context, List<Function.Dependency> dependencies) throws ParserException, IOException, Lexer.LexerException {
        /* EXPRESSION =
            TERM | TERM TERM+
         */

        // Get a list of following terms
        List<Function> terms = new ArrayList<>();
        Function term;
        while ((term = parseTerm(context)) != null)
            terms.add(term);

        // If there are no terms
        int n = terms.size();
        if (n == 0)
            throw new ParserException(currentToken, "expected expression but not found");

        // If there is only one term and no dependencies, return that term
        // If there are dependencies, specialize the term with its own arguments, but with dependencies!
        if (n == 1) {
            Function f = terms.get(0);
            if (dependencies.isEmpty())
                return f;
            try {
                return f.specialize(f.getArguments(), dependencies);
            } catch (Function.SpecializationException e) {
                throw new ParserException(currentToken, e.getMessage());
            }
        }

        // If there is more than one term, specialize
        Function f = terms.get(0);
        terms.remove(0);
        try {
            return f.specialize(terms, dependencies);
        } catch (Function.SpecializationException e) {
            throw new ParserException(currentToken, e.getMessage());
        }
    }

    private Function parseTerm(Context context) throws ParserException, IOException, Lexer.LexerException {
        /* TERM =
            PATH | ( EXPRESSION )
        */

        if (found(Token.Type.IDENTIFIER)) {
            Token token = currentToken;
            String path = parsePath();
            // First look through the context
            Function f = context.getFunction(path);
            if (f != null)
                return f;
            // Then look through namespaces from the current one to the top
            for (Namespace space = currentNamespace; space != null; space = space.getParent()) {
                f = space.getFunction(path);
                if (f != null)
                    return f;
            }
            // If that failed, look through the other open namespaces
            List<Function> candidates = new ArrayList<>();
            for (Namespace space : openNamespaces) {
                f = space.getFunction(path);
                if (f != null)
                    candidates.add(f);
            }
            // If there is a unique candidate, we are good
            if (candidates.size() == 1)
                return candidates.get(0);
            // If more than one candidate, its ambiguous
            if (candidates.size() > 1) {
                // TODO: show what the options are, but then should remember the namespaces
                throw new ParserException(token, "ambiguous identifier '" + path + "'");
            }
            // Otherwise, if no candidates, throw unknown
            throw new ParserException(token, "unknown identifier '" + path + "'");
        }

        if (found(Token.Type.SEPARATOR, "(")) {
            consume();
            Function expression = parseExpression(context);
            consume(Token.Type.SEPARATOR, ")");
            return expression;
        }

        return null;
    }

    private String parsePath() throws ParserException, IOException, Lexer.LexerException {
        /*
            PATH = IDENTIFIER ( . IDENTIFIER )*
         */

        StringBuilder sb = new StringBuilder();
        sb.append(consume(Token.Type.IDENTIFIER).data);
        while (found(Token.Type.SEPARATOR, ".")) {
            sb.append(consume().data);
            sb.append(consume(Token.Type.IDENTIFIER).data);
        }
        return sb.toString();
    }

    // ---- Output method ----

    private void output(String message) {
        try {
            out.write(message.getBytes(StandardCharsets.UTF_8));
            out.write('\n');
        } catch (IOException ignored) {
        }
    }

    private void error(String message) {
        if(format == Format.JSON)
            output(Message.create(Message.Status.ERROR, message));
        else
            output("⚠️ " + message);
    }

    // ---- Parser Exceptions ----

    private static class ParserException extends Exception {

        private final Token token;

        public ParserException(Token token, String message) {
            super(message);
            this.token = token;
        }
    }

}
