<?php
require('reprofunctions.php');
dbgSquirt("============= Save Modified Resource ===============");
dbgSquirt(dbgShowFile($_POST));

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
$bounceURL = "Location: http://" . $_SERVER['HTTP_HOST'] . 
  dirname($_SERVER['PHP_SELF']) . 
  "/editresource.php?aor=" . $_POST['aor'] . 
  "&forwardType=" . $_POST['forwardType'] . 
  "&forward=" . $_POST['forward'] . 
  "&voicemail=" . $_POST['voicemail'] . 
  "&error=";

// make sure post variables have arrived
// note -- can't check for forward because if it was diabled on the previous
// screen by clicking No, it will not be sent as a POST variable
if (!isset($_POST['resourceId']) || !isset($_POST['aor']) || 
    !isset($_POST['forwardType']) || !isset($_POST['voicemail'])) {
  header($bounceURL . "The information to modify a resource was not provided.  Please enter the information and click Save.  If this error reoccurs, contact an administrator.");
  exit;
 }

// check if the user pressed cancel ... if so, back to user home
if ("Cancel" == $_POST['submit']) {
  header("Location: http://" . $_SERVER['HTTP_HOST'] . 
	 dirname($_SERVER['PHP_SELF']) . "/userhome.php");
  exit;
 }

// check that resourceId only contains digits
// the valid number check is for security to make sure that no one hacks the 
// URL and replaces the resourceId param with something designed to screw up
// the database.  In this case, there is nothing the user can fix, so send them
// back to userhome.
$resourceId = $_POST['resourceId'];
if (!ereg("^[0-9]+$",$resourceId)) {
  header("Location: http://" . $_SERVER['HTTP_HOST'] . 
	 dirname($_SERVER['PHP_SELF']) . "/userhome.php?error=The information to modify a resource was not provided.  Please enter the information and click Save.  If this error reoccurs, contact an administrator.");
  exit;
 }

// check that resource name is non-blank
if (empty($_POST['aor'])) {
  header($bounceURL . "The address must be filled in.");
  exit;
 }
$aor = $_POST['aor'];

// check that if forwarding is Yes, then a forward address must be provided
// in this case we need to check forwardType against "Yes" rather than "Y since
// the value comes from the previous form rather than that database (which only
// stores 1 char)
if (($_POST['forwardType'] == "Yes") && empty($_POST['forward'])) {
  header($bounceURL . "If forwarding is turned on, a forwarding address must be provided.");
  exit;
 }
$forwardType = $_POST['forwardType'];
$forward = $_POST['forward'];
$voicemail = $_POST['voicemail'];

// TODO: add code to validate that the forwarding address and voicemail
// address are valid SIP URI's

// update the resource to the database
// note: as an additional security measure, we pass in username and only update
// the resourceId if that resourceId matches the authenticated user.  As a
// consequence, even if the user hacks the URL and replaces the resourceId
// with a new value, they will be unable to modify any resources not associated
// with the user they are logged in as ... so they can't do any damage since
// all the resourceId's associated with their user are available to them 
// anyway
if (updateResource($resourceId,$username,$aor,$forwardType,$forward,$voicemail)) {
  // resource modified successfully
  $title = "Resource Modified";
  $heading = "Resource Modified";
  $msg = "Successfully updated the resource <em>$aor.</em>";
 } else {
  $title = "Error While Updating Resource";
  $heading = "Error While Updating Resource";
  $msg = "An error occurred while attempting to update this resource.  Please contact an administrator.";
 }
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<!--
System:  Repro
File:    savenewresource.php
Purpose: Validate the user provided information about a new resource and then
         add that resource to their profile
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
