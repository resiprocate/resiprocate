<?php
require('reprofunctions.php');

clearCookies();
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<!--
System:  Repro
File:    Index
Purpose: Collect users name and password to validate his credentials
Author:  Steven Chanin
-->

<html>
<head>
<link rel="stylesheet" type="text/css" href="repro_style.css" />
<title>Login</title>
</head>

<body>
<h1 class="title">Repro</h1>
<h1>Login</h1>
<hr />

<!-- if we've looped back due to an error, show the message -->
<?php
if (isset($_GET["error"])) {
    echo '<p class="error">' . $_GET["error"] . "</p>\n";
}
?>

<form method="POST" action="userhome.php">
<table>
<tr>
<td>Username</td>
<td><input type="text" id="username" name="username" value="<?php echo $_GET["username"]?>"/></td>
</tr>
<tr>
<td>Password</td>
<td><input type="password" id="password" name="password" /></td>
</tr>
<tr>
<td>&nbsp</td>
<td align="right">
  <input type="submit" id="submit" name="submit" value="Login" /></td>
<tr>
</table>
</form>
<hr />
<a href="forgotpassword.php">Forgot password</a>
<br />
<a href="createaccount.php">Sign up for an account</a>
<br />
</body>
</html>
