#!/usr/bin/perl

use HTML::Template;

my %values = ();
my @spells = ();
my @gsns = ();

my $tpl = new HTML::Template(filename	=>	"cygwin.c.tmpl");

my $spell_file = "magic.h";
my $gsn_file = "merc.h";

my $line;

open(FILE, "<$spell_file");
while(($line = <FILE>))
{
    my %value;

    chomp($line);
    if ($line =~ s/^DECLARE_SPELL_FUN\(\s*(\w+)\s*\);.*$/$1/g)
    {
	$value{'SPELL_NAME'} = $line;
	push(@spells, \%value);
    }
}

$values{'SPELLS'} = \@spells;
close(FILE);

open(FILE, "<$gsn_file");
while(($line = <FILE>))
{
    my %value;

    chomp($line);
    if ($line =~ s/^GSN\(\s*(\w+)\s*\);.*$/$1/g)
    {
	$value{'GSN'} = $line;
	push(@gsns, \%value);
    }
}

$values{'GSNS'} = \@gsns;
close(FILE);


$tpl->param(\%values);
print $tpl->output();
