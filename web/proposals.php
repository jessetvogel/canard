<!doctype html>
<html>

<head>
    <?php include('template/stdhead.php'); ?>
</head>

<body>
    <?php include('template/menu.html'); ?>

    <div id="content">
        <h2>Proposals</h2>
        <table id="table-proposals">
            <tr><th>User</th><th>Proposal</th></tr>
        </table>
    </div>

    <script type="text/javascript">
        dbGet('proposals', ['user', 'proposal']).then(data => {
            const table = $('table-proposals');
            for(let row of data) {
                const tr = create('tr');
                tr.append(create('td', row['user']));
                tr.append(create('td', row['proposal']));
                table.append(tr);
            }
        })
    </script>
</body>

</html>