@echo off
rem ---------------------------------------------------------------------------
rem TEST-REM
rem
rem $Id: test-rem.cmd,v 1.3 2000-02-18 03:46:32 dfs Exp $
rem
rem This file runs an OS/2 acceptance test for Remind.	To use it, type:
rem	 test-rem
rem in the build directory.
rem
rem This file is part of REMIND.
rem Copyright (C) 1992-1997 David F. Skoll        
rem Copyright (C) 1999-2000 Roaring Penguin Software Inc.
rem ---------------------------------------------------------------------------

del /f test.out > nul
setlocal
set TEST_GETENV=foo bar baz
if exist ..\os2-ex\remind.exe goto bcc
remind -e -dxte ./test.rem 16 feb 1991 > .\test.out
goto cmp
:bcc
..\os2-ex\remind -e -dxte .\test.rem 16 feb 1991 > .\test.out
:cmp
echo n | comp test.out test2.cmp
if errorlevel 1 goto oops
echo "Remind:  Acceptance test PASSED"
goto quit
:oops
echo "Remind:  Acceptance test FAILED"
echo ""
echo "Examine the file test.out to see where it differs from the"
echo "reference file test2.cmp."
:quit
endlocal
