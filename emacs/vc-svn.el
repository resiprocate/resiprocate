;;;; vc-svn.el --- a VC backend for Subversion
;;;; Jim Blandy <jimb@red-bean.com> --- July 2002

;;; Writing this back end has shown up some problems in VC: bugs,
;;; shortcomings in the back end interface, and so on.  But I want to
;;; first produce code that Subversion users can use with an already
;;; released Emacs distribution.
;;;
;;; So for now we're working within the limitations of the released
;;; VC; once we've got something functional, then we can start writing
;;; VC patches.


;;; To make this file load on demand, put this file into a directory
;;; in `load-path', and add this line to a startup file:
;;;
;;;     (add-to-list 'vc-handled-backends 'SVN)


;;; To do here:
;;; Provide more of the optional VC backend functions: 
;;; - dir-state
;;; - merge across arbitrary revisions
;;;
;;; Maybe we want more info in mode-line-string.  Status of props?  Status 
;;; compared to what's in the repository (svn st -u) ? 
;;;
;;; VC passes the vc-svn-register function a COMMENT argument, which
;;; is like the file description in CVS and RCS.  Could we store the
;;; COMMENT as a Subversion property?  Would that show up in fancy DAV
;;; web folder displays, or would it just languish in obscurity, the
;;; way CVS and RCS descriptions do?
;;;
;;; After manual merging, need some way to run `svn resolved'.  Perhaps
;;; we should just prompt for approval when somebody tries to commit a
;;; conflicted file?
;;;
;;; vc-svn ought to handle more gracefully an attempted commit that
;;; fails with "Transaction is out of date".  Probably the best
;;; approach is to ask "file is not up-to-date; do you want to merge
;;; now?"  I think vc-cvs does this.
;;;
;;; Perhaps show the "conflicted" marker in the modeline?
;;;
;;; If conflicted, before committing or merging, ask the user if they
;;; want to mark the file as resolved.
;;;
;;; Won't searching for strings in svn output cause trouble if the
;;; locale language is not English?
;;;
;;; After merging news, need to recheck our idea of which workfile
;;; version we have.  Reverting the file does this but we need to
;;; force it.  Note that this can be necessary even if the file has
;;; not changed.
;;;
;;; Does everything work properly if we're rolled back to an old
;;; revision?
;;;
;;; Perhaps need to implement vc-svn-latest-on-branch-p?


;;; To do in VC:
;;;
;;; Make sure vc's documentation for `workfile-unchanged-p' default
;;; function mentions that it must not run asynchronously, and the
;;; symptoms if it does.
;;; 
;;; Fix logic for finding log entries.
;;;
;;; Allow historical diff to choose an appropriate default previous
;;; revision number.  I think this entails moving vc-previous-revision
;;; et al into the back end.
;;;
;;; Should vc-BACKEND-checkout really have to set the workfile version
;;; itself?
;;;
;;; Fix smerge for svn conflict markers.
;;;
;;; We can have files which are not editable for reasons other than
;;; needing to be checked out.  For example, they might be a read-only
;;; view of an old revision opened with [C-x v ~].  (See vc-merge)
;;;
;;; Would be nice if there was a way to mark a file as
;;; just-checked-out, aside from updating the checkout-time property
;;; which in theory is not to be changed by backends.


(add-to-list 'vc-handled-backends 'SVN)

(defcustom vc-svn-program-name "svn"
  "*Name of Subversion client program, for use by Emacs's VC package."
  :type 'string
  :group 'vc
  :version "21.2.90.2")

(defcustom vc-svn-diff-switches nil
  "*A string or list of strings specifying extra switches for `svn diff' under VC."
  :type '(repeat string)
  :group 'vc
  :version "21.2.90.2")

