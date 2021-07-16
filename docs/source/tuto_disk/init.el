
(package-initialize)
(add-to-list 'package-archives
		 '("gnu" . "https://elpa.gnu.org/packages/"))
(add-to-list 'package-archives
		 '("melpa-stable" . "https://stable.melpa.org/packages/"))
(add-to-list 'package-archives
		 '("melpa" . "https://melpa.org/packages/"))
(add-to-list 'load-path "/source/ox-rst.git/")
(setq package-archive-priorities '(("melpa-stable" . 100)
                                   ("melpa" . 50)
                                   ("gnu" . 10)))

(require 'org)
(require 'ox-rst)

(defun auto-fill-mode-on () (TeX-PDF-mode 1))
(add-hook 'tex-mode-hook 'TeX-PDF-mode-on)
(add-hook 'latex-mode-hook 'TeX-PDF-mode-on)
(setq TeX-PDF-mode t)

(defun auto-fill-mode-on () (auto-fill-mode 1))
(add-hook 'text-mode-hook 'auto-fill-mode-on)
(add-hook 'emacs-lisp-mode 'auto-fill-mode-on)
(add-hook 'tex-mode-hook 'auto-fill-mode-on)
(add-hook 'latex-mode-hook 'auto-fill-mode-on)

(global-set-key (kbd "C-c l") 'org-store-link)

;; In org-mode 9 you need to have #+PROPERTY: header-args :eval never-export 
;; in the beginning or your document to tell org-mode not to evaluate every 
;; code block every time you export.
(setq org-confirm-babel-evaluate nil) ;; Do not ask for confirmation all the time!!


(org-babel-do-load-languages
 'org-babel-load-languages
 '(
   (emacs-lisp . t)
   (shell . t)
   (python . t)
   (R . t)
   (ruby . t)
   (ocaml . t)
   (ditaa . t)
   (dot . t)
   (octave . t)
   (sqlite . t)
   (perl . t)
   (screen . t)
   (plantuml . t)
   (lilypond . t)
   (org . t)
   (makefile . t)
   ))
(setq org-src-preserve-indentation t)

(add-hook 'org-babel-after-execute-hook 'org-display-inline-images) 
(add-hook 'org-mode-hook 'org-display-inline-images)
(add-hook 'org-mode-hook 'org-babel-result-hide-all)

(global-set-key (kbd "C-c S-t") 'org-babel-execute-subtree)
