<?php
require('reprofunctions.php');
dbgSquirt("============= Modify Resource ===============");
dbgSquirt("GET --" . dbgShowFile($_GET));
dbgSquirt("POST --" . dbgShowFile($_POST));

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
  dirname($_SERVER['PHP_SELF']) . "/userhome.php?error=";

// make sure post variables have arrived.  We should always get a resourceId,
// name, and either an edit or a delete
if (!(isset($_POST['resourceId']) && isset($_POST['aor']) && 
      (isset($_POST['edit']) || isset($_POST['delete'])))) {
  header($bounceURL . "The information to modify a resource was not provided.  Please enter the information and click Save.  If this error reoccurs, contact an administrator.");
  exit;
 }

// check that resourceId is non-blank ... this shouldn't happen since this is
// a system provided invisible field
if (empty($_POST['resourceId']) || empty($_POST['aor'])) {
  header($bounceURL . "The resource to be modified was not specified.  Please click one of the Add or Delete buttons.  If you see this message again, please contact an administrator.");
  exit;
 }
$resourceId = $_POST['resourceId'];
$aor = $_POST['aor'];

//see if the operation is Edit or Delete
if ("Delete" == $_POST['delete']) { 
  // delete that resouce
  if (deleteResource($username,$resourceId)) {
    // success
    $title = "Resource Deleted";
    $heading = "Resource Deleted";
    $msg = "Successfully deleted the resource: <em>$aor</em>";
  } else {
    // delete failed
    $title = "Error while deleting";
    $heading = "Error while deleting";
    $msg = "An error occurred while deleting the resource <em>$aor</em>.  Please contact an administrator.";
  }
 } else if ("Edit" == $_POST['edit']) {
  // get displayed values
  $forwardType = $_POST['forwardType'];
  $forward = $_POST['forward'];
  $voicemail = $_POST['voicemail'];
  
  // redirect to a new page for handling edits to an existing URL
  header("Location: http://" . $_SERVER['HTTP_HOST'] . 
	 dirname($_SERVER['PHP_SELF']) . "/editresource.php?resourceId=$resourceId&aor=$aor&forwardType=$forwardType&forward=$forward&voicemail=$voicemail");
  exit;
 } else {
  // shouldn't get here ... this means no valid action was requested
  header($bounceURL . "Error while modifying resources. Please contact an administrator.");
  exit;
 }

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<!--
System:  Repro
File: 	 modifyresource.php
Purpose: Allows a user to edit or delete a resource that is attached to their account
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
