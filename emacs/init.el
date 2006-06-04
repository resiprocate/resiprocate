;;; -*- Mode: Emacs-Lisp -*-

;;; Define a variable to indicate whether we're running XEmacs/Lucid Emacs.
;;; (You do not have to defvar a global variable before using it --
;;; you can just call `setq' directly like we do for `emacs-major-version'
;;; below.  It's clearer this way, though.)
(defvar running-xemacs (string-match "XEmacs\\|Lucid" emacs-version))

(put 'upcase-region 'disabled nil)
(put 'downcase-region 'disabled nil)

(setq dabbrev-case-replace nil)
(setq dabbrev-case-fold-search nil)
(setq bell-volume 0)
(setq sound-alist nil)

(if (not running-xemacs)
    (setq keyboard-translate-table "\0\^a\^b\^c\^d\^e\^f\^g\^?")
  (keyboard-translate ?\C-h ?\C-?))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;			Basic Customization			    ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Enable the commands `narrow-to-region' ("C-x n n") and 
;; `eval-expression' ("M-ESC", or "ESC ESC").  Both are useful
;; commands, but they can be confusing for a new user, so they're
;; disabled by default. (Re enable them)
(put 'narrow-to-region 'disabled nil)
(put 'eval-expression 'disabled nil)

;;; Define a variable to indicate whether we're running XEmacs/Lucid Emacs.
;;; (You do not have to defvar a global variable before using it --
;;; you can just call `setq' directly like we do for `emacs-major-version'
;;; below.  It's clearer this way, though.)
;(defvar running-xemacs (string-match "XEmacs\\|Lucid" emacs-version))

;; Make the sequence "C-x =" execute the `my-what-line' command, 
;; which prints the current line number in the echo area.
(global-set-key "\C-xw" 'what-line)

(defun my-what-line ()
  "print a message giving the current line number and the total number of lines in the buffer"
  (interactive)
  (save-restriction
    (widen)
    (let ((col (current-column)))
      (save-excursion
        (beginning-of-line)
        (let ((cur-line (+ 1 (count-lines 1 (point))))
              (end-line (+ 1 (count-lines 1 (point-max)))))
          (message "Line %d of %d. Column %d." cur-line end-line col))))))

(global-set-key "\^X="   'my-what-line)

(if (file-exists-p "~jason/emacs/other-part.el")
    (progn
      (load-file "~jason/emacs/other-part.el")
      ;; add ( regexp . replacement) pairs here
      ;; see other-part.el for more info
      (setq find-other-part-alist (append find-other-part-alist nil))
      (global-set-key "\^X\^O" 'find-other-part)))

;;
;; Load macros 
;;

(global-set-key "\C-x\C-e" 'compile)
(global-set-key "\C-xs" 'sort-lines)


;; If you prefer delete to actually delete forward then you want to
;; uncomment the next line.
;; (load-library "delbackspace")


(cond (running-xemacs
       ;;
       ;; Code for any version of XEmacs/Lucid Emacs goes here
       ;;
      
       ;; Change the values of some variables.
       ;; (t means true; nil means false.)
       ;;
       ;; Use the "Describe Variable..." option on the "Help" menu
       ;; to find out what these variables mean.
       (setq find-file-use-truenames nil
	     find-file-compare-truenames t
	     minibuffer-confirm-incomplete t
	     complex-buffers-menu-p t
	     next-line-add-newlines nil
	     mail-yank-prefix "> "
             options-save-faces t
	     kill-whole-line nil)

       ;; When running ispell, consider all 1-3 character words as correct.
       (setq ispell-extra-args '("-W" "3"))

       ;; Change the way the buffer name is displayed in the
       ;; modeline.  The variable for this is called
       ;; 'modeline-buffer-identification but was called
       ;; 'mode-line-buffer-identification in older XEmacsen.
       (if (boundp 'modeline-buffer-identification)
	   ;; Note that if you want to put more than one form in the
	   ;; `THEN' clause of an IF-THEN-ELSE construct, you have to
	   ;; surround the forms with `progn'.  You don't have to
	   ;; do this for the `ELSE' clause.
	   (progn
	     (setq-default modeline-buffer-identification '("XEmacs: %17b"))
	     (setq modeline-buffer-identification '("XEmacs: %17b")))
	 (setq-default mode-line-buffer-identification '("XEmacs: %17b"))
	 (setq mode-line-buffer-identification '("XEmacs: %17b")))

       (cond ((or (not (fboundp 'device-type))
		  (equal (device-type) 'x))
	      ;;
	      ;; Code which applies only when running emacs under X goes here.
	      ;; (We check whether the function `device-type' exists
	      ;; before using it.  In versions before 19.12, there
	      ;; was no such function.  If it doesn't exist, we
	      ;; simply assume we're running under X -- versions before
	      ;; 19.12 only supported X.)

	      ;; Remove the binding of C-x C-c, which normally exits emacs.
	      ;; It's easy to hit this by mistake, and that can be annoying.
	      ;; Under X, you can always quit with the "Exit Emacs" option on
	      ;; the File menu.
	      ;(global-set-key "\C-x\C-c" nil)

	      ;; Uncomment this to enable "sticky modifier keys" in 19.13
	      ;; and up.  With sticky modifier keys enabled, you can
	      ;; press and release a modifier key before pressing the
	      ;; key to be modified, like how the ESC key works always.
	      ;; If you hold the modifier key down, however, you still
	      ;; get the standard behavior.  I personally think this
	      ;; is the best thing since sliced bread (and a *major*
	      ;; win when it comes to reducing Emacs pinky), but it's
	      ;; disorienting at first so I'm not enabling it here by
	      ;; default.

	      ;(setq modifier-keys-are-sticky t)
              (setq bell-volume 0)
              (setq sound-alist nil)

	      ;; This gets rid of the annoying password color problem that
	      ;; passwd.el causes:
	      (setq-default passwd-invert-frame-when-keyboard-grabbed nil)
	      (setq-default passwd-echo nil)

	      ;; This changes kill (C-k) behaviours back to old  emacs style.
	      (setq kill-whole-line nil)

	      ;; This changes the variable which controls the text that goes
	      ;; in the top window title bar.  (However, it is not changed
	      ;; unless it currently has the default value, to avoid
	      ;; interfering with a -wn command line argument I may have
	      ;; started emacs with.)
	      (if (equal frame-title-format "%S: %b")
		  (setq frame-title-format
			(concat "%S: " invocation-name
				" [" emacs-version "]"
				(if nil ; (getenv "NCD")
				    ""
				  "   %b"))))

	      ;; If we're running on display 0, load some nifty sounds that
	      ;; will replace the default beep.  But if we're running on a
	      ;; display other than 0, which probably means my NCD X terminal,
	      ;; which can't play digitized sounds, do two things: reduce the
	      ;; beep volume a bit, and change the pitch of the sound that is
	      ;; made for "no completions."
	      ;;
	      ;; (Note that sampled sounds only work if XEmacs was compiled
	      ;; with sound support, and we're running on the console of a
	      ;; Sparc, HP, or SGI machine, or on a machine which has a
	      ;; NetAudio server; otherwise, you just get the standard beep.)
	      ;;
	      ;; (Note further that changing the pitch and duration of the
	      ;; standard beep only works with some X servers; many servers
	      ;; completely ignore those parameters.)
	      ;;
;	      (cond ((string-match ":0" (getenv "DISPLAY"))
;		     (load-default-sounds)
;		     )
;		    (t
;		     (setq bell-volume 40)
;		     (setq sound-alist
;			   (append sound-alist '((no-completion :pitch 500))))
;		     ))

	      ;; Make `C-x C-m' and `C-x RET' be different (since I tend
	      ;; to type the latter by accident sometimes.)
	      (define-key global-map [(control x) return] nil)

	      ;; Change the cursor used when the mouse is over a mode line
	      (setq x-mode-pointer-shape "leftbutton")

	      ;; Change the cursor used during garbage collection.
	      ;;
	      ;; Note that this cursor image is rather large as cursors go,
	      ;; and so it won't work on some X servers (such as the MIT
	      ;; R5 Sun server) because servers may have lamentably small
	      ;; upper limits on cursor size.
	      ;;(if (featurep 'xpm)
	      ;; (setq x-gc-pointer-shape
	      ;;  (expand-file-name "trash.xpm" data-directory)))

	      ;; Here's another way to do that: it first tries to load the
	      ;; cursor once and traps the error, just to see if it's
	      ;; possible to load that cursor on this system; if it is,
	      ;; then it sets x-gc-pointer-shape, because we knows that
	      ;; will work.  Otherwise, it doesn't change that variable
	      ;; because we know it will just cause some error messages.
	      (if (featurep 'xpm)
		  (let ((file (expand-file-name "recycle.xpm" data-directory)))
		    (if (condition-case error
			    (make-cursor file) ;returns a cursor if successful.
			  (error nil))	    ; returns nil if an error occurred.
			(setq x-gc-pointer-shape file))))
	 
	      ;; Add `dired' to the File menu
	      (add-menu-item '("File") "Edit Directory" 'dired t)

	      ;; Here's a way to add scrollbar-like buttons to the menubar
					;(add-menu-item nil "Top" 'beginning-of-buffer t)
					;(add-menu-item nil "<<<" 'scroll-down t)
					;(add-menu-item nil " . " 'recenter t)
					;(add-menu-item nil ">>>" 'scroll-up t)
					;(add-menu-item nil "Bot" 'end-of-buffer t)
	      
	      ;; Change the behavior of mouse button 2 (which is normally
	      ;; bound to `mouse-yank'), so that it inserts the selected text
	      ;; at point (where the text cursor is), instead of at the
	      ;; position clicked.
	      ;;
	      ;; Note that you can find out what a particular key sequence or
	      ;; mouse button does by using the "Describe Key..." option on
	      ;; the Help menu.
	      (setq mouse-yank-at-point nil)

	      ;; When editing C code (and Lisp code and the like), I often
	      ;; like to insert tabs into comments and such.  It gets to be
	      ;; a pain to always have to use `C-q TAB', so I set up a more
	      ;; convenient binding.  Note that this does not work in
	      ;; TTY frames.
	      (define-key global-map '(shift tab) 'self-insert-command)

	      ;; LISPM bindings of Control-Shift-C and Control-Shift-E.
	      ;; Note that "\C-C" means Control-C, not Control-Shift-C.
	      ;; To specify shifted control characters, you must use the
	      ;; more verbose syntax used here.
	      (define-key emacs-lisp-mode-map '(control C) 'compile-defun)
	      (define-key emacs-lisp-mode-map '(control E) 'eval-defun)

	      ;; If you like the FSF Emacs binding of button3 (single-click
	      ;; extends the selection, double-click kills the selection),
	      ;; uncomment the following:

	      ;; Under 19.13, the following is enough:
              ;(define-key global-map 'button3 'mouse-track-adjust)

	      ;; But under 19.12, you need this:
              ;(define-key global-map 'button3
              ;    (lambda (event)
              ;      (interactive "e")
              ;      (let ((default-mouse-track-adjust t))
              ;        (mouse-track event))))

	      ;; Under both 19.12 and 19.13, you also need this:
              ;(add-hook 'mouse-track-click-hook
              ;          (lambda (event count)
              ;            (if (or (/= (event-button event) 3)
              ;                    (/= count 2))
              ;                nil ;; do the normal operation
              ;              (kill-region (point) (mark))
              ;              t ;; don't do the normal operations.
              ;              )))
))))

;;; Older versions of emacs did not have these variables
;;; (emacs-major-version and emacs-minor-version.)
;;; Let's define them if they're not around, since they make
;;; it much easier to conditionalize on the emacs version.

(if (and (not (boundp 'emacs-major-version))
	 (string-match "^[0-9]+" emacs-version))
    (setq emacs-major-version
	  (string-to-int (substring emacs-version
				    (match-beginning 0) (match-end 0)))))
(if (and (not (boundp 'emacs-minor-version))
	 (string-match "^[0-9]+\\.\\([0-9]+\\)" emacs-version))
    (setq emacs-minor-version
	  (string-to-int (substring emacs-version
				    (match-beginning 1) (match-end 1)))))

;;; Define a function to make it easier to check which version we're
;;; running.

(defun running-emacs-version-or-newer (major minor)
  (or (> emacs-major-version major)
      (and (= emacs-major-version major)
	   (>= emacs-minor-version minor))))

(defun my-c-mode-common-hook ()
  ;; my customizations for all of c-mode and related modes
  (c-set-offset 'substatement-open 0)
  ;; other customizations can go here
  )
(add-hook 'c-mode-common-hook 'my-c-mode-common-hook)

(defun my-c-mode-common-hook ()
  ;; use Ellemtel style for all C like languages
  (c-set-style "ellemtel")
  (c-set-offset 'innamespace 0)
  
  ;(setq c-echo-syntactic-information-p t)
  (setq c-toggle-auto-hungry-state 1)
  (setq c-auto-newline t)
  (setq tab-width 4)
  ;; other customizations can go here
  (auto-fill-mode)
  )
(add-hook 'c-mode-common-hook 'my-c-mode-common-hook)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;		Customization of Specific Packages		    ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(setq auto-mode-alist
      (append '(("\\.C$"  . c++-mode)
		("\\.cc$" . c++-mode)
		("\\.hh$" . c++-mode)
		("\\.c$"  . c-mode)
		("\\.h$"  . c++-mode)
		("\\.tar$" . tar-mode)
               ("\\.ada$" . ada-mode))
	      auto-mode-alist))

;;; ********************
;;; cc-mode (the mode you're in when editing C, C++, and Objective C files)

;; Tell cc-mode not to check for old-style (K&R) function declarations.
;; This speeds up indenting a lot.
(setq c-recognize-knr-p nil)
(setq c-C++-comment-start-regexp "//\\|/\\*\\|//|///")
(setq c-comment-continuation-stars "* ")
(setq c-macro-preprocessor "/usr/ccs/lib/cpp -C")
(setq c-macro-cppflags "-I/usr/openwin/include -I/usr/dt/include")

;;; ********************
;;; Load a partial-completion mechanism, which makes minibuffer completion
;;; search multiple words instead of just prefixes; for example, the command
;;; `M-x byte-compile-and-load-file RET' can be abbreviated as `M-x b-c-a RET'
;;; because there are no other commands whose first three words begin with
;;; the letters `b', `c', and `a' respectively.
;;;
;(load-library "completer")


;;; ********************
;;; Load crypt, which is a package for automatically decoding and reencoding
;;; files by various methods - for example, you can visit a .Z or .gz file,
;;; edit it, and have it automatically re-compressed when you save it again.
;;; 
(setq crypt-encryption-type 'pgp   ; default encryption mechanism
      crypt-confirm-password t	   ; make sure new passwords are correct
      crypt-never-ever-decrypt t  ; if you don't encrypt anything, set this to
				   ; tell it not to assume that "binary" files
				   ; are encrypted and require a password.
      )
;(require 'crypt)
;(setq crypt-encryption-type 'pgp)

(require 'tar-mode)
(require 'uncompress)
(require 'desktop)

;;; ********************
;;; Edebug is a source-level debugger for emacs-lisp programs.
;;;
(define-key emacs-lisp-mode-map "\C-xx" 'edebug-defun)

;;; ********************
;;; func-menu is a package that scans your source file for function
;;; definitions and makes a menubar entry that lets you jump to any
;;; particular function definition by selecting it from the menu.  The
;;; following code turns this on for all of the recognized languages.
;;; Scanning the buffer takes some time, but not much.
;;;
;;; Send bug reports, enhancements etc to:
;;; David Hughes <ukchugd@ukpmr.cs.philips.nl>
;;;
(cond (running-xemacs
       (require 'func-menu)
       (define-key global-map 'f8 'function-menu)
       (add-hook 'find-file-hooks 'fume-add-menubar-entry)
       (define-key global-map "\C-cl" 'fume-list-functions)
       (define-key global-map "\C-cg" 'fume-prompt-function-goto)

       ;; The Hyperbole information manager package uses (shift button2) and
       ;; (shift button3) to provide context-sensitive mouse keys.  If you
       ;; use this next binding, it will conflict with Hyperbole's setup.
       ;; Choose another mouse key if you use Hyperbole.
       (define-key global-map '(shift button3) 'mouse-function-menu)

       ;; For descriptions of the following user-customizable variables,
       ;; type C-h v <variable>
       (setq fume-max-items 25
             fume-fn-window-position 3
             fume-auto-position-popup t
             fume-display-in-modeline-p t
             fume-menubar-menu-location "File"
             fume-buffer-name "*Function List*"
             fume-no-prompt-on-valid-default nil)
       ))

;;; ********************
;;; resize-minibuffer-mode makes the minibuffer automatically
;;; resize as necessary when it's too big to hold its contents.

;(autoload 'resize-minibuffer-mode "rsz-minibuf" nil t)
;(resize-minibuffer-mode)
;(setq resize-minibuffer-window-exactly nil)

(global-set-key "\^Xd"   'delete-window)
(global-set-key "\^X\^E" 'compile)                    
(global-set-key "\^X\^I" 'insert-file)
(global-set-key "\^X\^N" 'next-error)
(global-set-key "\^X\^P" 'previous-error)
(global-set-key "\e." 'find-tag)
(global-set-key "\eg" 'goto-line)
;(global-set-key "\e[" 'scroll-up-command)
;(global-set-key "\e]" 'scroll-down-command)

(setq ask-about-buffer-names nil)
(setq backup-by-copying-when-linked t)
(setq default-case-fold-search t)	;for searching
(setq default-fill-column 80)
;(setq completion-auto-help nil)
(setq default-truncate-lines nil)
(setq display-time-day-and-date t)
(setq echo-keystrokes 2)
(setq enable-recursive-minibuffers nil)
(setq-default indent-tabs-mode nil)
(setq insert-default-directory t)
(setq mail-yank-prefix ">")
(setq make-backup-files t)
(setq next-screen-context-lines 2)
(setq require-final-newline t)
(setq split-height-threshold 10)
(setq shell-file-name "/bin/csh")
(setq compile-command "make -k")

(define-key minibuffer-local-completion-map " " 'minibuffer-complete)
(define-key minibuffer-local-must-match-map " " 'minibuffer-complete)
(define-key minibuffer-local-completion-map "\t" 'minibuffer-complete-word)
(define-key minibuffer-local-must-match-map "\t" 'minibuffer-complete-word)
 
;;; ********************
;;; W3 is a browser for the World Wide Web, and takes advantage of the very
;;; latest redisplay features in XEmacs.  You can access it simply by typing 
;;; 'M-x w3'; however, if you're unlucky enough to be on a machine that is 
;;; behind a firewall, you will have to do something like this first:

;;
;     (global-set-key "4f" (quote find-file-other-window))
(global-set-key [f14] (quote undo))
(global-set-key [f16] (quote copy-region-as-kill))
(global-set-key [f18] (quote yank))
(global-set-key [f20] (quote kill-region))
(global-set-key [f11] (quote keyboard-quit))
(global-set-key [f19] (quote search-forward-regexp))
(global-set-key [f12] (quote repeat-complex-command))

(add-hook 'text-mode-hook 'auto-fill-mode)

(setq ediff-ignore-similar-regions t)

(defun my-revert () 
  (interactive)
  (revert-buffer t t))

;(global-set-key "\C-x\C-a" 'my-revert)

(setq completion-ignored-extensions
      (delete ".log" completion-ignored-extensions))


(defalias 'insert-sdp-description (read-kbd-macro
                                   "TAB /** RET TAB * RET TAB * RET TAB */ RET TAB"))

(defun insert-c++-header-ifdefs ()
  "insert the initial if defined(...) endif guard for a C/C++ header file"
  (interactive)
  (let ((define-name (replace-in-string (buffer-name) "\\." "_"))
        (class-name (replace-in-string (buffer-name) ".hxx" "")))
    (goto-char (point-min))
    (insert (concat "#if !defined(" define-name ")\n"))
    (insert (concat "#define " define-name "\n"))
    (insert-file "~/emacs/copyright-header")
    (replace-string "XXXFILENAMEXXX" (buffer-name))
    (goto-char (point-max))
    (insert (concat "\n\nclass " class-name))
    (insert "\n{\n   public:\n\n};\n")
    (insert "\n\n#endif\n")
    )
)
(global-set-key [f8] 'insert-c++-header-ifdefs)
(mwheel-install)
(global-font-lock-mode 1)
(setq visible-bell t)
