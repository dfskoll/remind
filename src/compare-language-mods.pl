#!/usr/bin/perl
use strict;
use warnings;

my $language_map = {
        en => 'ENGLISH',
        de => 'GERMAN',
        nl => 'DUTCH',
        fi => 'FINNISH',
        fr => 'FRENCH',
        'no' => 'NORWEGIAN',
        da => 'DANISH',
        pl => 'POLISH',
        pt => 'BRAZPORT',
        it => 'ITALIAN',
        ro => 'ROMANIAN',
        es => 'SPANISH',
};

if (!$ARGV[0]) {
        print STDERR "Usage: $0 lang_code\n";
        exit(1);
}

my $lang = $ARGV[0];
if (!exists($language_map->{$lang})) {
        print STDERR "$lang is not a valid language.\n";
        exit(1);
}

my $flag = $language_map->{$lang};
print STDERR "Testing for: $lang - $flag.\n";
my_sys("make clean") && die("make clean failed");
my_sys("make -j6 all LANGDEF=-DLANG=$flag") && die("make all failed");
my_sys("./remind -q -r ../tests/tstlang.rem > test-$lang-compiled.out 2>&1");

my_sys("make clean") && die("make clean failed");
my_sys("make -j6 all") && die("make all failed");
my_sys("./remind -q -r -ii=\\\"../include/lang/$lang.rem\\\" ../tests/tstlang.rem > test-$lang-runtime.out 2>&1");

exit(0);

sub my_sys
{
        print STDERR "Running: " . join(' ', @_) . "\n";
        return system(@_);
}
