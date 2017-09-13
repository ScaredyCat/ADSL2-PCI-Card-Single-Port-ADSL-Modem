package wld;
use strict;
use vars qw($VERSION @ISA @EXPORT);
$VERSION=1.0;
require Exporter;
@ISA=qw(Exporter);
@EXPORT=qw(wld);
sub min3 {
    my ($x, $y, $z)=@_;
    $y = $x if ($x lt $y);
    $z = $y if ($y lt $z);
    $z;
}

sub wld	{	#/* weighted Levenshtein distance */
  my ($needle, $haystack) = @_;
  my($i, $j);
  my $l1 = length($needle);
  my $l2 = length($haystack);
  my @dw;
  my ($WMAX,$P,$Q,$R);
  $WMAX=$l1>$l2?$l1:$l2;
  $P=1;
  $Q=1;
  $R=1;

  $dw[0][0]=0;
  for ($j=1; $j<=$WMAX; $j++) {
    $dw[0][$j]=$dw[0][$j-1]+$Q;
  }
  for ($i=1; $i<=$WMAX; $i++) {
    $dw[$i][0]=$dw[$i-1][0]+$R;
  }
  for ($i=1; $i<=$l1; $i++) {
    for($j=1; $j<=$l2; $j++) {
      $dw[$i][$j]=&min3($dw[$i-1][$j-1]+((substr($needle,$i-1,1) eq
        substr($haystack,$j-1,1))?0:$P),$dw[$i][$j-1]+$Q,$dw[$i-1][$j]+$R);
    }	
  }
  return($dw[$l1][$l2]);
}

1;
