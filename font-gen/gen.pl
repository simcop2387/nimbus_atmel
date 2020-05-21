#!/usr/bin/env perl

use strict;
use warnings;
use v5.28;

use Font::PCF;
use Data::Dumper;
use Data::Dumper::Compact 'ddc';
use GD;

my $custom_chars = {
  0 => [ # Blank base character, save for later
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000, # screen is only 7 high, but we'll use the 8th bit to prevent code from trimming otherwise blank columns
  ],
  1 => [ # Smiley 
    0b00000000,
    0b01101100,
    0b01101100,
    0b00000000,
    0b00000000,
    0b10000010,
    0b01111100,
    0b00000000,
  ],
  2 => [ # Frowny
    0b00000000,
    0b01101100,
    0b01101100,
    0b00000000,
    0b00000000,
    0b01111100,
    0b10000010,
    0b00000000,
  ],
  3 => [ # Winky
    0b00000000,
    0b01001100,
    0b00001100,
    0b00000000,
    0b00000000,
    0b10000010,
    0b01111100,
    0b00000000,
  ],
  4 => [ # Excited
    0b00000000,
    0b01101100,
    0b01101100,
    0b00000000,
    0b11111110,
    0b10000010,
    0b01111100,
    0b00000000,
  ],
  5 => [ # ? not sure yet TODO
    0b00000000,
    0b01001100,
    0b00001100,
    0b00000000,
    0b00000000,
    0b10000010,
    0b01111100,
    0b00000000,
  ],
  6 => [ # Mug
    0b10001000,
    0b10001110,
    0b10001001,
    0b10001001,
    0b10001001,
    0b10001110,
    0b11111000,
    0b00000000,
  ],
  7 => [ # Music note
    0b00011100,
    0b00010010,
    0b00010000,
    0b00010000,
    0b00010000,
    0b01100000,
    0b01100000,
    0b00000000,
  ],
  8 => [ # Poop
    0b01000100,
    0b00010000,
    0b00111000,
    0b01000100,
    0b01111100,
    0b10000010,
    0b11111110,
    0b00000000,
  ],
  9 => [ # Heart
    0b01101100,
    0b10010010,
    0b10010010,
    0b10000010,
    0b01000100,
    0b00101000,
    0b00010000,
    0b00000000,
  ],
  10 => [ # Spade
    0b00010000,
    0b01111100,
    0b11111110,
    0b11111110,
    0b10111010,
    0b00010000,
    0b11111110,
    0b00000000,
  ],
  11 => [ # Club, TODO make better
    0b01111100,
    0b10111010,
    0b11111110,
    0b10111010,
    0b00111000,
    0b00010000,
    0b00111000,
    0b00000000,
  ],
  12 => [ # Diamond
    0b00010000,
    0b00101000,
    0b01000100,
    0b10000010,
    0b01000100,
    0b00101000,
    0b00010000,
    0b00000000,
  ],
  13 => [ # Table  
    0b00000000,
    0b00000000,
    0b00000000,
    0b11111110,
    0b01000100,
    0b01000100,
    0b01000100,
    0b00000000,
  ],
  14 => [ # Warning
    0b01100000,
    0b11110000,
    0b11110000,
    0b01100000,
    0b01100000,
    0b00000000,
    0b01100000,
    0b00000000, # force keep this blank column
  ],
  15 => [ # Error
    0b00010000,
    0b00111000,
    0b00111000,
    0b01111100,
    0b01111100,
    0b11111110,
    0b11111110,
    0b00000000, # force keep this blank column
  ],
  16 => [ # 
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000, # force keep this blank column
  ],

  # The following are because the characters are too high, or in the wrong spots
  ord("j") => [
    0b00100000,
    0b00000000,
    0b00100000,
    0b00100000,
    0b00100000,
    0b00100000,
    0b01000000,
    0b00000000, # force keep this blank column
  ],
  ord("q") => [
    0b00000000,
    0b00000000,
    0b01100000,
    0b10010000,
    0b10010000,
    0b01110000,
    0b00010000,
    0b00000000, # force keep this blank column
  ],
  ord("p") => [
    0b00000000,
    0b00000000,
    0b01100000,
    0b10010000,
    0b10010000,
    0b11100000,
    0b10000000,
    0b00000000, # force keep this blank column
  ],
  ord("g") => [
    0b00000000,
    0b01100000,
    0b10010000,
    0b10010000,
    0b01110000,
    0b00010000,
    0b11100000,
    0b00000000, # force keep this blank column
  ],
  ord("y") => [
    0b00000000,
    0b00000000,
    0b10010000,
    0b10010000,
    0b01110000,
    0b00010000,
    0b11100000,
    0b00000000, # force keep this blank column
  ],
  ord("Q") => [
    0b01100000,
    0b10010000,
    0b10010000,
    0b10010000,
    0b10010000,
    0b10110000,
    0b01101000,
    0b00000000, # force keep this blank column
  ],
  ord("V") => [
    0b10001000,
    0b10001000,
    0b10001000,
    0b01010000,
    0b01010000,
    0b00100000,
    0b00100000,
    0b00000000, # force keep this blank column
  ],
  ord("W") => [
    0b10000010,
    0b10000010,
    0b10010010,
    0b10010010,
    0b10010010,
    0b10010010,
    0b01101100,
    0b00000000, # force keep this blank column
  ],
  ord(",") => [
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00100000,
    0b00100000,
    0b01000000,
    0b00000000, # force keep this blank column
  ],
  ord(".") => [
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b01100000,
    0b01100000,
    0b00000000, # force keep this blank column
  ],
  ord(":") => [
    0b00000000,
    0b00000000,
    0b01000000,
    0b01000000,
    0b00000000,
    0b01000000,
    0b01000000,
    0b00000000, # force keep this blank column
  ],
  ord(";") => [
    0b00000000,
    0b00000000,
    0b01000000,
    0b01000000,
    0b00000000,
    0b01000000,
    0b10000000,
    0b00000000, # force keep this blank column
  ],
  ord("_") => [
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b11111000,
    0b00000000, # force keep this blank column
  ],




  # Formatting meta-chars
  31 => [ # Clear formatting
    0b01010000,
    0b10010000,
    0b01011000,
    0b00000000,
    0b11000000,
    0b10000000,
    0b10000000,
    0b00100000, # force keep this blank column
  ],
  30 => [ # Blink
    0b10010000,
    0b11010000,
    0b11011000,
    0b00000000,
    0b10100000,
    0b11000000,
    0b10100000,
    0b00000000, # force keep this blank column
  ],
  29 => [ # Inverse
    0b11100000,
    0b01001100,
    0b11101010,
    0b00000000,
    0b10100000,
    0b01000000,
    0b00000000,
    0b00010000, # force keep this blank column
  ],
  28 => [ # Inverse-Blink, invert instead of disappearing when blinking
    0b11100000,
    0b01001100,
    0b11101010,
    0b00000000,
    0b10101000,
    0b01001100,
    0b00001100,
    0b00010000, # force keep this blank column
  ],
};

