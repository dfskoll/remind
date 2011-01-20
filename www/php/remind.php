<?php

function parse_remind_output ($fp) {
    while(1) {
	$line = fgets($fp);
	if ($line === false) break;
	$line = trim($line);
	if ($line == '# rem2ps begin') break;
    }
    if ($line === false) {
	return array('success' => 0,
		     'error' => 'Could not find any Rem2PS data');
    }

    $line = fgets($fp);
    if ($line === false) {
	return array('success' => 0,
		     'error' => 'Unexpected end-of-file');
    }

    $line = trim($line);
    list($month, $year, $days_in_mon, $first_day, $monday_flag) = explode(' ', $line);
    $retval = array('month' => $month,
		    'year'  => $year,
		    'days_in_mon' => $days_in_mon,
		    'first_day' => $first_day,
		    'monday_flag' => $monday_flag);

    $line = fgets($fp);
    if ($line === false) {
	return array('success' => 0,
		     'error' => 'Unexpected end-of-file');
    }

    $line = trim($line);
    $retval['day_names'] = explode(' ', $line);

    $line = fgets($fp);
    if ($line === false) {
	return array('success' => 0,
		     'error' => 'Unexpected end-of-file');
    }
    $line = trim($line);

    list($m, $n) = explode(' ', $line);
    $retval['prev'] = array('month' => $m, 'days' => $n);

    $line = fgets($fp);
    if ($line === false) {
	return array('success' => 0,
		     'error' => 'Unexpected end-of-file');
    }
    $line = trim($line);

    list($m, $n) = explode(' ', $line);
    $retval['next'] = array('month' => $m, 'days' => $n);

    $line_info = 0;

    $entries = array();
    while (1) {
	$line = fgets($fp);
	if ($line === false) break;
	$line = trim($line);
	if ($line == '# rem2ps end') break;
	if (strpos($line, '# fileinfo ') === 0) {
	    list($lno, $fname) = explode(' ', substr($line, 12), 2);
	    $lineinfo = array('file' => $fname, 'line' => $lno);
	    continue;
	}
	list($date, $special, $tags, $duration, $time, $body) = explode(' ', $line, 6);
	list($y, $m, $d) = explode('/', $date);
	$d = preg_replace('/^0(.)/', '$1', $d);
	$m = preg_replace('/^0(.)/', '$1', $m);
	$entry = array('day' => $d,
		       'month' => $m,
		       'year' => $y,
		       'special' => $special,
		       'tags' => $tags,
		       'duration' => $duration,
		       'time' => $time,
		       'body' => $body);
	if (is_array($lineinfo)) {
	    $entry['line'] = $lineinfo['line'];
	    $entry['file'] = $lineinfo['file'];
	    $lineinfo = 0;
	}
	if (!is_array($entries[$d])) {
	    $entries[$d] = array();
	}
	$entries[$d][] = $entry;
    }
    $retval['entries'] = $entries;
    return array('success' => 1, 'results' => $retval);
  }

$fp = popen('rem -p -l', 'r');
$ans = parse_remind_output($fp);
pclose($fp);
print_r($ans);

?>