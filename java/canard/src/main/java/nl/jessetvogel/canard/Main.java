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

        // Parse any files given
        for(String arg : args) {
            File file = new File(arg);
            if(!file.exists() || !file.isFile()) {
                System.out.println("could not find file '" + arg + "'");
                return;
            }

            Parser parserFile = new Parser(new FileInputStream(file), System.out, session);
            parserFile.setLocation(file.getAbsoluteFile().getParent(), file.getName());
            parserFile.parse();
        }

        // Parse via System.in
        Parser parser = new Parser(System.in, System.out, session);
        parser.parse();
    }
}
