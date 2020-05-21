#!/usr/bin/env perl

use strict;
use warnings;
use v5.28;

use Font::PCF;
use Data::Dumper;
use Data::Dumper::Compact 'ddc';


my $custom_chars = {
  0 => [ # Blank base character, save for later
    0b00000000,
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
    0b00000000,
  ],
  6 => [ # Mug
    0b10001000,
    0b10001110,
    0b10001001,
    0b10001001,
    0b10001001,
    0b10001110,
    0b10001000,
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
    0b00000000,
  ],
  8 => [ # Poop
    0b10010010,
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
    0b00000000,
    0111111110,
    0b01000100,
    0b01000100,
    0b01000100,
    0b00000000,
  ],
  14 => [ # Warning
    0b01100100,
    0b11110100,
    0b11110100,
    0b01100100,
    0001100100,
    0b00000100,
    0b01100100,
    0b01100100,
    0b00001000, # force keep this blank column
  ],
  15 => [ # Error
    0b00010000,
    0b00111000,
    0b00111000,
    0b01111100,
    0001111100,
    0b11111110,
    0b11111110,
    0b11111110,
    0b00000000, # force keep this blank column
  ],
  16 => [ # 
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0000000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000, # force keep this blank column
  ],
};

{
  package MyGlyphs;

  sub bitmap {
    return $custom_chars->{$$_[0]};
  }

  sub width {
    return 8
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
  my $g = $font->get_glyph_for_char( chr $char );
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
