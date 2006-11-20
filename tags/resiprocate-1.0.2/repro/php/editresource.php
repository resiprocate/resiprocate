<?php
require('reprofunctions.php');
dbgSquirt("============= Edit Resource ===============");
dbgSquirt("GET --" . dbgShowFile($_GET));

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

// this page is only entered via GET's
// all of these should be set all the time, even though they might be 
// empty... if they aren't set, something is strange about how we got to this
// page
if (!isset($_GET['resourceId']) || !isset($_GET['aor']) || 
    !isset($_GET['forwardType']) || !isset($_GET['forward']) ||
    !isset($_GET['voicemail'])) {
  header($bounceURL . "Information missing in request to modify a resource. Please try again.  If this error reoccurs, please contact an administrator.");
  exit();
 }

$resourceId = $_GET['resourceId'];
$aor = $_GET['aor'];
$forwardType = $_GET['forwardType'];
$forward = $_GET['forward'];
$voicemail = $_GET['voicemail'];

// make sure resourceId isn't blank.  Other fields could be blank
if (empty($resourceId)) {
  header($bounceURL . "Information missing in request to modify a resource. Please try again.  If this error reoccurs, please contact an administrator.");
  exit();
 }

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<!--
System:  Repro
File:    editresource.php
Purpose: display the current information for a resource.  Allow an 
         authenticated user to edit that information.
Author:  S. Chanin
-->
<html>
<head>
<link rel="stylesheet" type="text/css" href="repro_style.css" />
<title></title>
<script type="text/javascript">
<!--
function disableForward() {
  document.resourceForm.forward.value = ""
  document.resourceForm.forward.disabled = true
}

function enableForward() {
  document.resourceForm.forward.disabled = false
}
//-->
</script>
</head>

<body>
<h1 class="title">Repro</h1>
<h1>Edit Resource</h1>
<hr />

<?php
// if we've looped back due to an error, show the message
if (isset($_GET["error"]) && !empty($_GET['error'])) {
    echo '<p class="error">' . $_GET["error"] . "</p>\n";
}
?>

<form method="POST" action="savemodifiedresource.php" name="resourceForm" id="resourceForm">
  <table>
<input type="hidden" name="resourceId" id="resourceId" value="<?php echo $resourceId ?>"/>
<tr>
<td>Address</td>
<td><input type="text" name="aor" id="aor" value="<?php echo $aor ?>"/></td>
</tr>
<tr>
<td>Forward</td>
<td><input type="radio" name="forwardType" id="forwardType" value="Yes" 
  onclick="enableForward()" 
  <?php if ("Y" == $forwardType || "" == $forwardType)
   echo 'checked="checked"'; ?>
   >Yes</td></tr>
<tr><td>&nbsp</td>
<td><input type="radio" name="forwardType" id="forwardType" value="No" 
       onclick="disableForward()"
       <?php if ("N" == $forwardType) echo 'checked="checked"'; ?>
       >No</td></tr>
       <tr><td>Forward Address</td>
       <td><input type="text" name="forward" id="forward" value="<?php echo $forward; ?>"/>
  </td>
</tr>
<tr>
<td>Voicemail</td>
<td><input type="text" name="voicemail" id="voicemail" value="<?php echo $voicemail; ?>"/></td>
</tr>
<tr>
<td>&nbsp</td>
<td>
<input type="submit" name="submit" id="submit" value="Save" />
<input type="submit" name="submit" id="submit" value="Cancel" />
</td>
</tr>
</table>

<?php
// if forwardType is "N" we also need to disable the forward address box
       if ("N" == $forwardType) {
?>
<script type="text/javascript">
<!--
disableForward()
//-->
</script>
<?php
       }
?>

</form>

<br /><hr />
<a href="userhome.php">Return to User Home</a><br />
<a href="logout.php">Logout</a><br />
<br />

</body>
</html>
