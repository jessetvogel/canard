<!doctype html>
<html>

<head>
    <?php $MathJax = true;
    include('template/stdhead.php'); ?>
</head>

<body>
    <?php include('template/menu.html'); ?>

    <h2>Help (Advanced)</h2>

    <div id="content" class="mathjax-process">
        The 'Advanced' page allows you to search programmatically for examples of rings, modules, schemes, morphisms having (or not having) certain properties. Moreover, it also allows to search for implications, and for properties of properties.

        <h2>Example 1 (find objects with properties)</h2>
        Let's say we want to search for examples of some mathematical object having some property. Note that this amounts to finding two pieces of data: the mathematical object itself and a proof that that object admits the given property.
        <ul style="line-height: 32px;">
            <li>An <a target="_blank" href="docs.php#algebraic_geometry.scheme.irreducible">irreducible</a> scheme: <code>search (X : Scheme) (h : irreducible X)</code></li>
            <li>A <a target="_blank" href="docs.php#commutative_algebra.ring.domain">domain</a> which is not a <a target="_blank" href="docs.php#commutative_algebra.ring.field">field</a>: <code>search (R : Ring) (h1 : domain R) (h2 : not (field R))</code></li>
            <li>A non-<a target="_blank" href="docs.php#commutative_algebra.module.flat">flat</a> module: <code>search (R : Ring) (M : Module R) (h : not (module.flat (M)))</code></li>
            <li>A non-<a target="_blank" href="docs.php#commutative_algebra.ring.noetherian">noetherian</a> ring: <code>search (R : Ring) (h : not (ring.noetherian R))</code></li>
        </ul>

        <h2>Example 2 (does $P$ imply $Q$ ?)</h2>
        Suppose we have a morphism of schemes $f : X \to Y$ which is <a target="_blank" href="docs.php#algebraic_geometry.morphism.finite">finite</a>. Is it also <a target="_blank" href="docs.php#algebraic_geometry.morphism.proper">proper</a>?
        This amounts to finding a function whose input consists of two schemes, a morphism between them, and a proof that the morphism is finite, and whose output is a proof that the morphism if proper.
        <code class="display">search (i (X Y : Scheme) (f : Morphism X Y) (h : morphism.finite f) : proper f)</code>

        <h2>Example 3 (connected is not Zariski local)</h2>
        The scheme property of being <a target="_blank" href="docs.php#algebraic_geometry.scheme.connected">connected</a> is clearly not Zariski local, but can we construct a counterexample?
        <code class="display">search (h : not (zariski_local connected))</code>



    </div>
</body>

</html>