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
                if ($special =~ /^(html|htmlclass|week|moon|shade|color|colour|postscript|psfile|pango)$/) {
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
        my ($self, $pdf, $cr, $settings, $so_far, $day, $col, $height) = @_;

        my ($x1, $y1, $x2, $y2) = $pdf->col_box_coordinates($so_far, $col, $height, $settings);
        my $layout = Pango::Cairo::create_layout($cr);

        $layout->set_width(1024 * ($x2 - $x1 - 2 * $settings->{border_size}));
        $layout->set_wrap('word-char');
        $layout->set_text($self->{body});
        my $desc = Pango::FontDescription->from_string($settings->{entry_font} . ' ' . $settings->{entry_size} . 'px');
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

sub render
{
        my ($self, $pdf, $cr, $settings, $so_far, $day, $col, $height) = @_;
        # Do nothing in pre-render mode
        return 0 unless $height;

        # Render in small text at bottom-right
        my ($x1, $y1, $x2, $y2) = $pdf->col_box_coordinates($so_far, $col, $height, $settings);
        my $layout = Pango::Cairo::create_layout($cr);

        $layout->set_text($self->{body});
        my $desc = Pango::FontDescription->from_string($settings->{entry_font} . ' ' . int(0.75 * $settings->{entry_size}) . 'px');
        $layout->set_font_description($desc);
        my ($wid, $h) = $layout->get_pixel_size();

        $cr->save();
        $cr->move_to($x2 - $settings->{border_size}/4 - $wid, $y2 - $settings->{border_size}/4 - $h);
        Pango::Cairo::show_layout($cr, $layout);
        $cr->restore();

        return 0;
}

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
        my ($self, $pdf, $cr, $settings, $so_far, $day, $col, $height) = @_;

        # Do nothing in pre-render mode
        return 0 unless $height;

        my ($x1, $y1, $x2, $y2) = $pdf->col_box_coordinates($so_far, $col, $height, $settings);

        my $layout;
        my $bodywidth = 0;
        if ($self->{fontsize} <= 0) {
                $self->{fontsize} = $settings->{entry_size};
        }
        if ($self->{size} <= 0) {
                $self->{size} = $settings->{daynum_size};
        }

        if ($self->{phase} !~ /^[0123]$/) {
                # Invalid phase
                return 0;
        }

        if ($self->{body} ne '') {
                $layout = Pango::Cairo::create_layout($cr);
                $layout->set_text($self->{body});
                my $desc = Pango::FontDescription->from_string($settings->{entry_font} . ' ' . $self->{fontsize} . 'px');
                $layout->set_font_description($desc);
                ($bodywidth, undef) = $layout->get_pixel_size();
        }
        my ($xc, $yc);
        if ($settings->{numbers_on_left}) {
                $yc = $so_far + $settings->{border_size} + ($self->{size} / 2);
                $xc = $x2 - $settings->{border_size} - ($self->{size} / 2);
                if ($bodywidth) {
                        $xc -= ($bodywidth + $settings->{border_size});
                }
        } else {
                $xc = $x1 + $settings->{border_size} + ($self->{size} / 2);
                $yc = $so_far + $settings->{border_size} + ($self->{size} / 2);
        }
        $self->draw_moon($xc, $yc, $cr);
        if ($layout) {
                $cr->save();
                $cr->move_to ($xc + ($self->{size}/2) + $settings->{border_size},
                              $yc + ($self->{size}/2) - $self->{fontsize} );
                Pango::Cairo::show_layout($cr, $layout);
                $cr->restore();
        }
}

sub draw_moon
{
        my ($self, $xc, $yc, $cr) = @_;
        $cr->save();
        $cr->new_path();
        $cr->arc($xc, $yc, $self->{size}/2, 0, 2*3.1415926535);
        if ($self->{phase} == 0) {
                $cr->stroke_preserve();
                $cr->fill();
        } elsif ($self->{phase} == 1) {
                $cr->stroke();
                $cr->arc($xc, $yc, $self->{size}/2, 3.1415926535/2, 3 * 3.1415926535/2);
                $cr->stroke_preserve();
                $cr->fill();
        } elsif ($self->{phase} == 2) {
                $cr->stroke();
        } elsif ($self->{phase} == 3) {
                $cr->stroke();
                $cr->arc($xc, $yc, $self->{size}/2, 3 * 3.1415926535/2, 3.1415926535/2);
                $cr->stroke_preserve();
                $cr->fill();
        }
        $cr->restore();
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
package Remind::PDF::Entry::pango;
use base 'Remind::PDF::Entry';
sub adjust
{
        my ($self) = @_;
        if ($self->{body} =~ /^@([-0-9.]+),\s*([-0-9.]+)\s*(.*)/) {
                $self->{atx} = $1;
                $self->{aty} = $2;
                $self->{body} = $3;
        }
}

sub render
{
        my ($self, $pdf, $cr, $settings, $so_far, $day, $col, $height) = @_;

        my ($x1, $y1, $x2, $y2) = $pdf->col_box_coordinates($so_far, $col, $height, $settings);
        my $layout = Pango::Cairo::create_layout($cr);

        $layout->set_width(1024 * ($x2 - $x1 - 2 * $settings->{border_size}));
        $layout->set_wrap('word-char');
        $layout->set_markup($self->{body});

        if (($layout->get_text() // '') eq '') {
                # Invalid markup
                return 0;
        }
        my $desc = Pango::FontDescription->from_string($settings->{entry_font} . ' ' . $settings->{entry_size} . 'px');
        $layout->set_font_description($desc);
        my ($wid, $h) = $layout->get_pixel_size();

        if ($height) {
                $cr->save();
                if (defined($self->{atx}) && defined($self->{aty})) {
                        my ($x, $y);
                        if ($self->{atx} < 0) {
                                $x = $x2 + $self->{atx} - $wid;
                        } else {
                                $x = $x1 + $self->{atx};
                        }
                        if ($self->{aty} < 0) {
                                $y = $y2 + $self->{aty} - $h;
                        } else {
                                $y = $y1 + $self->{aty};
                        }
                        $cr->move_to($x, $y);
                } else {
                        $cr->move_to($x1 + $settings->{border_size}, $so_far);
                }
                Pango::Cairo::show_layout($cr, $layout);
                $cr->restore();
        }
        return $h;
}

package Remind::PDF::Entry::UNKNOWN;
use base 'Remind::PDF::Entry';

1;

