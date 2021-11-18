<!doctype html>
<html>

<head>
    <?php include('template/stdhead.php'); ?>
</head>

<body>
    <?php include('template/menu.html'); ?>

    <h2>Propose</h2>

    <div id="content">
        If you want to add an example, theorem or property to the website, you can submit it via the form below.
        Please include references (e.g. to books or to the Stacks Project) in case of theorems, or properties with justification in case of examples.
        <div id="proposal-form">
            <textarea id="proposal-text" placeholder="Write your proposal here..."></textarea>
            <input id="proposal-name" type="text" placeholder="Your name" />
            <button id="submit-proposal">Submit</button>
        </div>
    </div>

    <script type="text/javascript">
        onClick($('submit-proposal'), async () => {
            const proposal = $('proposal-text').value;
            const user = $('proposal-name').value;
            const response = await dbInsert('proposals', {
                'proposal': proposal,
                'user': user
            }, prompt('Password:'));

            if (response === true) {
                alert('Thank you! Your proposal is registered!');
                $('proposal-text').value = '';
                $('proposal-user').value = '';
                window.location.reload();
            } else {
                alert('Failed to register proposal! Maybe the password is incorrect?');
                console.log(response);
            }
        });
    </script>
</body>

</html>