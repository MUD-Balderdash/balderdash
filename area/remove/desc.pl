#!/usr/bin/perl

$descfile = "descr.txt";
$path = "/usr/local/mud";

opendir(DIR, "$path/player"); 
@files = readdir(DIR);
close(DIR);

$count = 0;

open(DESC, ">$path/$descfile");

foreach $file (@files)
{
  open(FH,"$path/player/$file");
  $pname = "";
  $descr = "";

  if (($file ne ".") && ($file ne "..") && (index($file,"~") < 0))
  {
    while (<FH>)
    {
      s/[\n\r~]//g;

      @tmp=split(/\s+/, $_, 2);

      if (@tmp[0] eq "Race")
      {
        last;
      }

      if (@tmp[0] eq "Name")
      {
        $pname = @tmp[1];
      }

      if (@tmp[0] eq "Desc")
      {
        @tmp[1] =~ s/\{.//g;

        $descr = @tmp[1] . "\n";

        while(<FH>)
        {
          s/[\n\r]//g;
          s/\{.//g;

          $descr .= $_ . "\n";
          if (/\~/)
          {
            last;
          }
        }
        print DESC "$pname:\n$descr\n\n";
        $count++;
        last;
      }
    }
  }
  close(FH);
}

print "Total: $count\n";
close(DESC);
