#! /usr/bin/perl

use Globus::GRAM::Error;
use IO::File;
use Test;
use File::Path;
use File::Compare;

my (@tests, @todo) = ();
my $contact = $ENV{CONTACT_STRING};
my $testtmp = &make_tmpdir();
my $testdatadir = "$ENV{GLOBUS_LOCATION}/share/globus-gram-job-manager-test";

my @test_cases=qw(
    submit001
    submit002
    submit003
    submit004
    submit005
    submit006
    submit007
    submit008
    submit009
    submit010
    submit011
    submit012
    submit013
    submit014
    submit015
    submit016
    submit017
    submit018
    submit019
    submit020
    submit021
    submit022
    submit023
    submit024
    submit025
    submit026
    submit027
);

my @todo_cases=qw(
);

sub test_rsl
{
    my $testname = shift;
    my $additional_rsl = shift;
    my $testrsl = "$testname.rsl";
    my $additionalrslfile = "$testtmp/$testname.rsl";
    my $testout = "$testtmp/$testname.out";
    my $testerr = "$testtmp/$testname.err";
    my $test_rsl_fp = new IO::File("$testdatadir/$testrsl", '<');
    my $out_rsl_fp = new IO::File($additionalrslfile, '>');

    while(<$test_rsl_fp>)
    {
	$out_rsl_fp->print($_);
    }
    $test_rsl_fp->close();

    if($additional_rsl ne "")
    {
	$out_rsl_fp->print($additional_rsl);
    }
    $out_rsl_fp->close();


    system("globusrun -s -r \"$contact\" -f $additionalrslfile >$testout 2>$testerr");

    $rc = $? >> 8 ||
          File::Compare::compare("$testdatadir/$testname.out", $testout) ||
	  File::Compare::compare("$testdatadir/$testname.err", $testerr);

    ok("$testname:$additional_rsl:$rc", "$testname:$additional_rsl:0");
}

foreach(@test_cases)
{
    push(@tests, "test_rsl(\"$_\", \"\")");
    push(@tests, "test_rsl(\"$_\", \"(save_state = yes)\")");
}
foreach(@todo_cases)
{
    push(@tests, "test_cases(\"$_\", \"\")");
    push(@todo, scalar(@tests));
}

if(@ARGV)
{
    plan tests => scalar(@ARGV);
    foreach(@ARGV)
    {
	eval "&$tests[$_-1]";
    }
}
else
{
    plan tests => scalar(@tests), todo => \@todo;

    foreach (@tests)
    {
	eval "&$_";
    }
}

sub make_tmpdir
{
    my $root;
    my $suffix = '/gram_jobmanager_test_';
    my $created = 0;
    my $tmpname;
    my @acceptable = split(//, "abcdefghijklmnopqrstuvwxyz".
			       "ABCDEFGHIJKLMNOPQRSTUVWXYZ".
			       "0123456789");
    if(exists($ENV{TMPDIR}))
    {
	$root = $ENV{TMPDIR};
    }
    else
    {
	$root = '/tmp';
    }
    while($created == 0)
    {
	$tmpname = $root . $suffix .
	           $acceptable[rand() * $#acceptable] .
	           $acceptable[rand() * $#acceptable] .
	           $acceptable[rand() * $#acceptable] .
	           $acceptable[rand() * $#acceptable] .
	           $acceptable[rand() * $#acceptable] .
	           $acceptable[rand() * $#acceptable];
	$created = mkdir($tmpname, 0700);
	if($created)
	{
	    if(-l $tmpname or ! -d $tmpname or ! -o $tmpname)
	    {
		$created = 0;
	    }
	}
    }
    return $tmpname;
}

END
{
    if(-d $testtmp and -o $testtmp)
    {
	File::Path::rmtree($testtmp);
    }
}
