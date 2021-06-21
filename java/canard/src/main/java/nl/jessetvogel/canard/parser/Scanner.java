package nl.jessetvogel.canard.parser;

import java.io.IOException;
import java.io.InputStream;

class Scanner {

    private final InputStream stream;
    int line = 1, column = 0;
    private Character c0, c1;


    Scanner(InputStream stream) {
        this.stream = stream;
    }

    Character get() throws IOException {
        int r = stream.read();
        if(r == -1)
            return null;

        Character c = (char) r;
        if(c.equals('\n')) {
            line ++;
            column = 0;
        }
        else
            column++;

        c1 = c0;
        c0 = c;
        return c;
    }

    public Character prevChar() {
        return c1;
    }
}