(defun vc-svn-registered (file)
  "Return true if FILE is registered under Subversion."
  ;; First, a quick false positive test: is there a `.svn/entries' file?
  (and (file-exists-p (expand-file-name ".svn/entries"
                                        (file-name-directory file)))
       (not (null (vc-svn-run-status file)))))


(put 'vc-svn-with-output-buffer 'lisp-indent-function 0)
(defmacro vc-svn-with-output-buffer (&rest body)
  "Save excursion, switch to buffer ` *Subversion Output*', and erase it."
  `(save-excursion
     ;; Let's not delete this buffer when we're done --- leave
     ;; it around for debugging.
     (set-buffer (get-buffer-create " *Subversion Output*"))
     (erase-buffer)
     ,@body))


(defun vc-svn-pop-up-error (&rest args)
  "Pop up the Subversion output buffer, and raise an error with ARGS."
  (pop-to-buffer " *Subversion Output*")
  (goto-char (point-min))
  (shrink-window-if-larger-than-buffer)
  (apply 'error args))


(defun vc-svn-run-status (file &optional update)
  "Run `svn status -v' on FILE, and return the result.
If optional arg UPDATE is true, pass the `-u' flag to check against
the repository, across the network.
See `vc-svn-parse-status' for a description of the result."
  (vc-svn-with-output-buffer

    ;; We should call vc-do-command here, but Subversion exits with an
    ;; error status if FILE isn't under its control, and we want to
    ;; return that as nil, not display it to the user.  We can tell
    ;; vc-do-command to
    
    (let ((status (apply 'call-process vc-svn-program-name nil t nil
                         (append '("status" "-v")
                                 (if update '("-u") '())
                                 (list file)))))
      (goto-char (point-min))
      (if (not (equal 0 status)) ; not zerop; status can be a string
          ;; If you ask for the status of a file that isn't even in a
          ;; Subversion-controlled directory, then Subversion exits with
          ;; this error.
          (if (or (looking-at "\\(.*\n\\)*.*is not a working copy directory")
                  (looking-at "\\(.*\n\\)*.*is not a versioned resource")
                  (looking-at "\\(.*\n\\)*.*: No such file or directory"))
              nil
            ;; Other errors we should actually report to the user.
            (vc-svn-pop-up-error
             "Error running Subversion to check status of `%s'"
             (file-name-nondirectory file)))

        ;; Otherwise, we've got valid status output in the buffer, so
        ;; just parse that.
        (vc-svn-parse-status)))))


(defun vc-svn-parse-status ()
  "Parse the output from `svn status -v' at point.
We return nil for a file not under Subversion's control,
or (STATE LOCAL CHANGED) for files that are, where:
STATE is the file's VC state (see the documentation for `vc-state'),
LOCAL is the base revision in the working copy, and
CHANGED is the last revision in which it was changed.
Both LOCAL and CHANGED are strings, not numbers.
If we passed `svn status' the `-u' flag, then CHANGED could be a later
revision than LOCAL.
If the file is newly added, LOCAL is \"0\" and CHANGED is nil."
  (let ((state (vc-svn-parse-state-only)))
    (cond
     ((not state) nil)
     ;; A newly added file has no revision.
     ((looking-at "....\\s-+\\(\\*\\s-+\\)?[-0]\\s-+\\?")
      (list state "0" nil))
     ((looking-at "....\\s-+\\(\\*\\s-+\\)?\\([0-9]+\\)\\s-+\\([0-9]+\\)")
      (list state
            (match-string 2)
            (match-string 3)))
     ((looking-at "^I +") nil)       ;; An ignored file
     ((looking-at " \\{40\\}") nil)  ;; A file that is not in the wc nor svn?
     (t (error "Couldn't parse output from `svn status -v'")))))


