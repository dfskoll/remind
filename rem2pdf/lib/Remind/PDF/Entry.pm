package Remind::PDF::Entry;
use strict;
use warnings;

use Cairo;
use Pango;
use Encode;

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
        bless $hash, $class;
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

sub render
{
        my ($self, $month, $cr, $settings, $so_far, $day, $col, $height) = @_;

        my ($x1, $y1, $x2, $y2) = $month->col_box_coordinates($so_far, $col, $height, $settings);
        my $layout = Pango::Cairo::create_layout($cr);

        $layout->set_width(1024 * ($x2 - $x1 - 2 * $settings->{border_size}));
        $layout->set_wrap('word-char');
        $layout->set_text(Encode::decode('UTF-8', $self->{body}));
        my $desc = Pango::FontDescription->from_string($settings->{entry_font} . ' ' . $settings->{entry_size});
        $layout->set_font_description($desc);
        my ($wid, $h) = $layout->get_pixel_size();

        if ($height) {
                $cr->save();
                $cr->set_source_rgb($self->{r} / 255,
                                    $self->{g} / 255,
                                    $self->{b} / 255);
                $cr->move_to($x1 + $settings->{border_size}, $so_far);
                Pango::Cairo::show_layout($cr, $layout);
                $cr->restore();
        }
        return $h;
}

package Remind::PDF::Entry::html;
use base 'Remind::PDF::Entry';
package Remind::PDF::Entry::htmlclass;
use base 'Remind::PDF::Entry';
package Remind::PDF::Entry::week;
use base 'Remind::PDF::Entry';
package Remind::PDF::Entry::moon;
use base 'Remind::PDF::Entry';

sub adjust {
        my ($self) = @_;
        my ($phase, $size, $fontsize, $msg) = split(/\s+/, $self->{body}, 4);
        $phase = '' unless defined($phase);
        $size = -1 unless defined($size);
        $fontsize = -1 unless defined($fontsize);
        $msg = '' unless defined($msg);
        $self->{phase} = $phase;
        $self->{size} = $size;
        $self->{fontsize} = $fontsize;
        $self->{body} = $msg;
}
sub render
{
        my ($self) = @_;
        return 0;
}
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

