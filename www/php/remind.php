<?php

class Remind
{
    function get_el(&$array, $i)
    {
	if (!array_key_exists($i, $array)) return null;
	return $array[$i];
    }

    function get_elem($array, $indexes)
    {
	foreach ($indexes as $i) {
	    if (!is_array($array)) return null;
	    if (!array_key_exists($i, $array)) return null;
	    $array = $array[$i];
	}
	return $array;
    }

    function munge_entry($day, &$results, &$specials, &$options, $str, &$e) {
	return htmlspecialchars($str);
    }

    function format_entry($day, &$results, &$specials, &$options, &$e) {
	$special = $this->get_el($e, 'special');
	$body = $this->get_el($e, 'body');

	if ($body === null) $body = '';
	if ($special === null || $special == '*') {
	    return $this->munge_entry($day, $results, $specials, $options, $body, $e);
	}
	if ($special == 'COLOR') {
	    if (preg_match('/^(\d+)\s+(\d+)\s+(\d+)\s+(.*)/', $body, $matches)) {
		return sprintf('<span style="color: #%02x%02x%02x">%s</span>',
			       $matches[1] % 255,
			       $matches[2] % 255,
			       $matches[3] % 255,
			       $this->munge_entry($day, $results, $specials, $options, $matches[4], $e));
	    }
	    return 'Bad COLOR spec: ' . htmlspecialchars($body);
	}

	# HTML is passed through un-munged.
	if ($special == 'HTML') return $body;

	# Ignore unknown specials
	return '';
    }

    function format_entries($day, &$results, &$specials, &$options, &$entries) {
	$html = '';
	foreach ($entries as $e) {
	    $html .= '<div class="rem-entry">' . $this->format_entry($day, $results, $specials, $options, $e) . '</div>';
	}
	return $html;
    }

    function do_one_day($day, &$results, &$specials, &$options) {
	$class = $this->get_elem($specials, array('HTMLCLASS', $day, 0, 'body'));
	$shade = $this->get_elem($specials, array('SHADE', $day, 0, 'body'));
	if ($class === null) $class = 'rem-cell';
	$bg = '';
	if ($shade !== null) {
	    if (preg_match('/(\d+)\s+(\d+)\s+(\d+)/', $shade, $matches)) {
		if ($matches[1] <= 255 && $matches[2] <= 255 && $matches[3] <= 255) {
		    $bg = sprintf(' style="background: #%02x%02x%02x"',
				  $matches[1], $matches[2], $matches[3]);
		}
	    }
	}
	$html = "<td class=\"$class\"$bg>";

	$week = $this->get_elem($specials, array('WEEK', $day, 0, 'body'));
	if ($week === null) {
	    $week = '';
	} else {
	    $week = ' ' . $week;
	}

	# Day number
	$html .= '<div class="rem-daynumber">' . $day . $week . '</div>';

	# And the entries
	$entries = $this->get_elem($results, array('entries', $day));
	if (is_array($entries) && count($entries) > 0) {
	    $html .= '<div class="rem-entries">';
	    $html .= $this->format_entries($day, $results, $specials, $options, $entries);
	    $html .= '</div>';
	}
	$html .= "</td>";
	return $html;
    }

    function generate_html(&$results, &$specials, &$options)
    {
	$monday_first = $results['monday_flag'];
	$first_col = $results['first_day'];
	if ($monday_first) {
	    $first_col--;
	    if ($first_col < 0) $first_col = 6;
	}

	$last_col = ($first_col + $results['days_in_mon'] -1) % 7;

	$html = '<table class="rem-cal"><caption class="rem-cal-caption">' .
	    htmlspecialchars($results['month']) . ' ' . htmlspecialchars($results['year']) .
	    "</caption>\n";

	$html .= '<tr class="rem-cal-hdr-row">';
	if (!$monday_first) $html    .= '<th class="rem-cal-hdr">' . htmlspecialchars($results['day_names'][0])  . '</th>';
	for ($i=1; $i<7; $i++) $html .= '<th class="rem-cal-hdr">' . htmlspecialchars($results['day_names'][$i]) . '</th>';
	if ($monday_first) $html     .= '<th class="rem-cal-hdr">' . htmlspecialchars($results['day_names'][0])  . '</th>';
	$html .= "</tr>\n";

	# Do the leading empty columns
	for ($col=0; $col < $first_col; $col++) {
	    if ($col == 0) $html .= '<tr class="rem-cal-body-row">';
	    $html .= '<td class="rem-empty">&nbsp;</td>';
	}

	for ($day=1; $day <= $results['days_in_mon']; $day++) {
	    if ($col == 0) $html .= '<tr class="rem-cal-body-row">';
	    $col++;
	    $html .= $this->do_one_day($day, $results, $specials, $options);
	    if ($col == 7) {
		$html .= "</tr>\n";
		$col = 0;
	    }
	}
	if ($col) {
	    while ($col++ < 7) {
		$html .= '<td class="rem-empty">&nbsp;</td>';
	    }
	}
	$html .= "</tr>\n";

	$html .= "</table>\n";
	return $html;
    }
    function parse_remind_output ($fp)
    {
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
	$specials = array();
	while (1) {
	    $line = fgets($fp);
	    if ($line === false) break;
	    $line = trim($line);
	    if ($line == '# rem2ps end') break;
	    if (strpos($line, '# fileinfo ') === 0) {
		list($lno, $fname) = explode(' ', substr($line, 11), 2);
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
	    if ($special != '*' && $special != 'COLOR' && $special != 'HTML') {
		if (!array_key_exists($special, $specials)) {
		    $specials[$special] = array();
		}
		if (!array_key_exists($d, $specials[$special])) {
		    $specials[$special][$d] = array();
		}
		$specials[$special][$d][] = $entry;
	    } else {
		if (!array_key_exists($d, $entries)) {
		    $entries[$d] = array();
		}
		$entries[$d][] = $entry;
	    }

	}
	$retval['entries'] = $entries;
	return array('success' => 1, 'results' => $retval, 'specials' => $specials);
    }
}

$fp = popen('rem -p -l', 'r');
$r = new Remind;
$ans = $r->parse_remind_output($fp);
pclose($fp);
#print_r($ans);
$options = array();
print $r->generate_html($ans['results'], $ans['specials'], $options);

?>
