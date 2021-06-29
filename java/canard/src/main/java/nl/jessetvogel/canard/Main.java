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
        boolean explicit = false;

        for (String arg : args) {
            // Check for options (starting with --)
            if(!arg.startsWith("--"))
                continue;

            if(arg.equals("--json")) {
                format = Parser.Format.JSON;
                continue;
            }

            if(arg.equals("--explicit")) {
                explicit = true;
                continue;
            }

            System.out.println("Invalid option " + arg);
            return;
        }

        long startTime = System.nanoTime();

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
            parserFile.setFormat(format, explicit);
            parserFile.parse();
        }

        long endTime = System.nanoTime();
        System.err.println("Parsed files in " + (endTime - startTime) / 1000000 + " ms");

        // Create parser
        Parser parser = new Parser(System.in, System.out, session);
        parser.setFormat(format, explicit);

        // Keep parsing (until exit) via System.in
        parser.parse();
    }
}
