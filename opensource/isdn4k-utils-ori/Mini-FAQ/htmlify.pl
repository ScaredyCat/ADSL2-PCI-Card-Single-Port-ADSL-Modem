#!/usr/bin/perl

&doit("all");
exit(0);

sub doit
{
local($part) = @_;
local($lang) = "german";

if($part eq "all")
{
  print'<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">' . "\n";
  print'<html><head><title>English-German mini-FAQ for ISDN4Linux</title></head>';
  print'<body text="#000000" link="#0000ff" vlink="#000080" bgcolor="#ffffff">';
  print"<pre>\n";
}

open(INFILE, $ARGV[0]) || die("Couldn't open $ARGV[0]") if($part eq "all");
open(INFILE2, $ARGV[0]) || die("Couldn't open $ARGV[0]") if($part ne "all");

while($line= (($part eq "all") ? <INFILE> : <INFILE2>))
{
  $line =~ s/&/&amp;/g;
  $line =~ s/</&lt;/g;
  $line =~ s/>/&gt;/g;
  if($line =~ /http:/)
  {
    $line =~ s/(http:[^ \n]+)/<a href="\1">\1<\/a>/g;
  }
  elsif($line =~ /ftp:/)
  {
    $line =~ s/(ftp:[^ \n]+)/<a href="\1">\1<\/a>/g;
  }
  elsif($line =~ /www./)
  {
    $line =~ s/(www.[^ \n]+)/<a href="http:\/\/\1">\1<\/a>/;
  }
  $line =~ s/\&lt;([A-Za-z0-9_.-]+\@[a-zA-Z0-9.-]+)\&gt;/<a href="mailto:\1">&lt;\1&gt;<\/a>/g;
  if($line =~ /mini-faq f/i)
  {
    $line =~ s/^/<\/pre><h1>/;
    $line =~ s/\n/<\/h1><pre>/;
  }
  if($line =~ /see below/i)
  {
    $line =~ s/^/<\/pre><h1><a href="#toc0">/;
    $line =~ s/\n/<\/a><\/h1><pre>/;
  }
  if($line =~ /^0/) {$lang = "english"};
  if($lang eq "german")
  {
    if($line =~ /^[0-9]+\./)
    {
      if($part eq "all")
      {
        $line =~ s/^([0-9]+)\./<br><a name="frage\1">\1.<\/a><b>/;
      }
      else
      {
        $line =~ s/^([0-9]+)\./<br><a href="#frage\1">\1.<\/a><b>/;
      }
      $header = "yes";
    }
    $line =~ s/(Frage ([0-9]+))/<a href="#frage\2">\1<\/a>/g;
  }
  else
  {
    if($line =~ /^[0-9]+\./)
    {
      if($part eq "all")
      {
        $line =~ s/^([0-9]+)\./<br><a name="question\1">\1.<\/a><b>/;
      }
      else
      {
        $line =~ s/^([0-9]+)\./<br><a name="toc\1" href="#question\1">\1.<\/a><b>/;
      }
      $header = "yes";
    }
    $line =~ s/(question ([0-9]+))/<a href="#question\2">\1<\/a>/g;
  }
  $line =~ s/^ /<br> /;
  if($line =~ /^$/)
  {
    if($header eq "yes")
    {
      $line =~ s/^$/<\/b>/;
      $header = "no";
    }
    else
    {
      $line =~ s/^$//;
    }
  }
  if($line =~ /germantoc/ && $part eq "all")
  {
    &doit("germantoc");
  }
  elsif($line =~ /englishtoc/ && $part eq "all")
  {
    &doit("englishtoc");
  }
  elsif($part eq "all")
  {
    print $line;
  }
  elsif($part eq "germantoc" && $lang eq "german" &&
                  ($header eq "yes" || $line =~ /<\/b>/))
  {
    print $line;
  }
  elsif($part eq "englishtoc" && $lang eq "english" &&
                  ($header eq "yes" || $line =~ /<\/b>/))
  {
    print $line;
  }
}

close(INFILE2);

if($part eq "all")
{
  print"</pre>";
  print '<p> <a href="http://validator.w3.org/check/referer"><img border=0
       src="http://validator.w3.org/images/vh32"
       alt="Valid HTML 3.2!" height=31 width=88></a>
       </p>';
  print"</body></html>\n";
}

}
