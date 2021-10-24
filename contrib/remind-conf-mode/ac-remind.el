;;; setup for remind autocompletion (totally optional)

;; put something like 
;; (add-hook 'remind-conf-mode '(load-library "ac-remind")) in your .emacs file.
(require 'auto-complete)

(define-key ac-complete-mode-map "\r" nil)

(defvar ac-remind-keywords
  '((candidates
     . (lambda ()
	 (all-completions ac-target remind-keywords ))))
  "Source for remind-conf completion keywords.")

(defvar ac-remind-time-words
'((candidates
   . (lambda ()
       (all-completions ac-target remind-time-words))))
"Source for remind-conf time words completions.")

(defvar ac-remind-builtin-variables
  '((candidates
	 . (lambda ()
	     (all-completions ac-target remind-builtin-variables))))
  "Source for remind-conf builtin variables.")

(defvar ac-remind-type-keywords
  '((candidates
     . (lambda ()
	 (all-completions ac-target remind-type-keywords))))
  "Source for remind-conf type keywords.")

(defvar ac-remind-builtin-functions
 '((candidates
	 . (lambda ()
	     (all-completions ac-target remind-builtin-functions))))
 "Source for remind-conf completion builtin functions.")
	       
(add-hook 'remind-conf-mode-hook
	  (lambda () "Makes auto-completion work in remind-conf-mode"
	    (make-local-variable 'ac-sources)
	    (setq ac-sources '(ac-remind-keywords
			       ac-remind-builtin-variables
			       ac-remind-builtin-functions
			       ac-remind-type-keywords
			       ac-remind-time-words
			       ac-source-abbrev))		
	    (auto-complete-mode 1)))

(provide 'ac-remind)
;; (define-skeleton ac-look
;; ""
;; (skeleton-read "well? ")
;; "(when (looking-at (regexp-opt remind-" str " 'words))" \n
;; >"(setq ac-sources '(ac-remind-" str ")))"
;; )