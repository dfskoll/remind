/* ---------------------------------------------------------------------------
   $Id: test-rem.rexx,v 1.1 1998-01-15 02:50:50 dfs Exp $
   TEST-REM
   This file runs an AmigaDOS acceptance test for Remind.   To use it, type:
	rx test-rem
   in the build directory.
   (Use this with the ENGLISH version only !!!)
   ---------------------------------------------------------------------------
*/

address 'COMMAND'
options results

'setenv TEST_GETENV "foo bar baz"'
'remind -e -dxte test.rem 16 feb 1991 >ram:test.out'
'diff ram:test.out test-rem.ami >NIL:'
if rc=0 then do
  say "Remind:  Acceptance test PASSED"
end
else do
  say "Remind:  Acceptance test FAILED"
end
'delete ram:test.out quiet'
