;;; clipper.el --- save strings of data for further use.

;; Copyright (C) 1997-2000 Free Software Foundation, Inc.

;; Author: Kevin A. Burton (burton@openprivacy.org)
;; Maintainer: Kevin A. Burton (burton@openprivacy.org)
;; Location: http://relativity.yi.org
;; Keywords: clip save text
;; Version: 1.1.1

;; This file is [not yet] part of GNU Emacs.

;; This program is free software; you can redistribute it and/or modify it under
;; the terms of the GNU General Public License as published by the Free Software
;; Foundation; either version 2 of the License, or any later version.
;;
;; This program is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
;; FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
;; details.
;;
;; You should have received a copy of the GNU General Public License along with
;; this program; if not, write to the Free Software Foundation, Inc., 59 Temple
;; Place - Suite 330, Boston, MA 02111-1307, USA.

;;; Commentary:

;; Clipper is a way to handle 'clips' of text with some persistance via handles.
;; A good example is something like the GNU Public License.  If you do a lot of
;; Free Software work and need to have a copy of the GPL for insertion in your
;; source files, you can save this text as a 'GPL' clip.  When you call
;; clipper-insert you will be prompted for a name and when you enter GPL this
;; will be inserted.
;;
;; Clipper can also perform search and replacement on token names.  For example
;; if want the current buffer filename you can use the token
;; CLIPPER_FILE_NAME_NONDIRECTORY.
;;
;; Available tokens are:
;;
;;     CLIPPER_FILE_NAME_NONDIRECTORY
;;
;;         The current filename without it's directory.  If this buffer isn't
;;         backed on disk then the buffer name is used.
;;
;;     CLIPPER_FILE_NAME_NONDIRECTORY_SANS_EXTENSION
;;
;;         The current filename without it's directory and without an extension.

