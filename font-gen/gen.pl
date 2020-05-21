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
  print ddc(\@raw_bits);

}

my $glyph={};

my $font = Font::PCF->open( "/usr/share/fonts/X11/misc/clR5x8.pcf.gz" );

for my $char (ord" "..ord"~") {
  my $g = $font->get_glyph_for_char( chr $char );
  $glyph->{$char} = transpose_glyph $g; # TODO transpose, etc.
#  printf "%s: %d [%d, %d] x [%d, %d]\n", chr($char), $g->width, $g->left_side_bearing, $g->right_side_bearing, $g->ascent, $g->descent;

#  printbits $_ for $glyph->bitmap->@*;
}
