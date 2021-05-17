package nl.jessetvogel.canard;

import nl.jessetvogel.canard.core.Session;
import nl.jessetvogel.canard.parser.Parser;

public class Main {

    public static void main(String[] args) {
        Session session = new Session();

        Parser parser = new Parser(System.in, System.out, session);
        parser.parse();


//        List<Function> empty = Collections.emptyList();
//
//        // Defining a type
//        Function Ring = context.createFunction(context.TYPE, empty);
//
//        // Defining an instance of a type
//        Function ZZ = context.createFunction(Ring, empty);
//
//        // Defining a dependent type, i.e. it has arguments, which themselves are functions
//        Function X = context.createFunction(Ring, empty);
//        Function Y = context.createFunction(Ring, empty);
//        Function RingMorphism = context.createFunction(context.TYPE, Arrays.asList(X, Y));
//
//        // Defining a ring morphism, from ZZ to ZZ (say multiplication by 2)
//        Function HomZZZZ = context.specialize(RingMorphism, Arrays.asList(ZZ, ZZ), empty);
//        Function mult2 = context.createFunction(HomZZZZ, empty);
//
//        // Defining endomorphisms
//        Function Z = context.createFunction(Ring, empty);
//        Function Endomorphism = context.specialize(RingMorphism, Arrays.asList(Z, Z), Collections.singletonList(Z));
//
//        // Context
////        context.functions.add(Ring);
////        context.functions.add(ZZ);
//////        context.functions.add(X);
//////        context.functions.add(Y); // This should not be in the context I think
////        context.functions.add(RingMorphism);
////        context.functions.add(HomZZZZ);
////        context.functions.add(mult2);
////        context.functions.add(Endomorphism);
//
//
//        // Set some labels for readability
//        X.setLabel("X");
//        Y.setLabel("Y");
//        Ring.setLabel("Ring");
//        ZZ.setLabel("ZZ");
//        mult2.setLabel("2");
//        RingMorphism.setLabel("RingMorphism");
//
//        System.out.println(HomZZZZ);
//
//        System.out.println(context.specialize(Endomorphism, Collections.singletonList(Z), empty));

//        // Search for a ring morphism (from some ring A to some ring B)
//        Searcher searcher = new Searcher();
//
//        Function A = new Dummy(Ring);
//        Function B = new Dummy(Ring);
//        Function HomAB = new Specialization(RingMorphism, Arrays.asList(A, B), Arrays.asList(A, B), Type);
//
//        context.setStr(A, "A");
//        context.setStr(B, "B");
//
//        System.out.println(context.str(HomAB));
//        System.out.println(context.strFull(HomAB));
//        System.out.println(context.strFull(RingMorphism));
//
//
//        searcher.findMatch(context, HomAB);
//
//        searcher.findInstance(context, HomAB, new Match());

    }
}