;;; Usage:
;;
;; install via (require 'clipper) in your .emacs file.
;;
;; The following functions allow you to manipulate clipper:
;;
;; `clipper-create' create a new clip
;;
;; `clipper-delete' delete an existing clip
;;
;; `clipper-insert' insert a clip into the current buffer
;;
;; `clipper-edit-clip' edit an existing clip.
;;
;; You might also want to setup personal key bindings:
;;
;; (global-set-key "\C-cci" 'clipper-insert)
;; (global-set-key "\C-ccc" 'clipper-create)


;;; TODO

;; sort the alist with `sort'

;;; History:
;;
;; - Wed Jan 30 2002 03:14 PM (burton@openprivacy.org): fixed a bug WRT data
;; loss when editing existing clips.
;; 
;; - Sun Nov 04 2001 05:33 PM (burton@openprivacy.org): we are now supporting a
;; file-name-nondirectory in special buffers.
;;
;; - Sun Nov 04 2001 05:31 PM (burton@openprivacy.org): clipper-save was not
;; smart enough.  We now save-excursion, use find-file-noselect and localize
;; find-file-hooks so that saves are cleaner and faster.
;; 
;; - Sat Mar 17 00:02:18 2001 (burton@relativity.yi.org): migrate to load-file
;; instead of manually evaluating the file
;; 
;; - Tue Jan  2 03:51:45 2001 (burton): Version 1.0.1.  Added support for editing
;;   clips thanks to a prototype function provided by Joe Humrickhouse
;;   <humr0002@tc.umn.edu> which was modularized with the current creation
;;   function.  Added fontlock for the input buffer.

;;
;;; Code:

(require 'font-lock)

(defvar clipper-alist '() "Associated list for holding clips.")

(defvar clipper-file "~/.clipper.el" "File used for saving clipper information.")

(defvar clipper-input-buffer "*clipper input*" "Buffer used for entering new clips.")

(defvar clipper-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map "\C-c\C-c" 'clipper-complete-input)
    map)
  "Mode specific keymap for `clipper-mode'.")

(defvar clipper-mode-string "Clipper" "Mode name for clipper.")

(defvar clipper-input-message "" "Value for the clipper input buffer.")
(if (equal clipper-input-message "")
    (setq clipper-input-message
          (concat clipper-input-message
                  "CLIPPER: --------------------------------------------------------------------------\n"
                  "CLIPPER: Lines beginning with `CLIPPER:' are removed automatically.\n"
                  "CLIPPER: Enter new clip.  Type C-c C-c when complete.\n"
                  "CLIPPER: \n"
                  "CLIPPER: The following variables are supported: \n"
                  "CLIPPER: \n"
                  "CLIPPER:      CLIPPER_FILE_NAME_NONDIRECTORY\n"
                  "CLIPPER: \n"
                  "CLIPPER:         The current filename without it's directory.  If this buffer isn't\n"
                  "CLIPPER:         backed on disk then the buffer name is used.\n"
                  "CLIPPER: \n"
                  "CLIPPER:      CLIPPER_FILE_NAME_NONDIRECTORY_SANS_EXTENSION\n"
                  "CLIPPER: \n"
                  "CLIPPER:         The current filename without it's directory and without an extension\n"
                  "CLIPPER: \n")))

(defun clipper-save()
  "Save the clipper information to file."

  (save-excursion

    (let((find-file-hooks nil))    
    
      (set-buffer (find-file-noselect clipper-file))
      
      ;;whatever is in this buffer is now obsolete
      (erase-buffer)

      (insert "(setq clipper-alist '")
      (prin1 clipper-alist (current-buffer))
      (insert ")")
      (save-buffer)
      (kill-buffer (current-buffer))
      
      (message "Wrote %s" clipper-file))))
  
(defun clipper-delete()
  "Delete an existing 'clip'"
  (interactive)

  (let (clip)

    ;; get the clipper to delete
    (setq clip (clipper-get-clip))

    (if (yes-or-no-p (format "Are you sure you want to delete clip: %s? " clip))
        (progn

          ;;remove it...
          (setq clipper-alist (delq (assoc (intern clip) clipper-alist) clipper-alist))
          
          ;;save the alist to disk
          (clipper-save)))))

(defun clipper-create()
  "Create a new 'clip' for use within Emacs"
  (interactive)

  (set-buffer (get-buffer-create clipper-input-buffer))
  (erase-buffer) ;; just in case

  (clipper-mode)

  (setq clipper-clip-name (read-string "Name of new clip: "))

  ;;make sure the clip that the user just specified doesn't already exist.
  (if (null (assoc (intern clipper-clip-name) clipper-alist))
      (progn

        (insert clipper-input-message)

        (pop-to-buffer clipper-input-buffer)
        (goto-char (point-max))

        (message "Enter new clip.  Type C-c C-c when complete."))
    (error "The specified clip already exists")))

(defun clipper-complete-input()
  "Called when the user is done entering text. "
  (interactive)
  
  (set-buffer (get-buffer-create clipper-input-buffer))

  ;;make sure font-lock is off in this buffer
  (font-lock-mode -1)
  
  ;;clean up the input buffer by removing comment lines.
  (save-excursion
    (goto-char (point-min))
    (while (re-search-forward "^CLIPPER: .*$" nil t)
      (delete-region (match-beginning 0) (match-end 0))
      (kill-line 1)))

  ;;now get the value of the buffer.
  (let(clipper-input begin end)

    (save-excursion
      (goto-char (point-min))
      (setq begin (point))
      (goto-char (point-max))
      (setq end (point)))

    (setq clipper-input (buffer-string))

    (add-to-list 'clipper-alist (cons (intern clipper-clip-name) clipper-input)))

  ;;now clean up...
  (kill-buffer clipper-input-buffer)
  (delete-window)

  (clipper-save))

(defun clipper-insert(clip-name)
  "Insert a new 'clip' into the current buffer"
  (interactive
   (list
    (clipper-get-clip)))

  (let (value insert-start insert-end)

    ;;the insert start and insert end variables keep track of where things were
    ;;inserted.
    
    (setq insert-start (point))
    
    (setq value (assoc (intern clip-name) clipper-alist))

    (insert (cdr value))

    (setq insert-end (point))

    (clipper-replace-tokens insert-start insert-end)))

(defun clipper-mode()
  "Mode for entering data into a 'clip'."

  (kill-all-local-variables)
  (use-local-map clipper-mode-map)

  (setq major-mode 'clipper-mode)
  (setq mode-name clipper-mode-string)

  (setq clipper-mode t)

  (run-hooks 'clipper-mode-hook)
  (font-lock-mode 1))

(defun clipper-restore()
  "Read the clipper data file from disk"
  (interactive)
  (when (file-readable-p clipper-file)
    (message "Reading %s..." clipper-file)
    
    (load-file clipper-file)
    
    (message "Reading %s...done" clipper-file)))

(defun clipper-get-clip()
  "Use completion to ask the user for a clip"

  ;;build a list for completion
  (let((completion-list nil)
       (index 1)
       (clip-name nil))
    (dolist(clip clipper-alist)

      (setq clip-name (symbol-name (car clip) ))

      (add-to-list 'completion-list
                   (list clip-name index))
      
      (setq index (1+ index)))

    (completing-read "Clip name: " completion-list nil t)))

(defun clipper-edit-clip(name)
  "Edit an existing clip.  Note that your clip MUST be saved even if
you don't edit it.  Otherwise the clip will be DELETED for good."
  (interactive
   (list
    (clipper-get-clip)))

  (setq clipper-clip-name name)
  (set-buffer (get-buffer-create clipper-input-buffer))
  (erase-buffer)
  (clipper-mode)

  (insert clipper-input-message)
  (setq value (assoc (intern name) clipper-alist))
  (insert (cdr value))
  (pop-to-buffer clipper-input-buffer)
  (goto-char (point-min)))

(defun clipper-replace-tokens(start end)
  "Search and replace clipper tokens in this buffer."

  (save-excursion
    (save-restriction

      (narrow-to-region start end)

      (goto-char (point-min))

      (let(file-name-nondirectory file-name-nondirectory-san-extension)

        (if (buffer-file-name)
            (setq file-name-nondirectory (file-name-nondirectory (buffer-file-name)))
          (setq file-name-nondirectory (buffer-name)))

        (if (null file-name-nondirectory)
            (setq file-name-nondirectory (buffer-name)))

        (setq file-name-nondirectory-san-extension (file-name-sans-extension file-name-nondirectory))
        
        ;;---------
        ;;setup the file-name-nondirectory extension
        (save-excursion

          (goto-char (point-min))
          
          (while (re-search-forward " \\(CLIPPER_FILE_NAME_NONDIRECTORY\\) " nil t)
            (replace-match file-name-nondirectory t nil nil 1)))

        ;;---------
        (save-excursion

          (goto-char (point-min))

          (while (re-search-forward "\\(CLIPPER_FILE_NAME_NONDIRECTORY_SANS_EXTENSION\\)" nil t)

            (replace-match file-name-nondirectory-san-extension t nil nil 1)))))))

;;initialze clipper
(clipper-restore)
;(font-lock-add-keywords 'clipper-mode '(("\\(^CLIPPER.*\\)" 1 'font-lock-comment-face t)))
(provide 'clipper)

;;; clipper.el ends here
