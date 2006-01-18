<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<!--
System:  Repro
File: forgotpassword.php
Purpose: Reset password & email to address on file for the user
Author: S. Chanin
-->
<html>
<head>
<link rel="stylesheet" type="text/css" href="repro_style.css" />
<title>Forgot Password</title>
</head>

<body>
<h1 class="title">Repro</h1>

<h1>Forgot Password</h1>
<hr />

<!-- if we've looped back due to an error, show the message -->
<?php
if (isset($_GET["error"])) {
	echo '<p class="error">' . $_GET["error"] . "</p>\n";
}
?>

<p>If you have forgotten your password, please enter your email address below
and the system will generate a new password for you and email that password
to you.</p>

<form action="generatepassword.php" method="post">
<table>
<tr>
<td>Username:</td>
<td><input type="text" name="username" id="username" value="<?php echo $_GET['username']; ?>" /></td>
</tr>

<tr>
<td>Email address:</td>
<td><input type="text" name="email" id="email" value="<?php echo $_GET['email']; ?>" /></td>
</tr>

<tr>
<td>&nbsp</td>
<td><input type="submit" name="submit" id="submit" value="Reset Password" />
</tr>
</table>
</form>

<br /><hr />
<a href="index.php">Return to Welcome Page</a>
<br />
</body>
</html>
