<?php
/*
System:	 Repro
File:	 emailactivationnotice.php
Purpose: validate the information entered in the account setup screen, create
		 an account based on this, and email an activation code to the user.
Author:  S. Chanin
*/

require('reprofunctions.php');

// edit checks on values entered
$error="";

// pull out the post variables
$username=$_POST['username'];
$password=$_POST['password'];
$password2=$_POST['password2'];
$fullname=$_POST['fullname'];
$domain=$_POST['domain'];
$email=$_POST['email'];
$email2=$_POST['email2'];
$userkey=$_POST['userkey'];
$keyvalue=$_POST['keyvalue'];

// ensure that all required values have been filled in
if (empty($username)) {
    $error = $error . "Username must be non-blank.<br />"; }
if (empty($password)) {
	$error = $error . "Password must be non-blank.<br />"; }
if (empty($password2)) {
	$error = $error . "Retyped Password must be non-blank.<br />"; }
if (empty($fullname)) {
	$error = $error . "Full Name must be non-blank.<br />"; }
if (empty($email)) {
	$error = $error . "Email must be non-blank.<br />"; }
if (empty($email2)) {
	$error = $error . "Retyped Email must be non-blank.<br />"; }

// ensure that double entered values match
if ($password != $password2) {
	$error = $error . "Values entered for passord do not match.<br />"; }
if ($email != $email2) {
	$error = $error . "Values entered for email address do not match.<br />"; }

// ensure that the CAPTCHA key was correctly read and reentered
if (md5($userkey) != $keyvalue) {
	$error = $error . "Security key does not match.<br />"; }

// validate that username isn't already in use
if (usernameInUse($username) == "Y") {
	$error = $error . "That username is already in use.<br />"; 	
}

// verify that they have passed the automated turing test
// verify the email address passes a sniff test

if (!empty($error)) {
	header("Location: http://" . $_SERVER['HTTP_HOST'] . 
			dirname($_SERVER['PHP_SELF']) . 
			"/createaccount.php?username=$username&fullname=$fullname&email=$email&email2=$email2&error=" . urlencode($error));
		exit;
}

//create an activation code
/* there may be a better way to do this.  My thought is that md5 gives me a
string that is pretty random and long enough that it is essentially impossible
to guess.  By seeding it with microtime and username the key should be different
for each user it would take a couple of thousand guesses to get a match if you
tried to brute force create every possible activationKey for the time around
when the attackers account was created.
*/
$activationCode = md5(microtime() . $username);

// create the actual account
$encryptedPassword = createPassword($username,$password);

if (!createAccount($username,$encryptedPassword,$fullname,$domain,$email,$activationCode)) {
	// oops ... got an error creating the account
	$error = $error . "Error while creating account.";
	header("Location: http://" . $_SERVER['HTTP_HOST'] . 
			dirname($_SERVER['PHP_SELF']) . 
			"/createaccount.php?username=$username&fullname=$fullname&email=$email&email2=$email2&error=" . urlencode($error));
		exit;
}

// create a default resource
$defaultAOR = $username . '@' . $domain;
if (!createResource($username,$defaultAOR,'N','','')) {
	// oops ... got an error creating the default resource
	$error = $error . "Error while creating account (default resource).";
	header("Location: http://" . $_SERVER['HTTP_HOST'] . 
			dirname($_SERVER['PHP_SELF']) . 
			"/createaccount.php?username=$username&fullname=$fullname&email=$email&email2=$email2&error=" . urlencode($error));
		exit;
}

// email the activation notice
// create activation link
$link = "http://" . $_SERVER['HTTP_HOST'] . dirname($_SERVER['PHP_SELF']) .
	"/activateaccount.php?user=$username&code=$activationCode";

// subject
$subject = "$provider Activation Notice";

// message
$message = '
<html>
<head>
  <title>' . $provider . ' Activation Notice</title>
</head>
<body>
<h1>Welcome to '. $provider . 
'</h1>

<p>This email address has recently been used to create a new 
account at ' . $provider . '</p><br />
<p>In order to ensure this account was actually requested by you, 
we send an email to the address provided and ask that you confirm the
new account request by clicking the link below:</p>
<br />' . $link . '<br />
If you do not click this link, the account will not be activated.
<br />
<br />
Sincerely,<br />' . $provider . 
'</body>
</html>
';

// To send HTML mail, the Content-type header must be set
$headers  = 'MIME-Version: 1.0' . "\r\n";
$headers .= 'Content-type: text/html; charset=iso-8859-1' . "\r\n";

// Additional headers
$headers .= "From: $providerEmail". "\r\n";

// Mail it
if (!mail($email, $subject, $message, $headers)) {
	// mail returned an error
	$error = $error . "Error while emailing activation notice.";
	header("Location: http://" . $_SERVER['HTTP_HOST'] . 
			dirname($_SERVER['PHP_SELF']) . 
			"/createaccount.php?username=$username&fullname=$fullname&email=$email&email2=$email2&error=" . urlencode($error));
		exit;
}
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html>
<head>
<link rel="stylesheet" type="text/css" href="repro_style.css" />
<title>Activation Notice</title>
</head>

<body>
<h1 class="title">Repro</h1>
<h1>Activation Notice</h1>
<hr />
<p>Congratulatins <?php echo $fullname ?> and welcome to <?php echo $provider ?>.
Your account has been established on our server.  In order to ensure all
the information provided is correct, we have sent an email with an activation
link to the address you provided in the signup process.</p>

<p>Please read that message and click the link provided, or follow the 
instructions in the email for manual activiation.</p>

<hr />
<p>Since I don't have an SMTP server running on this machine, I can't send out messages.
As a result, I can't test that the above email code actually works.</p>

<p>So for this version, here is the same link to click that is in the email.  Clicking
this link will activate your account.</p>

<a href="<?php echo $link ?>">Click here to activate</a>
<br />
Or copy the following link into your browser: <br />
<?php echo $link ?>

<br /><hr />
<a href="index.php">Return to Welcome Page</a>
<br />

</body>
</html>
