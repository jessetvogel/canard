<!doctype html>
<html>

<head>
    <?php $MathJax = true; include('template/stdhead.php'); ?>
    <script type="text/javascript" src="js/search.js"></script>
</head>

<body>
    <?php include('template/menu.html'); ?>

    <div id="object-bar">
        <div><input id="object-name" class="math" placeholder="X" /></div><span>:</span>
        <div><select id="object-type"></select></div>
        <div id="object-type-arguments" style="display: none;"></div>
        <div><button id="object-add">+ add</button></div>
    </div>

    <div id="object-overview">
        <div id="object-list"></div>
        <div id="object-properties"></div>
    </div>

    <div id="search-bar">
        <button id="button-search">Search</button>
        <button id="button-contradiction">Contradiction?</button>
        <a href="help.html" target="_blank">Help</a>
    </div>

    <div id="output"></div>
</body>

</html>