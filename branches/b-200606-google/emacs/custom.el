(custom-set-variables
 '(query-user-mail-address nil))
(custom-set-faces)

;(autoload 'bhl-mode "bhl" "BHL Mode" t)
;(add-to-list 'auto-mode-alist '("\\.bhl$" . bhl-mode))
(if (not running-xemacs)
    (add-to-list 'vc-handled-backends 'SVN))

(if (not running-xemacs)
  (require 'psvn))

(require 'clipper)
(require 'vc)

(global-set-key "\C-cci" 'clipper-insert)
(global-set-key "\C-ccc" 'clipper-create)

(autoload 'hide-copyleft-region   "hide-copyleft" nil t)
(autoload 'unhide-copyleft-region "hide-copyleft" nil t)
(add-hook 'emacs-lisp-mode-hook 'hide-copyleft-region)
(add-hook 'c-mode-hook 'hide-copyleft-region)
(add-hook 'c++-mode-hook 'hide-copyleft-region)
(add-hook 'java-mode-hook 'hide-copyleft-region)


