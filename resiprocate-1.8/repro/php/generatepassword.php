<?php
/*
System:  Repro
File: 	 generatepassword
Purpose: Take a username and an email address and, if that email address matches
the username, then generate a new, random, password, set that users
password to the new password, and email the password to the user.
Author:  S. Chanin
*/

require('reprofunctions.php');

// edit checks on values entered
$error="";

// pull out the post variables
$username=$_POST['username'];
$email=$_POST['email'];

// ensure that all required values have been filled in
if (empty($username)) {
    $error = $error . "Username must be non-blank.<br />"; }
if (empty($email)) {
	$error = $error . "Email must be non-blank.<br />"; }

/* validate that the username and email address match a known user.
 note -- this error could occur either because the username is unknown
 or because the email address doesn't match.  I don't show this
 distinction to avoid creating an easy way to fish for usernames. 

 This function also requires that the username correspond to an active account.
 We do not allow passwords to be reset for unverified or disabled accounts.
*/
if (!matchUserAndEmail($username,$email)) {
	$error = $error . "That username does not match the email address provided for any of our active accounts.<br />"; 	
}

if (!empty($error)) {
	header("Location: http://" . $_SERVER['HTTP_HOST'] . 
			dirname($_SERVER['PHP_SELF']) . 
			"/forgotpassword.php?username=$username&email=$email&error=" . urlencode($error));
		exit;
}
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html>
<head>
<link rel="stylesheet" type="text/css" href="repro_style.css" />
<title>Sending New Password</title>
</head>


<body>
<h1 class="title">Repro</h1>

<?php
// generate a random password
// TODO -- think about whether this works in a unicode, non-english environment
$password = "";
for($i=0; $i<8; $i++) {
	$type = rand(0,2);
	
	if (0 == $type) {
		// generate a lower case letter
		$password .= chr(rand(97,122));
	} else if (1 == $type) {
		// generate an upper case letter
		$password .= chr(rand(65,90));
	} else {
		// generate a number
		$password .= chr(rand(48,57));
	}
}

// update the account to use that password
$encryptedPassword = createPassword($username,$password);

if (!updatePassword($username,$encryptedPassword)) {
	// an error occurred while updating the password
	?>
	<h2>Error -- Internal Error Saving New Password.</h2>
	<p>An internal error occurred while attempting to change the password on
	your account.  Please contact an administrator for assistance.</p>
	<?php
} else {
	// generate the email 
	// subject
	$subject = "Password Reset Notice";

	// message
	$message = '
<html>
<head><title>Password Reset Notice</title></head>
<body>
<p>At your request, we have reset the password on your account to a new, random
password.  Your new password is:</p><p>' . $password .
'</p><p>Please change the password to one that you will remember on your next
log in.</p></body></html>';

	// To send HTML mail, the Content-type header must be set
	$headers  = 'MIME-Version: 1.0' . "\r\n";
	$headers .= 'Content-type: text/html; charset=iso-8859-1' . "\r\n";

	// Additional headers
	$headers .= "From: $providerEmail". "\r\n";

	// Mail it
	if (!mail($email, $subject, $message, $headers)) {
		// mail returned an error
		?>
		<h2>Error -- Internal Error Sending Confirmation Email.</h2>
		<p>An internal error occurred while sending you the email with your
		new password.  Please contact an administrator for assistance.</p>
	<?php
	} else {
		// show confirmation message
		// also my temp copy of the message since email isn't working
		?>
		<h2>Email sent</h2>
		<p>An email has been sent to <?php echo $email ?> with a new temporary
		password that has been created for you.  Please use that password to
		log back into the site and then change your password to one that you
		can remember.</p>
		
		<br /><hr />
		<p>Since I don't have a working SMTP server on this machine, I need to
		echo the new password to this page so I can test it.</p><br />
		<p>Your new password is:</p>
		<?php echo $password;
	}
}
?>

<br /><hr />
<a href="index.php">Return to Welcome Page</a>
<br />

</body>
</html>
