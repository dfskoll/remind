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
        bless($hash, $class);
        $hash->adjust();
        return $hash;
}

# Base class: Set the color to black
sub adjust
{
        my ($self) = @_;
        $self->{r} = 0;
        $self->{g} = 0;
        $self->{b} = 0;
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
sub adjust
{
        my ($self) = @_;
        if ($self->{body} =~ /^(\d+)\s+(\d+)\s+(\d+)/) {
                $self->{r} = $1;
                $self->{g} = $2;
                $self->{b} = $3;
        }
}

package Remind::PDF::Entry::color;
use base 'Remind::PDF::Entry';

# Strip the RGB prefix from body
sub adjust
{
        my ($self) = @_;
        $self->{body} =~ s/^\d+\s+\d+\s+\d+\s+//;
}

package Remind::PDF::Entry::postscript;
use base 'Remind::PDF::Entry';
package Remind::PDF::Entry::psfile;
use base 'Remind::PDF::Entry';
package Remind::PDF::Entry::UNKNOWN;
use base 'Remind::PDF::Entry';

1;

