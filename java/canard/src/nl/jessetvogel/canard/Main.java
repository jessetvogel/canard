package nl.jessetvogel.canard;

import nl.jessetvogel.canard.core.Session;
import nl.jessetvogel.canard.parser.Parser;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;

public class Main {

    public static void main(String[] args) throws FileNotFoundException {
        Session session = new Session();

        // Parse main file
        File mainFile = new File("main.txt");
        Parser parserFile = new Parser(new FileInputStream(mainFile), System.out, session);
        parserFile.setLocation("", "main.txt");
        parserFile.parse();

        // Parse via System.in
        Parser parser = new Parser(System.in, System.out, session);
        while (true) parser.parse();
    }
}
