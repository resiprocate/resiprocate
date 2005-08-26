<?php
require('reprofunctions.php');
dbgSquirt("============= Change Password ===============");

$result = checkCookies($forceLogin,$error,FALSE);
if (!($result) || $forceLogin) {
  // we got an error back that occurred while checkCookies was being run, 
  // or authentication failed.  Either way, bounce them back to the login screen
  header("Location: http://" . $_SERVER['HTTP_HOST'] . 
	 dirname($_SERVER['PHP_SELF']) . 
	 "/index.php?error=$error");
  exit;
 }
$username = $_COOKIE['user'];
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<!--
System:  Repro
File:    changepassword.php
Purpose: Allow an authenticated user to change the password stored for them
Author:  S. Chanin
-->
<html>
<head>
<link rel="stylesheet" type="text/css" href="repro_style.css" />
  <title>Change Password</title>
</head>

<body>
<h1 class="title">Repro</h1>
<h1>Change Password</h1>
<hr />
<?php
// if we've looped back due to an error, show the message
if (isset($_GET["error"])) {
    echo '<p class="error">' . $_GET["error"] . "</p>\n";
}
if (lookupUserInformation($username,$id,$fullname,$domain,$email)) {
  // lookup successful
?>
<form method="POST" action="updatepassword.php">
<table>
<tr>
<td>Current Password</td>
<td><input type="password" name="current" id="current" value=""/></td>
</tr>
<tr>
<td>New Password</td>
<td><input type="password" name="newpassword" id="newpassword" value=""/></td>
</tr>
<tr>
<td>Retype New Password</td>
<td><input type="password" name="newpassword2" id="newpassword2" value=""/></td>
</tr>
<tr>
<td>&nbsp</td>
<td>
<input type="submit" name="submit" id="submit" value="Save" />
<input type="submit" name="submit" id="submit" value="Cancel" />
</td>
</tr>
</table>
</form>

<?php
   } else {
  echo "<p>Internal Error while accessing user information.</p>\n";
  echo "<p>Please contact an administrator.</p>\n";
 }
?>

<br /><hr />
<a href="userhome.php">Return to User Home</a><br />
<a href="logout.php">Logout</a><br />

</body>
</html>
