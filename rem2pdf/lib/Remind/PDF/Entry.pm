package Remind::PDF::Entry;
use strict;
use warnings;

sub new_from_hash
{
        my ($class, $hash) = @_;
        if (exists($hash->{passthru})) {
                my $special = lc($hash->{passthru});
                if ($special =~ /^(html|htmlclass|week|moon|shade|color|colour|postscript|psfile)$/) {
                        $special = 'color' if $special eq 'colour';
                        $class = 'Remind::PDF::Entry::' . $special;
                } else {
                        $class = 'Remind::PDF::Entry::UNKNOWN';
                }
        }
        return bless($hash, $class);
}

package Remind::PDF::Entry::html;
use base 'Remind::PDF::Entry';
package Remind::PDF::Entry::htmlclass;
use base 'Remind::PDF::Entry';
package Remind::PDF::Entry::week;
use base 'Remind::PDF::Entry';
package Remind::PDF::Entry::moon;
use base 'Remind::PDF::Entry';
package Remind::PDF::Entry::shade;
use base 'Remind::PDF::Entry';
package Remind::PDF::Entry::color;
use base 'Remind::PDF::Entry';
package Remind::PDF::Entry::postscript;
use base 'Remind::PDF::Entry';
package Remind::PDF::Entry::psfile;
use base 'Remind::PDF::Entry';
package Remind::PDF::Entry::UNKNOWN;
use base 'Remind::PDF::Entry';

1;

