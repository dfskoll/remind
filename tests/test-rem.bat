@echo off
rem ---------------------------------------------------------------------------
rem TEST-REM
rem
rem $Id: test-rem.bat,v 1.1 1998-01-15 02:50:50 dfs Exp $
rem
rem This file runs an MSDOS acceptance test for Remind.  To use it, type:
rem	 test-rem
rem in the build directory.
rem
rem This file is part of REMIND.
rem Copyright (C) 1992-1997 by David F. Skoll        
rem ---------------------------------------------------------------------------

del test.out > nul
set TEST_GETENV=foo bar baz
if exist ..\msdos-ex\remind.exe goto bcc
remind -e -dxte ./test.rem 16 feb 1991 > test.out
goto cmp
:bcc
..\msdos-ex\remind -e -dxte .\test.rem 16 feb 1991 > test.out
:cmp
echo n | comp test.out test1.cmp
if errorlevel 1 goto oops
echo "Remind:  Acceptance test PASSED"
goto quit
:oops
echo "Remind:  Acceptance test FAILED"
echo ""
echo "Examine the file test.out to see where it differs from the"
echo "reference file test1.cmp."
:quit
