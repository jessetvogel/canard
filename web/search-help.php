<!doctype html>
<html>

<head>
    <?php $MathJax = true;
    include('template/stdhead.php'); ?>
</head>

<body>
    <?php include('template/menu.html'); ?>

    <h2>Help (Search)</h2>

    <div id="content" class="mathjax-process">
        The 'Search' page allows you to search for examples of rings, modules, schemes, etc. (and morphisms thereof) having (or not having) certain properties.

        <h2>Example 1 (non-separated schemes)</h2>
        Let's say we want to find examples of non-<a target="_blank" href="docs.php#algebraic_geometry.scheme.separated">separated</a> schemes.
        <ol style="line-height: 24px;">
            <li>Type a name for your scheme, e.g. $X$, select the type <span style="display: inline-block; background-color: white; box-shadow: 0px 0px 5px rgba(0, 0, 0, 0.12); padding: 4px 12px; border-radius: 4px;">Scheme</span> and click <button>+add</button>.</li>
            <li>A list of schemes properties appears. You can click these properties to indicate that your scheme $X$ should (or should not) have one or more of these properties. In this case, click twice on '<b>separated</b>' to indicate that your scheme should not be separated.</li>
            <li>Click <button>Search</button> and wait.</li>
            <li>Now, you will (hopefully) see examples of schemes $X$ which are non-separated.
        </ol>

        <h2>Example 2 (reduced is not fppf-local)</h2>
        The property of a scheme being reduced is not an fppf-local property. Suppose we want to find a counterexample, that is, a morphism $f : X \to Y$ which is an <a target="_blank" href="docs.php#algebraic_geometry.morphism.fppf_cover">fppf-cover</a> with $Y$ being <a target="_blank" href="docs.php#algebraic_geometry.scheme.reduced">reduced</a> and $X$ non-reduced.
        <ol style="line-height: 24px;">
            <li>Create two schemes $X$ and $Y$, and create a morphism $f : X \to Y$ using <button>+add</button>.</li>
            <li>Click on the properties '<b>fppf-cover</b>' for $f$, and '<b>reduced</b>' for $Y$ to indicate that $f$ and $Y$ should have these properties. Also click twice on '<b>reduced</b>' for $X$ to indicate that $X$ should not be reduced.</li>
            <li>Click <button>Search</button> and wait.</li>
            <!-- <li>Looking through the conclusions, you will not find that it follows that $X$ is reduced as well. However, there are a number of examples of schemes and morphisms with the desired properties. Hoping to find a counterexample, we refine our search.</li> -->
            <!-- <li>Selecting $X$, click twice on '<b>reduced</b>' to indicate that $X$ should not be reduced.</li> -->
            <!-- <li>Click <button>Search</button> again.</li> -->
            <li>We find (at least one) example of an fppf-cover with reduced target but non-reduced source. This shows that being reduced is not an fppf-local property.</li>
        </ol>
    </div>
</body>

</html>