(defun vc-svn-parse-state-only ()
  "Parse the output from `svn status -v' at point, and return a state.
The documentation for the function `vc-state' describes the possible values."
  (cond
   ;; Be careful --- some of the later clauses here could yield false
   ;; positives, if the clauses preceding them didn't screen those
   ;; out.  Making a pattern more selective could break something.

   ;; nil                 The given file is not under version control,
   ;;                     or does not exist.
   ((looking-at "\\?\\|^$") nil)

   ;; 'needs-patch        The file has not been edited by the
   ;;                     user, but there is a more recent version
   ;;                     on the current branch stored in the
   ;;                     master file.
   ((looking-at "  ..\\s-+\\*") 'needs-patch)

   ;; 'up-to-date         The working file is unmodified with
   ;;                     respect to the latest version on the
   ;;                     current branch, and not locked.
   ;;
   ;;                     This is also returned for files which do not
   ;;                     exist, as will be the case when finding a
   ;;                     new file in a svn-controlled directory.  That
   ;;                     case is handled in vc-svn-parse-status.
   ((looking-at "  ") 'up-to-date)

   ;; 'needs-merge        The file has been edited by the user,
   ;;                     and there is also a more recent version
   ;;                     on the current branch stored in the
   ;;                     master file.  This state can only occur
   ;;                     if locking is not used for the file.
   ((looking-at "\\S-+\\s-+\\*") 'needs-merge)

   ;; 'edited             The working file has been edited by the
   ;;                     user.  If locking is used for the file,
   ;;                     this state means that the current
   ;;                     version is locked by the calling user.
   (t 'edited)))


;;; Is it really safe not to check for updates?  I haven't seen any
;;; cases where failing to check causes a problem that is not caught
;;; in some other way.  However, there *are* cases where checking
;;; needlessly causes network delay, such as C-x v v.  The common case
;;; is for the commit to be OK; we can handle errors if they occur. -- mbp
(defun vc-svn-state (file)
  "Return the current version control state of FILE.
For a list of possible return values, see `vc-state'.

This function should do a full and reliable state computation; it is
usually called immediately after `C-x v v'.  `vc-svn-state-heuristic'
provides a faster heuristic when visiting a file.

For svn this does *not* check for updates in the repository, because
that needlessly slows down vc when the repository is remote.  Instead,
we rely on Subversion to trap situations such as needing a merge
before commit."
  (car (vc-svn-run-status file)))


(defun vc-svn-state-heuristic (file)
  "Estimate the version control state of FILE at visiting time.
For a list of possible values, see the doc string of `vc-state'.
This is supposed to be considerably faster than `vc-svn-state'.  It
just runs `svn status -v', without the `-u' flag, so it's a strictly
local operation."
  (car (vc-svn-run-status file)))



(defun vc-svn-workfile-version (file)
  "Return the current workfile version of FILE."
  (cadr (vc-svn-run-status file)))


