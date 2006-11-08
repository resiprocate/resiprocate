<?php
require('reprofunctions.php');
dbgSquirt("============= Update Password ===============");

// check that the user has authenticated
$result = checkCookies($forceLogin,$error,FALSE);
if (!($result) || $forceLogin) {
  // we got an error back that occurred while checkCookies was being run, 
  // or authentication failed.  Either way, bounce them back to the login screen
  dbgSquirt("Authentication failed");
  header("Location: http://" . $_SERVER['HTTP_HOST'] . 
	 dirname($_SERVER['PHP_SELF']) . 
	 "/index.php?error=$error");
  exit;
 }
$username = $_COOKIE['user'];

// check if we got to this page due to a submit or a cancel
dbgSquirt("Checking for cancel");
if ("Cancel" == $_POST['submit']) {
  dbgSquirt("...cancel");
  header("Location: http://" . $_SERVER['HTTP_HOST'] . 
	 dirname($_SERVER['PHP_SELF']) . 
	 "/userhome.php");
  exit;
 }

// verify that a new password was provided via POST (and retyped)
dbgSquirt("Checking post");
if (!isset($_POST['current']) || !isset($_POST['newpassword']) || 
    !isset($_POST['newpassword2'])) {
  // error .. no post variables provided ... possibly because they've jumped
  // directly to this page?
  dbgSquirt("...not set");
  header("Location: http://" . $_SERVER['HTTP_HOST'] . 
	 dirname($_SERVER['PHP_SELF']) . 
	 "/changepassword.php?error=No new password was provided.  Please enter one and click Save.  If this error reoccurs, contact an administrator.");
  exit;
 }

// verify that the new password is non-blank
$newPassword = $_POST['newpassword'];
dbgSquirt("Checking blank -- $newPassword");
if (empty($newPassword)) {
  // error ... requested password is blank... bounce them back to change 
  // password page
  dbgSquirt("...Empty");
  header("Location: http://" . $_SERVER['HTTP_HOST'] . 
	 dirname($_SERVER['PHP_SELF']) . 
	 "/changepassword.php?error=The new password must not be blank.");
  exit;
 }

// verify that the retype of the new password matches
$newPassword2 = $_POST['newpassword2'];
dbgSquirt("Checking match -- $newPassword and $newPassword2");
if ($newPassword != $newPassword2) {
  // error ... password entries don't match... bounce them back to change 
  // password page
  dbgSquirt("...NO.  Don't match");
  header("Location: http://" . $_SERVER['HTTP_HOST'] . 
	 dirname($_SERVER['PHP_SELF']) . 
	 "/changepassword.php?error=Password and retyped password don't match");
  exit;
 }

// verify that the new password is actually different
$currentPassword = $_POST['current'];
dbgSquirt("Checking that new password is different -- $newPassword and $currentPassword");
if ($newPassword == $currentPassword) {
  // error ... password entries shouldn't match ... what's the point of changing
  dbgSquirt("Trying to reuse the current password");
  header("Location: http://" . $_SERVER['HTTP_HOST'] . 
	 dirname($_SERVER['PHP_SELF']) . 
	 "/changepassword.php?error=The new password is the same as the existing password.");
  exit;
 }

// make sure the current password they entered matches
$encryptedPassword = createPassword($username,$currentPassword);
$result = validateUser($username,$encryptedPassword);
dbgSquirt("Verifying current password");
if ("A" != $result) {
  // either didn't match, or user is unverified or disabled
  // only way a user should end up here and be unverified or disabled is if
  // an admin changed their account status in the middle of a session.
  // but we'll check for it anyway...
  dbgSquirt("...doesn't match an active user");
  header("Location: http://" . $_SERVER['HTTP_HOST'] . 
	 dirname($_SERVER['PHP_SELF']) . 
	 "/changepassword.php?error=Current password doesn't match an active user.  Please try again.  If you receive this error again, contact an administrator.");
  exit;
  
 }

// update the password for this user with the provided value
$encryptedPassword = createPassword($username,$newPassword);

if (updatePassword($username,$encryptedPassword)) {
  // update successful
  $title = "Password changed";
  $heading = "Password changed";
  $msg = "Password successfully updated.";
 } else {
  // update failed
  $title = "Error while changing password";
  $heading = "Error while changing password";
  $msg = "An error occurred while attempting to change your password.  Please contact an administrator.";
 }
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<!--
System:  Repro
File:    updatepassword.php
Purpose: Check permissions, verify requested change, and update password
Author:  S. Chanin
-->
<html>
<head>
<link rel="stylesheet" type="text/css" href="repro_style.css" />
  <title><?php echo $title; ?></title>
</head>

<body>
<h1 class="title">Repro</h1>
<h1><?php echo $heading; ?></h1>
<hr />
<p><?php echo $msg; ?></p>
<br /><hr />
<a href="userhome.php">Return to User Home</a><br />
<a href="logout.php">Logout</a><br />

</body>
</html>
