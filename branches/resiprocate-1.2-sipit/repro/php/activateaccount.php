<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<!--
System:  Repro
File:    activateaccount.php
Purpose: Accepts a username and an activation code.  If that user exists,
         is in unverified state, and the activationcode matches, then the 
	 account is activated.

	 If any of those things are not true, an appropriate error message
	 is shown.
Author:  S. Chanin
-->
   
<?php

require('reprofunctions.php');

$msgHeading = "";
$msgBody = "";

// check if username and activationCode were passed in as GET params
// and are non-empty

if ((isset($_GET['user'])) && (!empty($_GET['user'])) &&
 (isset($_GET['code'])) && (!empty($_GET['code']))) {
  $username = $_GET['user'];
  $activationCode = $_GET['code'];

  // let's lookup the activation state of the user
  $state = getUserState($username,$activationCode);
      
  if ("A" == $state) {
    // active user ... shouldn't be activated a second time
    $msgHeading = "Duplicate activation request";
    $msgBody = "This account is already active.  It cannot be activated a second time.  Please log in from the Welcome Page.";
  } else if ("U" == $state) {
    // unverified user ... so we'll activate
    if (activateUser($username,$activationCode)) {
      $msgHeading = "Congratulations $username.";
      $msgBody = "Your account is now active.  Please follow the link below back to the Welcome Page to log in.";
    } else {
      // activation failed
      $msgHeading = "Error while activating your account.";
      $msgBody = "An error occurred while activating your account.  Please contact an administrator for assistance.";
    }
  } else if ("D" == $state) {
    // disabled user ... shouldn't be activated. should be handled by
    // the administrator
    $msgHeading = "Request to activate a disabled account";
    $msgBody = "This account has been disabled.  It cannot be reactivated without action by the administrators.  Please contact an administrator for assistance.";
  } else if ("N" == $state) {
    // no match (either user is unknown or activationCode is wrong)
    $msgHeading = "Invalid activation information";
    $msgBody = "The parameters that were passed in are incorrect.  Please recheck the link in your email.  If you see this error again, please try manually cutting and pasting the link into your browser.";
  } else {
    // internal error ... state should always be one of the previous
    // four values.
    $msgHeading = "Internal error during activation";
    $msgBody = "An error occurred while attempting to activate your account.  Please contact an administrator for assistance.";
  }
 } else {
  // bad parameters
  $msgHeading = "Invalid activation information";
  $msgBody = "The parameters that were passed in are incorrect.  Please recheck the link in your email.  If you see this error again, please try manually cutting and pasting the link into your browser.";
 }
?>

<html>
<head>
<link rel="stylesheet" type="text/css" href="repro_style.css" />
<title>Account Activated</title>
</head>

<body>
<h1 class="title">Repro</h1>
<h1><?php echo $msgHeading ?></h1>
<hr />
<p><?php echo $msgBody ?></p>

<br /><hr />
<a href="index.php">Return to Welcome Page</a>
<br />

</body>
</html>