(defun vc-svn-checkout-model (file)
  'implicit)


(defun vc-svn-register (file &optional rev comment)
  "Register FILE with Subversion.
REV is an initial revision; Subversion ignores it.
COMMENT is an initial description of the file; currently this is ignored."
  (vc-svn-with-output-buffer
    (let ((status (call-process vc-svn-program-name nil t nil "add" file)))
      (or (equal 0 status) ; not zerop; status can be a string
          (vc-svn-pop-up-error "Error running Subversion to add `%s'"
                               (file-name-nondirectory file))))))


(defun vc-svn-checkin (file rev comment)
  (apply 'vc-do-command nil 0 vc-svn-program-name file 
         "commit" (if comment (list "-m" comment) '())))


(defun vc-svn-checkout (file &optional editable rev destfile)
  "Check out revision REV of FILE into the working area.
The EDITABLE argument must be non-nil, since Subversion doesn't
support locking.
If REV is non-nil, that is the revision to check out (default is
current workfile version).  If REV is the empty string, that means to
check out the head of the trunk.  For Subversion, that's equivalent to
passing nil.
If optional arg DESTFILE is given, it is an alternate filename to
write the contents to; we raise an error."
  (unless editable
    (error "VC asked Subversion to check out a read-only copy of file"))
  (when destfile
    (error "VC asked Subversion to check out a file under another name"))
  (when (equal rev "")
    (setq rev nil))
  (apply 'vc-do-command nil 0 vc-svn-program-name file
         "update" (if rev (list "-r" rev) '()))
  (vc-file-setprop file 'vc-workfile-version nil))


(defun vc-svn-revert (file &optional contents-done)
  "Revert FILE back to the current workfile version.
If optional arg CONTENTS-DONE is non-nil, then the contents of FILE
have already been reverted from a version backup, and this function
only needs to update the status of FILE within the backend.  This
function ignores the CONTENTS-DONE argument."
  (vc-do-command nil 0 vc-svn-program-name file "revert"))


(defun vc-svn-merge-news (file)
  "Merge recent changes into FILE.

This calls `svn update'.  In the case of conflicts, Subversion puts
conflict markers into the file and leaves additional temporary files
containing the `ancestor', `mine', and `other' files.

You may need to run `svn resolved' by hand once these conflicts have
been resolved.  

Returns a vc status, which is used to determine whether conflicts need
to be merged."
  (prog1
      (vc-do-command nil 0 vc-svn-program-name file "update")
    
    ;; This file may not have changed in the revisions which were
    ;; merged, which means that its mtime on disk will not have been
    ;; updated.  However, the workfile version may still have been
    ;; updated, and we want that to be shown correctly in the
    ;; modeline.

    ;; vc-cvs does something like this
    (vc-file-setprop file 'vc-checkout-time 0)
    (vc-file-setprop file 'vc-workfile-version
                     (vc-svn-workfile-version file))))


(defun vc-svn-print-log (file)
  "Insert the revision log of FILE into the *vc* buffer."
  (vc-do-command nil 'async vc-svn-program-name file "log"))


(defun vc-svn-show-log-entry (version)
  "Search the log entry for VERSION in the current buffer.
Make sure it is displayed in the buffer's window."
  (when (re-search-forward (concat "^-+\n\\(rev\\) "
                                   (regexp-quote version)
                                   ":[^|]+|[^|]+| [0-9]+ lines?"))
    (goto-char (match-beginning 1))
    (recenter 1)))


(defun vc-svn-diff (file &optional rev1 rev2)
  "Insert the diff for FILE into the *vc-diff* buffer.
If REV1 and REV2 are non-nil, report differences from REV1 to REV2.
If REV1 is nil, use the current workfile version (as found in the
repository) as the older version; if REV2 is nil, use the current
workfile contents as the newer version.
This function returns a status of either 0 (no differences found), or
1 (either non-empty diff or the diff is run asynchronously)."
  (let* ((diff-switches-list
          ;; In Emacs 21.3.50 or so, the `vc-diff-switches-list' macro
          ;; started requiring its symbol argument to be quoted.
          (condition-case nil
              (vc-diff-switches-list svn)
            (void-variable (vc-diff-switches-list 'SVN))))
         (status (vc-svn-run-status file))
         (local (elt status 1))
         (changed (elt status 2))
         
         ;; If rev1 is the default (the base revision) set it to nil.
         ;; This is nice because it lets us recognize when the diff
         ;; will run locally, and thus when we shouldn't bother to run
         ;; it asynchronously.  But it's also necessary, since a diff
         ;; for vc-default-workfile-unchanged-p *must* run
         ;; synchronously, or else you'll end up with two diffs in the
         ;; *vc-diff* buffer.  `vc-diff-workfile-unchanged-p' passes
         ;; the base revision explicitly, but this kludge lets us
         ;; recognize that we can run the diff synchronously anyway.
         ;; Fragile, no?
         (rev1 (if (and rev1 (not (equal rev1 local))) rev1))

         (rev-switches-list
          (cond
           ;; Given base rev against given rev.
           ((and rev1 rev2) (list "-r" (format "%s:%s" rev1 rev2)))
           ;; Given base rev against working copy.
           (rev1 (list "-r" rev1))
           ;; Working copy base against given rev.
           (rev2 (list "-r" (format "%s:%s" local rev2)))
           ;; Working copy base against working copy.
           (t '())))

         ;; Run diff asynchronously if we're going to have to go
         ;; across the network.
         (async (or rev1 rev2)))

    (let ((status (apply 'vc-do-command "*vc-diff*" (if async 'async 0)
                         vc-svn-program-name file
                         (append '("diff") rev-switches-list))))
      (if (or async (> (buffer-size (get-buffer "*vc-diff*")) 0))
          1 0))))

(defun vc-svn-find-version (file rev buffer)
  (vc-do-command buffer 0 vc-svn-program-name file 
         "cat" "-r" rev))

(provide 'vc-svn)
