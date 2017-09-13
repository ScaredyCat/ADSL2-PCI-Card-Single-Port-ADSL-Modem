package CDB_File_Dump;

=head1 NAME

CDB_File_Dump - Write cdb dump files

=head1 DESCRIPTION

This is a AnyDBM_File implementation to write cdb dump files. As cdb can
only be written in one step, this produces (and appends) dump files, which
can be fed into cdbmake to produce the final database.

=head1 SYNOPSIS

  use AnyDBM_File;

  BEGIN {
    @AnyDBM_File::ISA = qw( CDB_File_Dump );
  }

  my(%db)
  tie(%db, 'AnyDBM_File','dump_file', O_RDWR ,0644); # write
  $db{'key'} = 'value';
  ...
  untie(%db);

=head1 AUTHOR

Copyright 1999 by Leopold Toetsch <lt@toetsch.at>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

=head1 BUGS

rereading dumpfiles (on opening an existing cdb) doesn't check
length of keys and values, they are just matched like this:
   /(.*)->(.*)/

=head1 SEE ALSO

AnyDBM_File(3pm) Tie::Hash(3pm) http://cr.yp.to/cdb.html

=cut

use strict;
use vars qw(@ISA);
require Tie::Hash;

BEGIN {
    @ISA=qw(Tie::StdHash);
}

sub TIEHASH  {
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $file = shift;
    my $self = {};
    $self->{file} = $file;
    if (-e "$file") {	# read old
	open (FH, $file);
	while (<FH>) {
	    chomp;
	    if(/\+\d+,\d+:(.*)->(.*)/) {
    		$self->{data}{$1}=$2;
	    }
	}
	close FH;
    }
    open(FH, ">$file");
    $self->{'fh'} = \*FH;
    bless $self, $class;
    $self;
}

sub STORE {
    my ($self, $key, $value) = @_;
    $self->{data}{$key} = $value;
}

sub FETCH {
    $_[0]->{data}{$_[1]};
}

sub EXISTS {
    exists $_[0]->{data}{$_[1]};
}

# write all on closing
sub DESTROY {
    my $self = $_[0];
    my $fh = $self->{fh};
    my ($key, $value);
    while (($key, $value) = each(%{ $self->{data} })) {
	print $fh "+",length($key),",",length($value),":",$key,"->",$value,"\n";
    }
}
1;
