<!doctype html>
<html>

<head>
    <?php include('template/stdhead.php'); ?>
    <script type="text/javascript" src="js/search.js"></script>
    <script type="text/javascript" src="js/advanced.js"></script>
</head>

<body>
    <?php include('template/menu.html'); ?>

    <h2>Advanced</h2>

    <div id="content">
        <div id="context" class="input">
            <div class="line-numbers"><span>1</span></div>
            <textarea id="input-search" wrap="off" placeholder="search ..."></textarea>
        </div>

        <div id="search-bar"><button id="button-search">Search</button><a href="advanced-help.html" target="_blank">Help</a></div>

        <div id="output"></div>

    </div>
</body>

</html>