{
  package MyGlyphs;
  use Data::Dumper;

  sub bitmap {
    print Dumper($_[0], $custom_chars->{${$_[0]}});
    return [map {$_ << 24} $custom_chars->{$_[0]->$*}->@*];
  }

  sub width {
    return 8
  }

  sub name {
    return "CUSTOM";
  }
}

sub printbits {
   my ( $bits ) = @_;
   while( $bits ) {
      print +( $bits & (1<<31) ) ? '#' : ' ';
      $bits <<= 1;
   }
   print "\n";
}

sub transpose_glyph {
  my $g = shift;

  my @raw_bits = map {[split('', substr(sprintf("%032b", $_), 0, $g->width))]} $g->bitmap->@*;
  my @new_bits;

  for my $row (0..$#raw_bits) {
    for my $col (0..$g->width-1) {
      $new_bits[$col][$row] = $raw_bits[$row][$col];
    }
  }

  # they're not cis bits
  my @trans_bits = grep {$_ ne "0b00000000"} map {"0b".join '', $_->@*} @new_bits;

  push @trans_bits, "0b00000000"; # Always ensure we have at least one column, and that it's blank at the end.  This adds kerning and prevents issues with some characters that are blank
  push @trans_bits, "0b00000000" if $g->name eq ' '; # make sure space is at least two pixels wide.
  unshift @trans_bits, "0b00000000" if $g->name eq '.'; # center the . a bit to balance things

#  print Dumper(\@trans_bits);
  return {width => scalar @trans_bits, columns => \@trans_bits}; 
}

my @bytes;
my @widths;

my $font = Font::PCF->open( "/usr/share/fonts/X11/misc/clR5x8.pcf.gz" );

for my $char (0..127) {
  my $g;

  if (exists($custom_chars->{$char})) {
    $g = bless \$char, "MyGlyphs";
  } else {
   $g = $font->get_glyph_for_char( chr $char );
  }
  my $glyph = transpose_glyph $g;

  push @bytes, $glyph->{columns}->@*;
  push @widths, $glyph->{width};
}

my $template = <<"EOT";

uint8_t font_glyphs[] = {
    %B%
};
uint8_t font_starts[] = {%S%};
uint8_t font_widths[] = {%W%};

EOT

my $B = join "", (join ",\n    ", @bytes);
my $total = 0;
my $S = join ', ', map {$total += $_; sprintf "% 4d", $total} @widths;
my $W = join ', ', @widths;
print $template =~ s/%B%/$B/r =~ s/%W%/$W/r =~ s/%S%/$S/r;

my $gd = GD::Image->new(scalar @bytes, 8);
my $white = $gd->colorAllocate(255,255,255);
my $black = $gd->colorAllocate(0,0,0);

for my $x (0..$#bytes) {
  my $col = $bytes[$x];
  for my $y (0..7) {
    my $bit = oct($col) & (2**$y) ? 1 : 0;
    
    if ($bit) {
      $gd->setPixel($x, 7-$y, $black)
    }
  }
}

open(my $imfh, ">image.png");
print $imfh $gd->png;
close($imfh);
