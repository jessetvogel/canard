package nl.jessetvogel.canard;

import nl.jessetvogel.canard.core.Session;
import nl.jessetvogel.canard.parser.Parser;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;

public class Main {

    public static void main(String[] args) throws FileNotFoundException {
        // Create session
        Session session = new Session();

        // Set options
        Parser.Format format = Parser.Format.PLAIN;
        for (String arg : args) {
            // Check for options (starting with --)
            if(!arg.startsWith("--"))
                continue;

            if(arg.equals("--json")) {
                format = Parser.Format.JSON;
                continue;
            }

            System.out.println("Invalid option " + arg);
            return;
        }

        // Parse files
        for (String arg : args) {
            // (files are the arguments which are not options)
            if(arg.startsWith("--"))
                continue;

            File file = new File(arg);
            if (!file.exists() || !file.isFile()) {
                System.out.println("Could not find file '" + arg + "'");
                return;
            }

            // Create a parser to parse this file
            Parser parserFile = new Parser(new FileInputStream(file), System.out, session);
            parserFile.setLocation(file.getAbsoluteFile().getParent(), file.getName());
            parserFile.setFormat(format);
            parserFile.parse();
        }

        // Create parser
        Parser parser = new Parser(System.in, System.out, session);
        parser.setFormat(format);

        // Keep parsing (until exit) via System.in
        while (true) parser.parse();
    }
}
