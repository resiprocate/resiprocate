;;; sourcepair.el --- Load the corresponding C/C++ header or source file for the current buffer.
;;
;; Copyright (C) 2002 Mohamed Hendawi
;;
;; Emacs Lisp Archive Entry
;; Filename: sourcepair.el
;; Version: 1.0
;; Keywords:   c languages oop
;; Author: Mohamed Hendawi <moedev *AT* hendawi *DOT* com>
;; Description: Load the corresponding C/C++ header or source file for the current buffer.
;; URL: http://www.hendawi.com/emacs/sourcepair.el
;; Compatibility: Emacs20, Emacs21
;;
;; $Id: sourcepair.el,v 1.14 2002/07/09 16:41:44 moe Exp $
;;
;; This program is free software; you can redistribute it and/or
;; modify it under the terms of the GNU General Public License as
;; published by the Free Software Foundation; either version 2 of
;; the License, or (at your option) any later version.
;; 
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;; 
;; You should have received a copy of the GNU General Public
;; License along with this program; if not, write to the Free
;; Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
;; MA 02111-1307 USA
;;
;; This code is inspired by a similar function written by Abiyu Diro.
;;
;;; Commentary:
;;
;; This Emacs lisp file provides the function "sourcepair-load" which will load
;; the corresponding header or source file for the current buffer.  For example,
;; if you are looking at the file FooParser.cpp and enter M-x sourcepair-load 
;; (or whatever keybinding you've set), the file FooParser.h will be loaded.  
;; It also works the other way as well.  To use it put this file somewhere 
;; in your lisp library path and then add something like this to your .emacs 
;; file:
;;
;; (load-file "sourcepair.el")
;;
;; KEYBINDINGS:
;;
;; You should set a keybinding to use this function easily.  For example, I add
;; the following to my .emacs file:
;;
;; (define-key global-map "\C-xz" 'sourcepair-load)
;;
;;
;; GLOBAL VARIABLES:
;;
;; There are five global variables that can be used to adjust how the function
;; works:
;;
;; sourcepair-source-extensions :
;;
;;    A list containing the recognized extensions for source files.  By default
;;    this is set to ( ".cpp" ".cxx" ".cc" ".c" ).  For example: with the default 
;;    setting if you are looking at "foo.h", the function will look for 
;;    "foo.cpp", "foo.cxx", "foo.cc" or "foo.c" in that order in the 
;;    directories specified by sourcepair-source-path.
;;
;; sourcepair-header-extensions :
;;
;;    A list containing the recognized extensions for header files.  By default
;;    this is set to ( ".h" ".hpp" ".hh" ).  For example: with the default 
;;    setting if you are looking at "foo.cpp", the function will look for 
;;    "foo.h", "foo.hpp" or "foo.hh" in that order in the directories specified
;;    by sourcepair-header-path.
;;    
;; sourcepair-source-path :
;;
;;    A list containing the path's to search for source files.  By default this
;;    is set to ( "." ) which means source files will only be searched for in
;;    the current directory.  Paths that end in "/*" will be searched 
;;    recursively.  For example, if you specified sourcepair-source-path as
;;    ( "." "../*" ) the function will look for source files first in the 
;;    current directory, and then in the parent directory, and then in any 
;;    subdirectories of the parent directory.
;;
;; sourcepair-header-path :
;;
;;    Similar to sourcepair-source-path except for header files.
;;
;; sourcepair-recurse-ignore :
;;
;;    A list containing directories to ignore when recursively searching
;;    subdirectories for header or source files.  By default this is set
;;    to ( "CVS" )
;;
;;
;; For example, in my .emacs file I have the following:
;;
;; (setq sourcepair-source-path    '( "." "../*" ))
;; (setq sourcepair-header-path    '( "." "include" "../include" "../*"))
;; (setq sourcepair-recurse-ignore '( "CVS" "Obj" "Debug" "Release" ))

;;; Code:

(defcustom sourcepair-source-extensions '( ".cpp" ".cxx" ".cc" ".C" ".c" ".3")
  "*List of recognized extensions for source files.

This variable is used by `sourcepair-load'.  The value should be a list
containing the recognized extensions for source files.  For example: if the
value is ( \".cpp\" \".cxx\" \".cc\" \".C\" \".c\" ), and you are looking at
\"foo.h\", `sourcepair-load' will look for \"foo.cpp\", \"foo.cxx\",
\"foo.cc\" or \"foo.c\" in that order in the directories specified by
`sourcepair-source-path'."
:type '(repeat string))

(defcustom sourcepair-header-extensions '( ".h" ".hpp" ".hxx" ".hh" )
  "*List of recognized extensions for header files.

This variable is used by `sourcepair-load'.  The value should be a list
containing the recognized extensions for header files.  For example: if the
value is (\".h\" \".hpp\" \".hh\" ), and you are looking at \"foo.cpp\",
`sourcepair-load' will look for \"foo.h\", \"foo.hpp\" or \"foo.hh\" in that
order in the directories specified by `sourcepair-header-path'."
:type '(repeat string))

(defcustom sourcepair-source-path       '( "." )
  "*List of directories to search for corresponding source file.

This variable is used by `sourcepair-load'.  The value should be a list
containing the directories to search for source files.  By default this is set
to ( \".\" ) which means source files will only be searched for in the current
directory.  Paths that end in \"/*\" will be searched recursively.  For
example, if you specified `sourcepair-source-path' as ( \".\" \"../*\" )
`sourcepair-load' will look for source files first in the current directory,
and then in the parent directory, and then in any subdirectories of the parent
directory."
:type '(repeat string))

(defcustom sourcepair-header-path       '( "." )
  "*List of directories to search for corresponding header file.

This is similar to `sourcepair-source-path' except for header files.  See the
documentation for `sourcepair-source-path' for more info."
:type '(repeat string))

(defcustom sourcepair-recurse-ignore    '( "CVS" ".svn")
  "*List of directories to ignore when recursively searching subdirectories.

This variable is used by `sourcepair-load'.  The value should be a list
containing the names of directories to ignore when `sourcepair-load' is
recursively searching subdirectories for header or source files.  By default
this is set to ( \"CVS\" )"
:type '(repeat string))

(defun sourcepair-header-file-p (filename)
  "Return t if argument is a C/C++ header file, nil otherwise

This function returns t if the filename specified is a C/C++ header 
file, or nil otherwise.  Header files are identified by extension via 
the variable `sourcepair-header-extensions'."

  (let* ((extension (concat (member ?. (append filename nil))))
		 (basename (substring filename 0 (- 0 (length extension)))))
	(if (member extension sourcepair-header-extensions)
		t
	  nil)))


(defun sourcepair-source-file-p (filename)
  "Return t if argument is a C/C++ source file, nil otherwise

This function returns t if the filename specified is a C/C++ source file,
or nil otherwise.  Source files are identified by extension via the
variable `sourcepair-source-extensions'."

  (let* ((extension (concat (member ?. (append filename nil))))
		 (basename (substring filename 0 (- 0 (length extension)))))
	(if (member extension sourcepair-source-extensions)
		t
	  nil)))


(defun sourcepair-load ()
  "Load the corresponding C/C++ header or source file for the current buffer.

This function can be invoked by \\[sourcepair-load].  It will load the the
corresponding header or source file for the current buffer.  For example, if
you are looking at the file FooParser.cpp and press \\[sourcepair-load], the
file FooParser.h will be loaded.  It also works the other way as well.

There are five global variables that can be used to adjust how the function
works:

 `sourcepair-source-extensions'
 `sourcepair-header-extensions'
 `sourcepair-source-path'
 `sourcepair-header-path'
 `sourcepair-recurse-ignore'

See the documentation for these variables for more info.
"

  (interactive)

  (defun sourcepair-analyze-filename (filename)
	(let* ((extension (concat (member ?. (append filename nil))))
		   (basename (substring filename 0 (- 0 (length extension)))))
	  (if (member extension sourcepair-header-extensions)
		  (cons sourcepair-source-path (mapcar '(lambda (arg) (concat basename arg)) sourcepair-source-extensions))
		(if (member extension sourcepair-source-extensions)
			(cons sourcepair-header-path (mapcar '(lambda (arg) (concat basename arg)) sourcepair-header-extensions))))))

  (defun sourcepair-find-one-of (path choices recurse)
	(catch 'matching-filename
	  (if (file-directory-p path)
		  (let ((possible-filenames choices)
				(matching-filename nil)
				(files-in-directory nil))
			
		  ;; Check if there's a match in this directory
		  (while possible-filenames
			(let ((possible-filename (expand-file-name (car possible-filenames) path)))
			  (if (file-exists-p possible-filename)
				  (throw 'matching-filename possible-filename)
				(setq possible-filenames (cdr possible-filenames)))))

		  ;; Recursively search subdirectories
		  (if (not (eq recurse nil))
			  (progn
				(setq files-in-directory (directory-files path nil "^[^\\.]"))
				(while files-in-directory
				  (let ((possible-subdir (car files-in-directory)))
					(if (not (member possible-subdir sourcepair-recurse-ignore))
						(progn
						  (setq possible-subdir (expand-file-name possible-subdir path))
						  (if (file-directory-p possible-subdir)
							  (progn 
								(message "Checking %s" possible-subdir)
								(setq matching-filename 
									  (sourcepair-find-one-of possible-subdir choices t))
								(if (not (eq matching-filename nil))
									(throw 'matching-filename matching-filename))))))
					(setq files-in-directory (cdr files-in-directory))))))))
	  ;; Return nil if nothing found
	  nil))

  (catch 'found-matching-file
	(let* ((temp (sourcepair-analyze-filename (buffer-file-name)))
		   (search-path (car temp))
		   (possible-filenames (cdr temp)))
	  (if (= (length possible-filenames) 0)
		  (message "%s is not a recognized source or header file (consider updating sourcepair-source-extensions or sourcepair-header-extensions)" (buffer-file-name))
		(progn
		  (while search-path
			(let ((path-to-check (car search-path))
				  (matching-filename nil))
			  (if (and (> (length path-to-check) 3)
					   (equal (substring path-to-check -2) "/*"))
				  (setq matching-filename (sourcepair-find-one-of (substring path-to-check 0 -2)
																  possible-filenames
																  t))
				(setq matching-filename 
					  (sourcepair-find-one-of path-to-check possible-filenames nil)))
			  
			  (if (eq matching-filename nil)
				  (setq search-path (cdr search-path))
				(throw 'found-matching-file (find-file matching-filename)))))

		  (message (concat "No matching file for " (buffer-file-name)
						   " (consider updating sourcepair-source-path, sourcepair-header-path)")))))))


(defun sourcepair-yank-advice ()
  "Advice function called after a yank.

This function is called when advising the yank function.  If you are 
looking at a header file and paste a method declaration that was copied 
from a source file, this function will remove the class prefix (e.g.
\"Foo::\"), add a semicolon at the end of the declaration and reindent the
region.  If you paste something other than a method declaration this 
function will just reindent the region.
"
  (if (member major-mode '(c-mode c++-mode))
	  (if (sourcepair-header-file-p (buffer-file-name))
		  (let* ((this-buffer-file-name (buffer-file-name))
				 (extension (concat (member ?. (append this-buffer-file-name nil))))
				 (basename (substring this-buffer-file-name 0 (- 0 (length extension))))
				 (class-prefix (concat basename "::"))
				 (begin-point  (region-beginning))
				 (end-point    (region-end))
				 (region-len   (- end-point begin-point)))
			(save-excursion
			  (set-window-point nil (- (point) region-len))
			  (if (re-search-forward class-prefix end-point t)
				  (progn
					(replace-match "" nil t)
					(set-window-point nil (- end-point (length class-prefix) 1))
					(re-search-backward "[^ ]")
					(set-window-point nil (+ (point) 1))
					(insert ";")
					(indent-region (region-beginning) (region-end) nil)
					(message "Removed class prefix when pasting"))
				(indent-region begin-point end-point nil))))
		(indent-region (region-beginning) (region-end) nil))))

;;; sourcepair.el ends here

