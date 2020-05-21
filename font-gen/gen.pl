#!/usr/bin/env perl

use strict;
use warnings;
use v5.28;

use Font::PCF;
use Data::Dumper;
use Data::Dumper::Compact 'ddc';


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

for my $char (32..127) {
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
my $S = join ', ', map {$total += $_; sprintf "0x%03X", $total} @widths;
my $W = join ', ', @widths;
print $template =~ s/%B%/$B/r =~ s/%W%/$W/r =~ s/%S%/$S/r;
