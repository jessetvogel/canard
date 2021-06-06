CANARD_CODE=$(cat <<-END
    open commutative_algebra
    open algebraic_geometry

    view ;
    exit ;
END
)

echo $CANARD_CODE | java -jar ../bin/canard.jar ../math/main.txt > ../doc/identifiers

echo "Done!"
