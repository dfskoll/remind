;;; remind-conf-mode.el --- A mode to help configure remind.

;; Copyright (C) 2008  Shelagh Manton <shelagh.manton@gmail.com>

;; Author: Shelagh Manton <shelagh.manton@gmail.com> with help from
;; David F. Skoll
;; Keywords: remind configure mode
;; Version: .04

;; This program is free software; you can redistribute it and/or
;; modify it under the terms of the GNU General Public License
;; as published by the Free Software Foundation; either version 2
;; of the License, or (at your option) any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program; if not, write to the Free Software
;; Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
;; 02111-1307, USA.

;;; Commentary:

;; Use this mode to help with the configuration of remind configuration files.
;; Put (require 'remind-conf-mode) in your .emacs file
;; or (autoload 'remind-conf-mode "remind-conf-mode" "Mode to help with remind files" t)
;; also put (add-to-list 'auto-mode-alist '("\\.rem\\'" . remind-conf-mode)) and
;; (setq auto-mode-alist
;;     (cons '(".reminders$" . remind-conf-mode) auto-mode-alist))
;; if you want to have the mode work automatically when you open a remind configuration file.

;; If you want to use the auto-complete stuff, you will need to download and install the
;; auto-complete library from  http://www.cx4a.org/pub/auto-complete.el and put
;; (require 'auto-complete) in your emacs with
;; (add-hook 'remind-conf-mode-hook
;;  (lambda ()
;;    (make-local-variable 'ac-sources)
;;    (setq ac-sources '(ac-remind-conf ac-remind-builtin-variables ac-remind-builtin-functions))
;;    (auto-complete t)))
;;   in your .emacs file

;;   PS. you could add ac-source-abbrev ac-source-words-in-buffer to have abbrevs and
;;   other words in buffer auto-complete too

;;; History:
;; Thu, Feb 14, 2008
;; Based mode on wpld-mode tutorial and sample-mode on emacs wiki.
;; Ideas from mupad.el for font-lock styles.
;; Mon, Jan 26, 2008
;; Added rem-setup-colors to make it easy for colourised remind output.
;; Added a demo skeleton for people to copy for easy entry of coloured remind entries.
;; tried to hook in the auto-complete library so that all known functions and keywords can be easily entered.
;; EXPERIMENTAL, but seems to work well here (emacs cvs).
;; Seems to work without case folding which is nice. wonder why it didn't yesterday?

;;; Code:


(require 'font-lock); this goes in the define-derived-mode part.
(when (featurep 'xemacs)
  (require 'overlay)) ;I wonder if this will help with font-lock and xemacs?


(defgroup remind-conf nil
  "Options for remind-conf-mode."
  :group 'remind-conf
  :prefix "remind-conf-")


(defvar remind-conf-mode-hook nil
  "Hook to run in `remind-conf-mode'.")

;; keymap

(defvar remind-conf-mode-map
  (let ((remind-conf-mode-map (make-sparse-keymap)))
    remind-conf-mode-map)
  "Keymap for `remind-conf-mode'.")

(define-key remind-conf-mode-map "\C-cr" 'rem-skel)
(define-key remind-conf-mode-map "\C-ct" 'rem-today)
(define-key remind-conf-mode-map "\C-cd" 'rem-today-skel)
(define-key remind-conf-mode-map "\C-cw" 'rem-week-away)
(define-key remind-conf-mode-map "\C-cx" 'rem-tomorrow)
(define-key remind-conf-mode-map "\C-ca" 'rem-days-away)
(define-key remind-conf-mode-map "\M-j" 'remind-indent-line)
(define-key remind-conf-mode-map (kbd "RET") 'remind-indent-line)
(define-key remind-conf-mode-map "\C-c\C-c" 'rem-save-file)

;; syntax-table

(defvar remind-conf-syntax-table
  (let ((remind-conf-syntax-table (make-syntax-table text-mode-syntax-table)))
    (modify-syntax-entry ?\; ". 1b" remind-conf-syntax-table)
    (modify-syntax-entry ?\# ". 1b" remind-conf-syntax-table)
    (modify-syntax-entry ?\n "> b" remind-conf-syntax-table)
					;Names with _ are still one word.
    (modify-syntax-entry ?_ "w" remind-conf-syntax-table)
    (modify-syntax-entry ?. "w" remind-conf-syntax-table)
       remind-conf-syntax-table)
  "Syntax table for `remind-conf-mode'.")

;;; keyword sets

(defconst remind-keywords
  (sort
   (list "RUN" "REM" "ONCE" "SATISFY" "BEFORE" "UNSET" "OMIT"
	 "OMIT" "DATE" "SKIP" "ONCE" "AFTER" "WARN" "PRIORITY" "AT" "SCHED" "IF" "ELSE" "ENDIF"
	 "WARN" "UNTIL" "THROUGH" "SCANFROM" "DURATION" "TAG" "MSG" "MSF" "CAL" "SPECIAL" "IFTRIG"
	 "PS" "PSFILE" "BANNER" "INCLUDE" "PUSH-OMIT-CONTEXT" "DEBUG" "DUMPVARS"
	 "CLEAR-OMIT-CONTEXT" "POP-OMIT-CONTEXT"  "SET" "ERRMSG" "FSET"
	 "EXIT" "FLUSH" "PRESERVE" "MOON" "COLOR" "COLOUR")
   #'(lambda (a b) (> (length a) (length b)))))

(defconst remind-type-keywords
  (sort
   (list "INT" "STRING" "TIME" "DATE" "SHADE")
     #'(lambda (a b) (> (length a) (length b)))))

(defconst remind-builtin-variables
  (sort
   (list "$CalcUTC" "$CalMode" "$DefaultPrio" "$EndSent" "$EndSentIg" "$NumTrig"
	 "$FirstIndent" "$FoldYear" "$FormWidth" "$MinsFromUTC" "$LatDeg" "$LatMin" "$LatSec"
	 "$Location" "$LongDeg" "$LongMin" "$LongSec" "$MaxSatIter" "$SubsIndent")
   #'(lambda (a b) (> (length a) (length b)))))

(defconst remind-time-words
  (sort
   (list "Jan" "January" "Feb" "Mar" "Apr" "Jun" "Jul" "Aug" "Sept" "Sep" "Oct" "Nov" "Dec" "February" "March" "April" "May" "June" "July" "August" "September" "October" "November" "December" "Mon" "Monday" "Tue" "Tues" "Tuesday" "Wed" "Wednesday" "Thu" "Thursday" "Fri" "Friday" "Saturday" "Sat" "Sun" "Sunday")
   #'(lambda (a b) (> (length a) (length b)))))

(defconst remind-builtin-functions
  (sort
   (list "abs" "access" "shell" "args" "asc" "baseyr" "char" "choose" "coerce" "date"
	 "dawn" "today" "day" "daysinmon" "defined" "dosubst" "dusk" "easterdate" "easter"
	 "filedir" "filename" "getenv" "hour" "iif" "trigger" "index" "isdst" "isleap"
	 "isomitted" "hebdate" "hebday" "hebmon" "hebyear" "language" "ord" "thisyear"
	 "sunrise" "sunset" "lower" "max" "min" "minute" "mon" "moondate" "moontime"
	 "moonphase" "now" "ostype" "plural" "realnow" "realtoday" "sgn" "strlen" "psshade"
	 "substr" "trigdate" "trigger" "trigtime" "trigvalid" "typeof" "upper" "psmoon"
	 "value" "version" "wkday" "wkdaynum" "msgprefix" "msgsuffix" "year")
   #'(lambda (a b) (> (length a) (length b)))))

;;; faces
;;example of setting up special faces for a mode.

(defvar remind-conf-command-face 'remind-conf-command-face
  "Remind commands.")
(defface remind-conf-command-face
  '((t :foreground "SeaGreen4" :bold t))
  "Font Lock mode face used to highlight commands."
  :group 'remind-conf)

(defvar remind-conf-keyword-face 'remind-conf-keyword-face
  "Remind keywords.")
(defface remind-conf-keyword-face
  '((t :foreground "blue violet"))
  "Font Lock mode face used to highlight keywords."
  :group 'remind-conf)

(defvar remind-conf-substitutes-face 'remind-conf-substitutes-face
  "Remind substitutes.")
(defface remind-conf-substitutes-face
  '((t :foreground "blue2"))
  "Font Lock mode face used to highlight substitutes."
  :group 'remind-conf)

(defvar remind-conf-endline-face 'remind-conf-endline-face
  "Remind endline.")
(defface remind-conf-endline-face
  '((t :foreground "goldenrod2" :bold t))
  "Font Lock mode face used to highlight commands."
  :group 'remind-conf)

(defvar remind-conf-variable-face 'remind-conf-variable-face
  "Remind variable.")
(defface remind-conf-variable-face
  '((t :foreground "DeepPink2" :bold t))
  "Font Lock mode face used to highlight commands."
  :group 'remind-conf)

(defvar remind-conf-color-face 'remind-conf-color-face
  "Remind color variables.")
(defface remind-conf-color-face
  '((t :foreground "gold" :bold t))
  "Font Lock mode face used to highlight color changes."
  :group 'remind-conf)

(defvar remind-conf-delta-face 'remind-conf-delta-face
  "Remind deltas.")
(defface remind-conf-delta-face
  '((t :foreground "sandy brown" :bold t))
  "Font Lock mode face used to highlight deltas."
  :group 'remind-conf)

(defvar remind-comment-face 'remind-comment-face
  "Remind comments.")
(defface remind-comment-face
  '((t :foreground "brown"))
  "Font-lock face for highlighting comments."
  :group 'remind-conf)

(defvar remind-string-face 'remind-string-face
  "Remind strings.")
(defface remind-string-face
  '((t :foreground "tomato"))
  "Font lock mode face used to highlight strings."
  :group 'remind-conf)

(defvar remind-time-face 'remind-time-face
  "Remind time words.")
(defface remind-time-face
  '((t :foreground "LightSeaGreen" :bold t))
  "Font lock mode face to highlight time phrases."
  :group 'remind-conf)

(defvar remind-conf-type-face 'remind-conf-type-face
  "Remind type keywords.")
(defface remind-conf-type-face
  '((t :foreground "orange" :bold t))
  "Font lock mode face to highlight type keywords."
  :group 'remind-conf)

(defcustom rem-post-save-function ""
  "Name of shell function that can be run when you save and close a remind file.

If you put a & after the name of the function, it will run asyncronously.  This might
be useful if the process takes a long time."
:type 'string
:group 'remind-conf
)

;; keywords

(defconst remind-conf-font-lock-keywords-1
  (list
   '("^[\;\#]\\s-+.*$" . remind-comment-face)
   (cons (regexp-opt remind-keywords 'words) remind-conf-keyword-face)
   '("%[\"_]" . font-lock-warning-face)
   '("\\(%[a-mops-w]\\)" . remind-conf-substitutes-face)
   '("\"[^\"]*\"" . remind-string-face))
  "Minimal font-locking for `remind-conf-mode'.")

(defconst remind-conf-font-lock-keywords-2
  (append remind-conf-font-lock-keywords-1
	  (list
	   (cons (regexp-opt remind-time-words 'words) remind-time-face)
	   (cons (regexp-opt remind-builtin-functions 'words) remind-conf-command-face)
	   '("%$" . remind-conf-endline-face)))
  "Additional commands to highlight in `remind-conf-mode'.")

(defconst remind-conf-font-lock-keywords-3
  (append remind-conf-font-lock-keywords-2
	  (list
	   (cons (regexp-opt remind-type-keywords 'words) remind-conf-type-face)
	   '("\[[a-zA-Z]\\{3,6\\}\]" . remind-conf-color-face)
	   '("\\s-+\\([12][0-9]\\|3[01]\\|0?[0-9]\\)\\s-+" . remind-conf-substitutes-face);better date regexp
	   '("\\s-+\\(\\(?:20\\|19\\)[0-9][0-9]\\)\\s-+" . remind-conf-substitutes-face);years
	   '("\\s-+\\(2[0-4]\\|[01]?[0-9][.:][0-5][0-9]\\)\\s-+" . remind-conf-substitutes-face);24hour clock, more precise
	   '("\\s-+\\([+-][+-]?[1-9][0-9]*\\)\\s-+" 1 remind-conf-delta-face prepend)
	   (cons (regexp-opt remind-builtin-variables 'words) remind-conf-variable-face)))
  "The ultimate in highlighting experiences for `remind-conf-mode'.")

(defcustom remind-conf-font-lock-keywords 'remind-conf-font-lock-keywords-3
  "Font-lock highlighting level for `remind-conf-mode'."
  :group 'remind-conf
  :type '(choice (const :tag "Barest minimum of highlighting." remind-conf-font-lock-keywords-1)
		 (const :tag "Medium highlighting." remind-conf-font-lock-keywords-2)
		 (const :tag "Highlighting deluxe." remind-conf-font-lock-keywords-3)))

;;; Indentation (I'm sure this could be made more simple. But at least it works.)

(defcustom remind-indent-level 4
  "User definable indentation."
  :group 'remind-conf
  :type '(integer)
  )

(defun remind-indent-line ()
  "Indent current line for remind configuration files."
  (interactive)
  (forward-line 0) ;remember this happens on every line as it is done per line basis
  (if (bobp)
      (indent-line-to 0)
    (let ((not-indented t) cur-indent)
     	(if (looking-at "^[ \t]*\\<\\(ENDIF\\|POP\\(?:-OMIT-CONTEXT\\)?\\)\\>")
	   (progn
	     (save-excursion
	     (forward-line -1) 
	     (setq cur-indent (- (current-indentation) remind-indent-level))) ;note that not-indented is still t
	     (if (< cur-indent 0) (setq cur-indent 0)))
	  (save-excursion
	    (while not-indented
	      (forward-line -1)
	      (if (looking-at "^[ \t]*\\<\\(ENDIF\\|POP\\(?:-OMIT-CONTEXT\\)?\\)\\>")
		  (progn
		    (setq cur-indent 0)
		    (delete-horizontal-space) ;don't know why I need this when other similar indent functions don't.
		    (setq not-indented nil))
		(if (looking-at "\\<\\(IF\\(?:TRIG\\)?\\|PUSH\\(?:-OMIT-CONTEXT\\)?\\)\\>")
		    (progn
		      (setq cur-indent remind-indent-level)
		      (setq not-indented nil))
		  (if (bobp) 
		      (setq not-indented nil))))))
	  (if cur-indent
	      (indent-line-to cur-indent)
	    (indent-line-to 0))))))

;;; Convenience functions

(define-skeleton rem-skel
  "Skeleton to insert a rem line in a remind configuration file."
  nil
  "REM "(skeleton-read "Date? " )
  ("Optional: How many days ahead? " " +" str )
  resume:
  ("Optional: At what time? Format eg 13:00. " " AT " str)
  resume:
  ("Optional: How many minutes ahead? " " +" str )
  resume:
  ("Optional: At what priority? eg 0-9999" " PRIORITY " str )
  resume:
  " MSG %\"" (skeleton-read "Your message? " )"%b%\"%" \n
  )

(define-skeleton rem-today-skel
  "Skeleton to insert a line for today's date."
  nil
  "REM " (format-time-string "%d %b %Y")
  ("Optional: At what time? Format eg 13:20. " " AT " str)
  resume:
  ("Optional: How many minutes ahead? " " +" str )
  resume:
  ("Optional: At what priority? eg 0-9999" " PRIORITY " str )
  resume:
  " MSG " (skeleton-read "Your message? " )"%b.%" \n
  )

(defun rem-today ()
  "Insert the date for today in a remind friendly style."
  (interactive)
  (insert (format-time-string "%e %b %Y")))

(defun rem-tomorrow ()
  "Insert tomorrow's date in a remind friendly style."
  (interactive)
  (insert (format-time-string "%e %b %Y" (time-add (current-time) (days-to-time 1)))))

(defun rem-days-away (arg)
  "Insert a day N number of days in the future.

Takes a prefix argument, but defaults to 4."
  (interactive "nHow many Days?: ")
  (insert (format-time-string "%e %b %Y" (time-add (current-time) (days-to-time arg)))))

(defun rem-week-away ()
  "Insert a day 7 days in the future."
  (interactive)
  (insert (format-time-string "%e %b %Y" (time-add (current-time) (days-to-time 7)))))

(setq skeleton-end-hook nil) ; so the skeletons will not automatically go to a new line.

;;; private function
;; could make it useful for others. Put somethin like the following in your .emacs
;(setq rem-post-save-function "~/bin/dailypic &")

(defun rem-save-file ()
  "Save the file and start the shell function in one go.

This function will close the window after running. It needs the
variable `rem-post-save-function' to be set. It will be most
useful to people who have some sort of function they run to use
remind data ie procucing calendars."
  (interactive)
  (if (boundp 'rem-post-save-function)
      (progn (save-buffer)
	     (shell-command rem-post-save-function)
	     (kill-buffer-and-window))
    (error "`rem-post-save-function' variable is not set")))

(defun rem-setup-colors ()
  "Insert set of variables for coloured output in remind messages."
  (interactive)
  (find-file (expand-file-name "~/.reminders"))
  (goto-char 0) ;we do want it somewhere near the top of the file.
  (save-excursion
    (re-search-forward "\n\n"); squeeze it in where you have a free line.
    (insert "\nSET Esc   CHAR(27)
SET Nrm   Esc + \"[0m\"
SET Blk   Esc + \"[0;30m\"
SET Red   Esc + \"[0;31m\"
SET Grn   Esc + \"[0;32m\"
SET Ylw   Esc + \"[0;33m\"
SET Blu   Esc + \"[0;34m\"
SET Mag   Esc + \"[0;35m\"
SET Cyn   Esc + \"[0;36m\"
SET Wht   Esc + \"[0;37m\"
SET Gry   Esc + \"[30;1m\"
SET BrRed Esc + \"[31;1m\"
SET BrGrn Esc + \"[32;1m\"
SET BrYlw Esc + \"[33;1m\"
SET BrBlu Esc + \"[34;1m\"
SET BrMag Esc + \"[35;1m\"
SET BrCyn Esc + \"[36;1m\"
SET BrWht Esc + \"[37;1m\" \n \n")))

    ;; So now you can do things like:

(define-skeleton birthcol
  "Make birthdays magenta.
Acts on the region or places point where it needs to be."
  nil
  "[Mag]" _ " [Nrm]")

;; finally the derived mode.

;;;###autoload
(define-derived-mode remind-conf-mode text-mode "REM"
  "Major mode for editing remind calendar configuration files.

\\{remind-conf-mode-map}"
  :syntax-table remind-conf-syntax-table
  (set (make-local-variable 'font-lock-keywords-case-fold-search) t) ;this is not working atm 2009-04-13
  (set (make-local-variable 'font-lock-defaults) '(remind-conf-font-lock-keywords))
  (set (make-local-variable 'comment-start) ";")
  (set (make-local-variable 'comment-start) "#")
  (set (make-local-variable 'comment-end) "\n")
  (set (make-local-variable 'fill-column) '100);cause I was having problems with autofill.
  (set (make-local-variable 'indent-line-function) 'remind-indent-line)
  (use-local-map remind-conf-mode-map)
  )


    (provide 'remind-conf-mode)
;;; remind-conf-mode.el ends here

;;; Indentation code
;;; work out how to make the syntax highlighting work only before the (MSG|MSF)
;;; keywords and not after. 
;;; for my own use. keymap to save file and do dailypic C-c C-c in time honoured tradition?